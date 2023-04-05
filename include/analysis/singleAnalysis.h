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
           std::shared_ptr<std::vector<std::vector<int>>> reuseVecO);
  void oneAnalysis();
  std::pair<int, int> getTRange(int row);
  int getActivePENum();
  int getComputationDelay();
  int getPerPEVolumn(std::vector<int> &innerTimeVec);
  void getInnerUniqueAccess(ARCH::DATATYPE dataType, MAPPING::Access &access,
                            std::vector<int> &innerTimeVec, Result &result,
                            int outerTimeSize);
  void accessAnalysis(Result &result);
  bool checkValidInnerDim(int varIndex, ARCH::DATATYPE dataType);
  void constructSingleDataTypeSet(int flag, std::set<int> &singleDataTypeSet,
                                  ARCH::DATATYPE dataType);
  void getInnerOuterTimeVec(std::vector<int> &innerTimeVec,
                            std::vector<int> &outerTimeVec);
  int getTimeSize(std::vector<int> &timeVec);
  int getStableDelay(int compDelay);
  void compSubTensorSize();
  bool checkThasEdge(int row);
  void generateEdgeState(std::vector<std::vector<int>> &state,
                         std::vector<int> varIndexVec);
  int getOneStateTimeSize(std::vector<int> &timeVec);
  void oneStateAccessAnalysis(Result &result, std::vector<int> &innerTimeVec,
                              int outerTimeSize);
  void generateVarIndexVec(std::vector<int> &timeVec,
                           std::vector<int> &varIndexVec);
};