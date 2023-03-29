
#include "Eigen/Core"
#include "Eigen/Dense"
#include "include/datastruct/arch.h"
#include "include/datastruct/mapping.h"
#include "include/datastruct/workload.h"
#include "include/util/eigenUtil.h"
#include <assert.h>
#include <iostream>
#include <map>
#include <math.h>
#include <memory>
#include <string>
#include <vector>
// using Eigen::MatrixXd;
using namespace Eigen;
using namespace Eigen::internal;
using namespace Eigen::Architecture;
typedef double valueType;
struct Result {
  int perPEVolumn[3]; // input weight output
  int uniqueVolumn[3];
  int totalVolumn[3];
  int reuseVolumn[3];
  int baseCycle;
  int baseData[3];
  Result() {
    for (int i = 0; i < 3; i++) {
      perPEVolumn[i] = 0;
      uniqueVolumn[i] = 0;
      totalVolumn[i] = 0;
      reuseVolumn[i] = 0;
      baseData[i] = 0;
    }
    baseCycle = 0;
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

public:
  Analyzer(std::vector<std::shared_ptr<WORKLOAD::Iterator>> &coupledVarVec,
           std::vector<std::shared_ptr<WORKLOAD::Iterator>> &lowerVarVec,
           MAPPING::Transform &T, int baseCycle, int baseData[3],
           WORKLOAD::Tensor &I, WORKLOAD::Tensor &W, WORKLOAD::Tensor &O,
           ARCH::Level &L1, MAPPING::Access &accessI, MAPPING::Access &accessW,
           MAPPING::Access &accessO)
      : _coupledVarVec(coupledVarVec), _T(T), _I(I), _W(W), _O(O), _L1(L1),
        _accessI(accessI), _accessW(accessW), _accessO(accessO),
        _lowerVarVec(lowerVarVec) {
    _result.baseCycle = baseCycle;
    for (int i = 0; i < 3; i++) {
      _result.baseData[i] = baseData[i];
    }

    getComputationDelay();
    accessAnalysis(ARCH::OUTPUT, _accessO);
    accessAnalysis(ARCH::INPUT, _accessI);
    accessAnalysis(ARCH::WEIGHT, _accessW);
    getDelay(1);
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

  int getPerPEVolumn(MAPPING::Access &access) {
    int rowNum = access.getRowNum();
    int colNum = access.getColNum();
    int oneAccessPointUniqueVolumn = 1;
    // std::vector<std::shared_ptr<WORKLOAD::Polynomial>> dimSet;
    int totalUniqueVolumn = 0;
    for (int i = 0; i < rowNum; i++) {
      std::shared_ptr<WORKLOAD::Polynomial> dim =
          std::make_shared<WORKLOAD::Polynomial>();
      for (int j = 0; j < colNum; j++) {
        if (access(i, j) && _T(0, j) == 0 && _T(1, j) == 0)
          dim = dim + access(i, j) * _coupledVarVec[j];
      }
      // dimSet.push_back(dim);
      oneAccessPointUniqueVolumn *= dim->getSize();
    }
    return oneAccessPointUniqueVolumn;
  }

  void accessAnalysis(ARCH::DATATYPE dataType, MAPPING::Access &access) {
    int uniqueVolumn;
    int totalVolumn = 1;
    for (auto var : _coupledVarVec) {
      totalVolumn *= var->getSize();
    }
    int reuseVolumn;
    ARCH::NETWORKTYPE networkType = _L1.getNetworkType(dataType);
    int perPEVolumn = getPerPEVolumn(access);
    std::pair<int, int> PEXRange = getTRange(0);
    std::pair<int, int> PEYRange = getTRange(1);
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
      uniqueVolumn = perPEVolumn * activeAccessPointNum;
      reuseVolumn = totalVolumn - uniqueVolumn;
    } else {
      uniqueVolumn = (PEYRange.second - PEYRange.first + 1) *
                     (PEXRange.second - PEXRange.first + 1) * perPEVolumn;
      reuseVolumn = totalVolumn - uniqueVolumn;
    }
    _result.perPEVolumn[dataType] = perPEVolumn;
    _result.uniqueVolumn[dataType] = uniqueVolumn;
    _result.reuseVolumn[dataType] = reuseVolumn;
    _result.totalVolumn[dataType] = totalVolumn;
  }

  void getDelay(int baseDelay) {
    std::pair<int, int> PEXRange = getTRange(0);
    std::pair<int, int> PEYRange = getTRange(1);

    int inputInitDelay = _L1.getInitOrOutDelay(ARCH::INPUT, 1, 16);
    int weightInitDelay = _L1.getInitOrOutDelay(ARCH::WEIGHT, 1, 16);
    int outputOutDelay = _L1.getInitOrOutDelay(ARCH::OUTPUT, 1, 16);

    int inputStableDelay = _L1.getStableDelay(ARCH::INPUT, 1, 16);
    int weightStableDelay = _L1.getStableDelay(ARCH::WEIGHT, 1, 16);
    int outputStableDelay = _L1.getStableDelay(ARCH::OUTPUT, 1, 16);

    int stableDelay =
        std::max(std::max(std::max(inputStableDelay, weightStableDelay),
                          outputStableDelay),
                 baseDelay);
    int coupleVarNum = _coupledVarVec.size();
    auto timeRange = getTRange(2);
    int timeSize = timeRange.second - timeRange.first + 1;
    for (int i = 3; i < coupleVarNum; i++) {
      timeRange = getTRange(i);
      timeSize *= timeRange.second - timeRange.first + 1;
    }
    int delay = timeSize * stableDelay +
                std::max(inputInitDelay, weightInitDelay) + outputOutDelay;
  }
  void compSubTensorSize() {}
};

// std::shared_ptr<std::vector<int>>

int main() {

  MAPPING::Transform T(3, std::make_shared<std::vector<int>>(
                              std::vector<int>{1, 0, 0, 0, 1, 0, 1, 1, 1}));
  ARCH::Level L1(16);
  L1.appendArray(32, 32, 16);
  L1.appendBuffer(ARCH::REG, ARCH::INPUT, 128, 16);
  L1.appendBuffer(ARCH::REG, ARCH::WEIGHT, 128, 16);
  L1.appendBuffer(ARCH::REG, ARCH::OUTPUT, 128, 16);
  L1.appendNetworkGroup(32, 32, ARCH::INPUT, 16, {1, 0, 1});
  L1.appendNetworkGroup(32, 32, ARCH::WEIGHT, 16, {0, 1, 1});
  L1.appendNetworkGroup(32, 32, ARCH::OUTPUT, 16, {0, 0, 1});

  std::shared_ptr<WORKLOAD::Iterator> k =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 3, {'k'}));
  std::shared_ptr<WORKLOAD::Iterator> c =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 4, {'c'}));
  std::shared_ptr<WORKLOAD::Iterator> y =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 31, {'y'}));
  std::shared_ptr<WORKLOAD::Iterator> x =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 2, {'x'}));
  std::shared_ptr<WORKLOAD::Iterator> p =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 3, {'p'}));
  std::shared_ptr<WORKLOAD::Iterator> q =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 3, {'q'}));
  WORKLOAD::Tensor I("I");
  WORKLOAD::Tensor W("W");
  WORKLOAD::Tensor O("O");
  I[c][y + p][x + q];
  W[k][c][p][q];
  O[k][y][x];

  std::cout << I.getRange(1).first << ' ' << I.getRange(1).second << std::endl;
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> coupledVarVec;
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> upperVarVec;
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> lowerVarVec;
  coupledVarVec.push_back(k);
  coupledVarVec.push_back(x);
  coupledVarVec.push_back(c);
  lowerVarVec.push_back(y);
  lowerVarVec.push_back(p);
  lowerVarVec.push_back(q);

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
                    accessI, accessW, accessO);
  return 0;
}