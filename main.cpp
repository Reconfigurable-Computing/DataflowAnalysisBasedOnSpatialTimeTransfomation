
#include "Eigen/Core"
#include "Eigen/Dense"
#include "include/datastruct/arch.h"
#include "include/datastruct/mapping.h"
#include "include/datastruct/workload.h"
#include "include/util/debug.h"
#include "include/util/eigenUtil.h"
#include <assert.h>
#include <iostream>
#include <map>
#include <math.h>
#include <memory>
#include <set>
#include <string>
#include <vector>
typedef double valueType;
struct Result {
  int uniqueVolumn[3]; // input weight output
  int totalVolumn[3];
  int reuseVolumn[3];
  int baseCycle;
  int baseData[3];
  int delay;
  Result() {
    for (int i = 0; i < 3; i++) {
      uniqueVolumn[i] = 0;
      totalVolumn[i] = 0;
      reuseVolumn[i] = 0;
      baseData[i] = 0;
    }
    baseCycle = 0;
    delay = 0;
  }
};
class Analyzer {
private:
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> &_coupledVarVec;
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> &_lowerVarVec;
  MAPPING::Transform &_T;
  WORKLOAD::Tensor &_I;
  WORKLOAD::Tensor &_W;
  WORKLOAD::Tensor &_O;
  ARCH::Level &_L1;
  MAPPING::Access &_accessI;
  MAPPING::Access &_accessW;
  MAPPING::Access &_accessO;
  Result _result;
  std::shared_ptr<std::vector<std::vector<int>>> _reuseVecI;
  std::shared_ptr<std::vector<std::vector<int>>> _reuseVecW;
  std::shared_ptr<std::vector<std::vector<int>>> _reuseVecO;
  std::map<ARCH::DATATYPE, std::shared_ptr<std::vector<std::vector<int>>>>
      _reuseVecMap;
  std::shared_ptr<WORKLOAD::Iterator> PEX;
  std::shared_ptr<WORKLOAD::Iterator> PEY;

public:
  Analyzer(std::vector<std::shared_ptr<WORKLOAD::Iterator>> &coupledVarVec,
           std::vector<std::shared_ptr<WORKLOAD::Iterator>> &lowerVarVec,
           MAPPING::Transform &T, int baseCycle, int baseData[3],
           WORKLOAD::Tensor &I, WORKLOAD::Tensor &W, WORKLOAD::Tensor &O,
           ARCH::Level &L1, MAPPING::Access &accessI, MAPPING::Access &accessW,
           MAPPING::Access &accessO,
           std::shared_ptr<std::vector<std::vector<int>>> reuseVecI,
           std::shared_ptr<std::vector<std::vector<int>>> reuseVecW,
           std::shared_ptr<std::vector<std::vector<int>>> reuseVecO)
      : _coupledVarVec(coupledVarVec), _T(T), _I(I), _W(W), _O(O), _L1(L1),
        _accessI(accessI), _accessW(accessW), _accessO(accessO),
        _lowerVarVec(lowerVarVec), _reuseVecI(reuseVecI), _reuseVecO(reuseVecO),
        _reuseVecW(reuseVecW) {
    int colNum = _T.getColNum();
    for (int i = 0; i < colNum; i++) {
      if (_T(0, i) == 1)
        PEX = _coupledVarVec[i];
      if (_T(1, i) == 1)
        PEY = _coupledVarVec[i];
    }
    _result.baseCycle = baseCycle;
    for (int i = 0; i < 3; i++) {
      _result.baseData[i] = baseData[i];
    }
    _reuseVecMap[ARCH::INPUT] = _reuseVecI;
    _reuseVecMap[ARCH::WEIGHT] = _reuseVecW;
    _reuseVecMap[ARCH::OUTPUT] = _reuseVecO;

    int coupleVarNum = _coupledVarVec.size();
    recursiveEdgeAnalysis(0, coupleVarNum, 0);
  }
  void recursiveEdgeAnalysis(int index, int coupleVarNum, bool isEdge) {
    if (index < coupleVarNum) {
      if (_coupledVarVec[index]->hasEdge()) {
        recursiveEdgeAnalysis(index + 1, coupleVarNum, isEdge);
        _coupledVarVec[index]->setEdge();
        recursiveEdgeAnalysis(index + 1, coupleVarNum, true);
        _coupledVarVec[index]->unsetEdge();
      } else {
        recursiveEdgeAnalysis(index + 1, coupleVarNum, isEdge);
      }
    } else {
      oneAnalysis(isEdge);
    }
  }

  void oneAnalysis(bool isEdge) {
    Result result;
    getComputationDelay();
    accessAnalysis(result, isEdge);
    _result.delay += getStableDelay(1);
  }

  std::pair<int, int> getTRange(int row) {
    int colNumT = _T.getColNum();
    std::shared_ptr<WORKLOAD::Polynomial> dim =
        std::make_shared<WORKLOAD::Polynomial>();
    for (int i = 0; i < colNumT; i++) {
      if (_T(row, i) == 1)
        dim = dim + _coupledVarVec[i];
    }
    std::pair<int, int> range = dim->getRange();
    return range;
  }

  int getActivePENum() {
    std::pair<int, int> PEXRange = getTRange(0);
    std::pair<int, int> PEYRange = getTRange(1);
    return (PEYRange.second - PEYRange.first + 1) *
           (PEXRange.second - PEXRange.first + 1);
  }

  int getComputationDelay() {
    int TDimNum = _T.getColNum();
    auto Tmatrix = _T.getMatrix();
    int totalActiveNum = 1;
    for (auto var : _coupledVarVec) {
      totalActiveNum *= var->getSize();
    }
    int activePENum = getActivePENum();

    return totalActiveNum / activePENum;
  }
  int getPerPEVolumn(std::set<int> &innerTimeSet) {
    PEX->lock();
    PEY->lock();
    int ret = getTimeSize(innerTimeSet);
    PEX->unlock();
    PEY->unlock();
    return ret;
  }
  int getInnerUniqueAccess(ARCH::DATATYPE dataType, MAPPING::Access &access,
                           bool isEdge, std::set<int> &innerTimeSet,
                           std::set<int> &outerTimeSet, Result &result) {

    ARCH::NETWORKTYPE networkType = _L1.getNetworkType(dataType);
    std::pair<int, int> PEXRange = getTRange(0);
    std::pair<int, int> PEYRange = getTRange(1);
    int perPEVolumn = getPerPEVolumn(innerTimeSet);
    if (networkType == ARCH::SYSTOLIC || networkType == ARCH::MULTICAST) {

      std::vector<std::pair<int, int>> accessPointSet;
      _L1.getAccessPoint(dataType, accessPointSet);
      int activeAccessPointNum = 0;
      for (auto accessPoint : accessPointSet) {
        if (accessPoint.first >= PEXRange.first &&
            accessPoint.first <= PEXRange.second &&
            accessPoint.second >= PEYRange.first &&
            accessPoint.second <= PEYRange.second) {
          activeAccessPointNum++;
        }
      }
      int uniqueVolumn = perPEVolumn * activeAccessPointNum;
      return uniqueVolumn;
    } else if (networkType == ARCH::STATIONARY) {
      int uniqueVolumn = (PEYRange.second - PEYRange.first + 1) *
                         (PEXRange.second - PEXRange.first + 1) * 1;
      return uniqueVolumn;
    } else {
      int uniqueVolumn = (PEYRange.second - PEYRange.first + 1) *
                         (PEXRange.second - PEXRange.first + 1) * perPEVolumn;
      return uniqueVolumn;
    }
  }
  void accessAnalysis(Result &result, bool isEdge) {
    std::set<int> innerTimeSet;
    std::set<int> outerTimeSet;
    getInnerOuterTimeSet(innerTimeSet, outerTimeSet);
    int innerTimeSize = getTimeSize(innerTimeSet);
    int outerTimeSize = getTimeSize(outerTimeSet);
    int totalVolumn = 1;
    for (auto var : _coupledVarVec) {
      totalVolumn *= var->getSize();
    }

    result.uniqueVolumn[ARCH::OUTPUT] = getInnerUniqueAccess(
        ARCH::OUTPUT, _accessO, isEdge, innerTimeSet, outerTimeSet, result);
    result.uniqueVolumn[ARCH::INPUT] = getInnerUniqueAccess(
        ARCH::INPUT, _accessI, isEdge, innerTimeSet, outerTimeSet, result);
    result.uniqueVolumn[ARCH::WEIGHT] = getInnerUniqueAccess(
        ARCH::WEIGHT, _accessW, isEdge, innerTimeSet, outerTimeSet, result);
    result.uniqueVolumn[ARCH::OUTPUT] *= outerTimeSize;
    result.uniqueVolumn[ARCH::INPUT] *= outerTimeSize;
    result.uniqueVolumn[ARCH::WEIGHT] *= outerTimeSize;
    result.totalVolumn[ARCH::OUTPUT] = totalVolumn;
    result.totalVolumn[ARCH::INPUT] = totalVolumn;
    result.totalVolumn[ARCH::WEIGHT] = totalVolumn;
    result.reuseVolumn[ARCH::OUTPUT] =
        totalVolumn - result.uniqueVolumn[ARCH::OUTPUT];
    result.reuseVolumn[ARCH::INPUT] =
        totalVolumn - result.uniqueVolumn[ARCH::INPUT];
    result.reuseVolumn[ARCH::WEIGHT] =
        totalVolumn - result.uniqueVolumn[ARCH::WEIGHT];
  }

  bool checkValidInnerDim(int varIndex, ARCH::DATATYPE dataType) {
    std::shared_ptr<std::vector<std::vector<int>>> reuseVecSet =
        _reuseVecMap[dataType];
    int m = (*reuseVecSet).size();
    int n = (*reuseVecSet)[0].size();
    for (auto reuseVecItem : (*reuseVecSet)) {
      int flag = 1;
      for (int j = 0; j < n; j++) {
        if (j == varIndex) {
          if (reuseVecItem[j] != 1) {
            flag = 0;
          }
        } else {
          if (reuseVecItem[j] == 1) {
            flag = 0;
          }
        }
      }
      if (flag) {
        return true;
      }
    }
    return false;
  }
  void constructSingleDataTypeSet(int flag, std::set<int> &singleDataTypeSet,
                                  ARCH::DATATYPE dataType) {
    int coupleVarNum = _coupledVarVec.size();
    if (flag) {
      for (int i = 2; i < coupleVarNum; i++) {
        if (checkValidInnerDim(i, dataType)) {
          singleDataTypeSet.insert(i);
        } else {
          break;
        }
      }
    } else {
      for (int i = 2; i < coupleVarNum; i++) {
        singleDataTypeSet.insert(i);
      }
    }
  }

  void getInnerOuterTimeSet(std::set<int> &innerTimeSet,
                            std::set<int> &outerTimeSet) {
    int coupleVarNum = _coupledVarVec.size();
    int inputIfStationary = _L1.checkIfStationary(ARCH::INPUT);
    int weightIfStationary = _L1.checkIfStationary(ARCH::WEIGHT);
    int outputIfStationary = _L1.checkIfStationary(ARCH::OUTPUT);
    std::set<int> inputSet;
    std::set<int> weightSet;
    std::set<int> outputSet;
    constructSingleDataTypeSet(inputIfStationary, inputSet, ARCH::INPUT);
    constructSingleDataTypeSet(weightIfStationary, weightSet, ARCH::WEIGHT);
    constructSingleDataTypeSet(outputIfStationary, outputSet, ARCH::OUTPUT);
    for (int i = 2; i < coupleVarNum; i++) {
      if (inputSet.count(i) && weightSet.count(i) && outputSet.count(i)) {
        innerTimeSet.insert(i);
      } else {
        break;
      }
    }
    for (int i = 2; i < coupleVarNum; i++) {
      if (!innerTimeSet.count(i)) {
        outerTimeSet.insert(i);
      }
    }
  }
  int getTimeSize(std::set<int> &timeSet) {
    std::pair<int, int> timeRange;
    int timeSize = 1;
    for (auto timeIndex : timeSet) {
      timeRange = getTRange(timeIndex);
      timeSize *= timeRange.second - timeRange.first + 1;
    }
    return timeSize;
  }

  int getStableDelay(int compDelay) {
    int doubleBufferFlag = 1;
    int lowestFlag = 1;
    std::pair<int, int> PEXRange = getTRange(0);
    std::pair<int, int> PEYRange = getTRange(1);
    int inputStableDelay = _L1.getStableDelay(ARCH::INPUT, 1, 16);
    int weightStableDelay = _L1.getStableDelay(ARCH::WEIGHT, 1, 16);
    int outputStableDelay = _L1.getStableDelay(ARCH::OUTPUT, 1, 16);
    int inputInitDelay =
        _L1.getInitOrOutDelay(ARCH::INPUT, 1, 16, PEXRange, PEYRange);
    int weightInitDelay =
        _L1.getInitOrOutDelay(ARCH::WEIGHT, 1, 16, PEXRange, PEYRange);
    int outputOutDelay =
        _L1.getInitOrOutDelay(ARCH::OUTPUT, 1, 16, PEXRange, PEYRange);
    int stableDelay =
        std::max(std::max(std::max(inputStableDelay, weightStableDelay),
                          outputStableDelay),
                 compDelay);
    int coupleVarNum = _coupledVarVec.size();
    std::set<int> innerTimeSet;
    std::set<int> outerTimeSet;
    getInnerOuterTimeSet(innerTimeSet, outerTimeSet);
    int innerTimeSize = getTimeSize(innerTimeSet);
    int outerTimeSize = getTimeSize(outerTimeSet);
    int delay;
    if (lowestFlag == 1 || doubleBufferFlag == 0) {
      delay =
          innerTimeSize * stableDelay +
          std::max(std::max(inputInitDelay, weightInitDelay), outputOutDelay);
    } else // doublebuffer
    {
      delay = std::max(
          std::max(std::max(inputInitDelay, weightInitDelay), outputOutDelay),
          innerTimeSize * stableDelay);
    }
    delay *= outerTimeSize;
    return delay;
  }

  void compSubTensorSize() {}
};

// std::shared_ptr<std::vector<int>>

int main() {

  // rank
  // c1 c2   c1 outer than c2
  // pedim only one
  // MAPPING::Transform T(4, std::make_shared<std::vector<int>>(
  //    std::vector<int>{1,0,0,0
  //    ,0,1,0,0
  //    ,1,1,1,0
  //    ,0,0,0,1}));
  // MAPPING::Transform T(3, std::make_shared<std::vector<int>>(
  //    std::vector<int>{1, 0, 0
  //    , 0, 1, 0
  //    , 1, 1, 1}));
  MAPPING::Transform T(
      6, std::make_shared<std::vector<int>>(std::vector<int>{
             1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0,
             0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1}));
  ARCH::Level L1(16);
  L1.appendArray(32, 16, 16);
  L1.appendBuffer(ARCH::REG, ARCH::INPUT, 128, 16);
  L1.appendBuffer(ARCH::REG, ARCH::WEIGHT, 128, 16);
  L1.appendBuffer(ARCH::REG, ARCH::OUTPUT, 128, 16);
  L1.appendNetworkGroup(32, 16, ARCH::INPUT, 16, {1, 0, 1});
  L1.appendNetworkGroup(32, 16, ARCH::WEIGHT, 16, {0, 1, 1});
  L1.appendNetworkGroup(32, 16, ARCH::OUTPUT, 16, {0, 0, 1});

  std::shared_ptr<WORKLOAD::Iterator> k =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 3, "k"));
  std::shared_ptr<WORKLOAD::Iterator> c =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 10, "c"));
  std::shared_ptr<WORKLOAD::Iterator> c2 =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 3, "c2"));
  std::shared_ptr<WORKLOAD::Iterator> c1 = std::make_shared<WORKLOAD::Iterator>(
      WORKLOAD::Iterator(0, 1, 0, 2, c2, "c1"));
  std::shared_ptr<WORKLOAD::Iterator> y =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 10, "y"));
  std::shared_ptr<WORKLOAD::Iterator> y2 =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 5, "y2"));
  std::shared_ptr<WORKLOAD::Iterator> y1 = std::make_shared<WORKLOAD::Iterator>(
      WORKLOAD::Iterator(0, 5, 0, 4, y2, "y1"));
  std::shared_ptr<WORKLOAD::Iterator> x =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 2, "x"));
  std::shared_ptr<WORKLOAD::Iterator> p =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 3, "p"));
  std::shared_ptr<WORKLOAD::Iterator> q =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 3, "q"));
  WORKLOAD::Tensor I("I");
  WORKLOAD::Tensor W("W");
  WORKLOAD::Tensor O("O");
  // I[c1 * 4 + c2] [y1 * 6 + y2 + p][x + q];
  // W[k] [c1 * 4 + c2][ p] [q];
  // O[k][y1 * 6 + y2][ x];
  // I[c][y1 * 6 + y2 + p][x + q];
  // W[k][c][p][q];
  // O[k][y1 * 6 + y2][x];
  I[c][y][x + q];
  W[k][c][p][q];
  O[k][y][x];
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> coupledVarVec;
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> upperVarVec;
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> lowerVarVec;
  // coupledVarVec.push_back(k);
  // coupledVarVec.push_back(x);
  // coupledVarVec.push_back(c2);
  // coupledVarVec.push_back(c1);
  coupledVarVec.push_back(k);
  coupledVarVec.push_back(x);
  coupledVarVec.push_back(c);
  coupledVarVec.push_back(p);
  coupledVarVec.push_back(q);
  coupledVarVec.push_back(y);
  // coupledVarVec.push_back(k);
  // coupledVarVec.push_back(x);
  // coupledVarVec.push_back(c);
  // coupledVarVec.push_back(p);
  // coupledVarVec.push_back(y);
  // coupledVarVec.push_back(q);
  lowerVarVec.push_back(y);
  lowerVarVec.push_back(q);

  getTimeLine(coupledVarVec, T, I, W, O);

  std::cout << I.getRange(1).first << ' ' << I.getRange(1).second << std::endl;
  I.bindVar(coupledVarVec);
  W.bindVar(coupledVarVec);
  O.bindVar(coupledVarVec);

  MAPPING::Access accessI = MAPPING::constructAccessMatrix(I, coupledVarVec);
  MAPPING::Access accessW = MAPPING::constructAccessMatrix(W, coupledVarVec);
  MAPPING::Access accessO = MAPPING::constructAccessMatrix(O, coupledVarVec);

  std::shared_ptr<std::vector<std::vector<int>>> reuseVecI =
      compReuseVec(T, accessI);
  std::shared_ptr<std::vector<std::vector<int>>> reuseVecW =
      compReuseVec(T, accessW);
  std::shared_ptr<std::vector<std::vector<int>>> reuseVecO =
      compReuseVec(T, accessO);
  int baseData[3] = {1, 1, 1};
  Analyzer analyzer(coupledVarVec, lowerVarVec, T, 1, baseData, I, W, O, L1,
                    accessI, accessW, accessO, reuseVecI, reuseVecW, reuseVecO);
  return 0;
}