#pragma once

#include "include/datastruct/arch.h"
#include "include/datastruct/mapping.h"
#include "include/datastruct/workload.h"
#include "include/util/eigenUtil.h"
#include <set>
#include <vector>
struct Result {
  int uniqueVolumn[3]; // input weight output
  int totalVolumn[3];
  int reuseVolumn[3];
  int requiredBufferSize[3];
  int baseCompCycle;
  int baseData[3];
  int delay;
  Result() { reset(); }
  void reset() {
    for (int i = 0; i < 3; i++) {
      uniqueVolumn[i] = 0;
      totalVolumn[i] = 0;
      reuseVolumn[i] = 0;
      baseData[i] = 0;
      requiredBufferSize[i] = 0;
    }
    baseCompCycle = 0;
    delay = 0;
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
  Result _result;
  std::shared_ptr<std::vector<std::vector<int>>> _reuseVecI;
  std::shared_ptr<std::vector<std::vector<int>>> _reuseVecW;
  std::shared_ptr<std::vector<std::vector<int>>> _reuseVecO;
  std::map<ARCH::DATATYPE, std::shared_ptr<std::vector<std::vector<int>>>>
      _reuseVecMap;
  std::shared_ptr<WORKLOAD::Iterator> PEX;
  std::shared_ptr<WORKLOAD::Iterator> PEY;
  bool _doubleBufferFlag;
  bool _lowestFlag;
  std::pair<int, int> compTRange(int row);
  bool checkValidInnerDim(int varIndex, ARCH::DATATYPE dataType);
  void constructSingleDataTypeTimeSet(int flag,
                                      std::set<int> &singleDataTypeSet,
                                      ARCH::DATATYPE dataType);
  void constructInnerOuterTimeVec(std::vector<int> &innerTimeVec,
                                  std::vector<int> &outerTimeVec);
  int compTimeSize(std::vector<int> &timeVec);
  int compOneStateTimeSize(std::vector<int> &timeVec);
  void generateVarIndexVec(std::vector<int> &timeVec,
                           std::vector<int> &varIndexVec);
  void generateEdgeState(std::vector<std::vector<int>> &state,
                         std::vector<int> varIndexVec);

  int compPerPEVolumn(std::vector<int> &innerTimeVec);
  void compInnerUniqueAccess(ARCH::DATATYPE dataType, MAPPING::Access &access,
                             std::vector<int> &innerTimeVec, int outerTimeSize);
  void oneStateAccessAnalysis(std::vector<int> &innerTimeVec,
                              int outerTimeSize);
  void accessAnalysis();
  int compOneStateStableDelay(std::vector<int> innerTimeVec);
  void delayAnalysis();

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

    _reuseVecMap[ARCH::INPUT] = _reuseVecI;
    _reuseVecMap[ARCH::WEIGHT] = _reuseVecW;
    _reuseVecMap[ARCH::OUTPUT] = _reuseVecO;

    _L.checkNetworkReuseValid(ARCH::INPUT, _reuseVecI);
    _L.checkNetworkReuseValid(ARCH::WEIGHT, _reuseVecW);
    _L.checkNetworkReuseValid(ARCH::OUTPUT, _reuseVecO);
  }
  void setBase(int baseCompCycle, int baseData[3]) {
    _result.baseCompCycle = baseCompCycle;
    for (int i = 0; i < 3; i++) {
      _result.baseData[i] = baseData[i];
    }
  }

  std::vector<std::shared_ptr<WORKLOAD::Iterator>> &getCoupledVarVec() {
    return _coupledVarVec;
  }
  int getActivePENum();
  int getComputationDelay();
  void oneAnalysis();
  void compRequiredBufferSize() {
    _result.requiredBufferSize[ARCH::OUTPUT] = _O.getVolumn();
    _result.requiredBufferSize[ARCH::INPUT] = _I.getVolumn();
    _result.requiredBufferSize[ARCH::WEIGHT] = _W.getVolumn();
  }
  Result &getResult() { return _result; }
};