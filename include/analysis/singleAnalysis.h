#pragma once

#include "include/datastruct/arch.h"
#include "include/datastruct/mapping.h"
#include "include/datastruct/workload.h"
#include <set>
#include <vector>
struct Result {
  int uniqueVolumn[3]; // input weight output
  int totalVolumn[3];
  int reuseVolumn[3];
  int baseCompCycle;
  int baseData[3];
  int delay;
  Result() {
    for (int i = 0; i < 3; i++) {
      uniqueVolumn[i] = 0;
      totalVolumn[i] = 0;
      reuseVolumn[i] = 0;
      baseData[i] = 0;
    }
    baseCompCycle = 0;
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
           MAPPING::Transform &T, int baseCompCycle, int baseData[3],
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
    _result.baseCompCycle = baseCompCycle;
    for (int i = 0; i < 3; i++) {
      _result.baseData[i] = baseData[i];
    }
    _reuseVecMap[ARCH::INPUT] = _reuseVecI;
    _reuseVecMap[ARCH::WEIGHT] = _reuseVecW;
    _reuseVecMap[ARCH::OUTPUT] = _reuseVecO;

    int coupleVarNum = _coupledVarVec.size();
  }
  void oneAnalysis();
  std::pair<int, int> getTRange(int row);
  int getActivePENum();
  int getComputationDelay();
  int getPerPEVolumn(std::vector<int> &innerTimeVec);
  void getInnerUniqueAccess(ARCH::DATATYPE dataType, MAPPING::Access &access,
                            std::vector<int> &innerTimeVec, int outerTimeSize);
  void accessAnalysis();
  bool checkValidInnerDim(int varIndex, ARCH::DATATYPE dataType);
  void constructSingleDataTypeSet(int flag, std::set<int> &singleDataTypeSet,
                                  ARCH::DATATYPE dataType);
  void getInnerOuterTimeVec(std::vector<int> &innerTimeVec,
                            std::vector<int> &outerTimeVec);
  int getTimeSize(std::vector<int> &timeVec);
  void getStableDelay();
  int getOneStateStableDelay(std::vector<int> innerTimeVec);
  void generateEdgeState(std::vector<std::vector<int>> &state,
                         std::vector<int> varIndexVec);
  int getOneStateTimeSize(std::vector<int> &timeVec);
  void oneStateAccessAnalysis(std::vector<int> &innerTimeVec,
                              int outerTimeSize);
  void generateVarIndexVec(std::vector<int> &timeVec,
                           std::vector<int> &varIndexVec);
};