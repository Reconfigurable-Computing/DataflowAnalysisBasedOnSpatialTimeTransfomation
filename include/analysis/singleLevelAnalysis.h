#pragma once

#include "include/datastruct/arch.h"
#include "include/datastruct/mapping.h"
#include "include/datastruct/workload.h"
#include "include/util/debug.h"
#include "include/util/eigenUtil.h"
#include "include/util/timeline.h"
#include <set>
#include <vector>
struct Base {
  int baseData[3];
  int baseCompCycle;
  Base() {
    for (int i = 0; i < 3; i++) {
      baseData[i] = 1;
    }
    baseCompCycle = 1;
  }
  Base(int newBaseCompCycle, int newBaseData[3]) {
    baseCompCycle = newBaseCompCycle;
    for (int i = 0; i < 3; i++) {
      baseData[i] = newBaseData[i];
    }
  }
};
struct Result {
  int uniqueVolumn[3]; // input weight output
  int totalVolumn[3];
  int reuseVolumn[3];
  int requiredDataSize[3];
  int delay;
  int compCycle;
  int occTimes;
  float compRate;
  int activePEMultTimeNum;
  int totalPEMultTimeNum;
  float PEUtilRate;
  std::vector<std::shared_ptr<Result>> subLevelResultVec;
  Result() { reset(); }
  void reset() {
    for (int i = 0; i < 3; i++) {
      uniqueVolumn[i] = 0;
      totalVolumn[i] = 0;
      reuseVolumn[i] = 0;
      requiredDataSize[i] = 0;
    }
    delay = 0;
    occTimes = 0;
    compCycle = 0;
    compRate = 0;
    activePEMultTimeNum = 0;
    totalPEMultTimeNum = 0;
    PEUtilRate = 0;
  }
  Result &operator+=(Result &other) {
    for (int i = 0; i < 3; i++) {
      uniqueVolumn[i] += other.uniqueVolumn[i] * other.occTimes;
      totalVolumn[i] += other.totalVolumn[i] * other.occTimes;
      reuseVolumn[i] += other.reuseVolumn[i] * other.occTimes;
      requiredDataSize[i] =
          std::max(requiredDataSize[i], other.requiredDataSize[i]);
    }
    delay = std::max(delay, other.delay);
    compRate += other.delay * other.occTimes;
    compCycle += other.compCycle * other.occTimes;
    occTimes += other.occTimes;
    activePEMultTimeNum += other.activePEMultTimeNum * other.occTimes;
    totalPEMultTimeNum += other.totalPEMultTimeNum * other.occTimes;
    PEUtilRate = 0;
    return *this;
  }
};

class Analyzer {
private:
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> &_coupledVarVec;
  MAPPING::Transform &_T;
  WORKLOAD::Tensor &_I;
  WORKLOAD::Tensor &_W;
  WORKLOAD::Tensor &_O;
  ARCH::Level &_L;
  MAPPING::Access _accessI;
  MAPPING::Access _accessW;
  MAPPING::Access _accessO;
  std::shared_ptr<Result> _result;
  std::shared_ptr<std::vector<std::vector<int>>> _reuseVecI;
  std::shared_ptr<std::vector<std::vector<int>>> _reuseVecW;
  std::shared_ptr<std::vector<std::vector<int>>> _reuseVecO;
  std::map<ARCH::DATATYPE, std::shared_ptr<std::vector<std::vector<int>>>>
      _reuseVecMap;
  std::shared_ptr<WORKLOAD::Iterator> PEX;
  std::shared_ptr<WORKLOAD::Iterator> PEY;
  bool _doubleBufferFlag;
  bool _lowestFlag;

  std::vector<Base> _baseSet;
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> _curSubCoupledVarVec;
  std::set<std::shared_ptr<WORKLOAD::Iterator>> _curSubCoupledVarSet;
  int _curBaseIndex;

  std::pair<int, int> compTRange(int row);
  bool checkValidInnerDim(int varIndex, ARCH::DATATYPE dataType);
  void constructSingleDataTypeTimeSet(int flag,
                                      std::set<int> &singleDataTypeSet,
                                      ARCH::DATATYPE dataType);
  void constructInnerOuterTimeVec(std::vector<int> &innerTimeVec,
                                  std::vector<int> &outerTimeVec);
  int compOneStateTimeSize(std::vector<int> &timeVec);
  void generateVarVec(std::vector<int> &timeVec,
                      std::vector<std::shared_ptr<WORKLOAD::Iterator>> &varVec);
  void compOneDataVolumn(ARCH::DATATYPE dataType, MAPPING::Access &access,
                         std::vector<int> &innerTimeVec, int outerTimeSize);
  int compOneStateStableDelay();
  int compOneStateInitDelay(std::pair<int, int> &PEXRange,
                            std::pair<int, int> &PEYRange);
  void compOneStateDelay(std::vector<int> &innerTimeVec, int &delay,
                         int &compCycle, int &initDelay,
                         int &activePEMultTimeNum);
  void accessAnalysis(std::vector<int> &innerTimeVec,
                      std::vector<int> &outerTimeVec);
  void delayAnalysis(std::vector<int> &innerTimeVec,
                     std::vector<int> &outerTimeVec);
  void setEdge(std::shared_ptr<WORKLOAD::Iterator> curI);
  void unsetEdge(std::shared_ptr<WORKLOAD::Iterator> curI);
  void changeBase();
  void compOneStateVolumn(int &uniqueVolumn, int &totalVolumn,
                          std::vector<int> &innerTimeVec,
                          ARCH::DATATYPE dataType);
  void
  changeEdgeByState(bool flag, int varNum, int i,
                    std::vector<std::vector<int>> &state,
                    std::vector<std::shared_ptr<WORKLOAD::Iterator>> &varVec);

public:
  Analyzer(std::vector<std::shared_ptr<WORKLOAD::Iterator>> &coupledVarVec,
           MAPPING::Transform &T, WORKLOAD::Tensor &I, WORKLOAD::Tensor &W,
           WORKLOAD::Tensor &O, ARCH::Level &L, bool lowestFlag,
           bool doubleBufferFlag)
      : _coupledVarVec(coupledVarVec), _T(T), _I(I), _W(W), _O(O), _L(L),
        _accessI(MAPPING::constructAccessMatrix(I, coupledVarVec)),
        _accessW(MAPPING::constructAccessMatrix(W, coupledVarVec)),
        _accessO(MAPPING::constructAccessMatrix(O, coupledVarVec)),
        _doubleBufferFlag(doubleBufferFlag), _lowestFlag(lowestFlag) {

    _reuseVecI = compReuseVec(T, _accessI);
    _reuseVecW = compReuseVec(T, _accessW);
    _reuseVecO = compReuseVec(T, _accessO);
    // getTimeLine(coupledVarVec, T, I, W, O);

    int colNum = _T.getColNum();
    for (int i = 0; i < colNum; i++) {
      if (_T(0, i) == 1)
        PEX = _coupledVarVec[i];
      if (_T(1, i) == 1)
        PEY = _coupledVarVec[i];
    }
    std::pair<int, int> PEXRange = compTRange(0);
    std::pair<int, int> PEYRange = compTRange(1);
    DEBUG::check(_L.checkPEDimRange(PEXRange, 1), DEBUG::PEDIMOUTOFRANGE,
                 PEX->to_string());
    DEBUG::check(_L.checkPEDimRange(PEYRange, 0), DEBUG::PEDIMOUTOFRANGE,
                 PEY->to_string());

    DEBUG::check(!PEX->hasEdge(), DEBUG::EDGEPEDIMERROR, PEX->to_string());
    DEBUG::check(!PEY->hasEdge(), DEBUG::EDGEPEDIMERROR, PEY->to_string());
    _reuseVecMap[ARCH::INPUT] = _reuseVecI;
    _reuseVecMap[ARCH::WEIGHT] = _reuseVecW;
    _reuseVecMap[ARCH::OUTPUT] = _reuseVecO;
    DEBUG::check(_L.checkNetworkReuseValid(ARCH::INPUT, _reuseVecI),
                 DEBUG::REUSEVECNOTFITNETWORKARCHERROR, "Input");
    DEBUG::check(_L.checkNetworkReuseValid(ARCH::WEIGHT, _reuseVecW),
                 DEBUG::REUSEVECNOTFITNETWORKARCHERROR, "Weight");
    DEBUG::check(_L.checkNetworkReuseValid(ARCH::OUTPUT, _reuseVecO),
                 DEBUG::REUSEVECNOTFITNETWORKARCHERROR, "Output");
  }
  void setCurSubCoupledVarVec(std::vector<std::shared_ptr<WORKLOAD::Iterator>>
                                  curSubCoupledVarVec = {});
  void setBase(std::vector<Base> baseSet);
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> &getCoupledVarVec();
  void oneAnalysis();
  void compRequiredDataSize();
  std::shared_ptr<Result> getResult();
  int getOccTimes();
  void
  setSubLevelResultVec(std::vector<std::shared_ptr<Result>> &subLevelResultVec);
};