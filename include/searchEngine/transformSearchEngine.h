#pragma once
#include "include/analysis/multiLevelAnalysis.h"
#include "include/datastruct/arch.h"
#include "include/datastruct/mapping.h"
#include "include/datastruct/workload.h"
#include <algorithm>
#include <numeric>
class TransformSearchEngine {
private:
  WORKLOAD::Tensor &_I;
  WORKLOAD::Tensor &_W;
  WORKLOAD::Tensor &_O;
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> &_coupledVarVec;
  ARCH::Level &_L;
  int _spatialDimNum;
  int count;

public:
  TransformSearchEngine(
      WORKLOAD::Tensor &I, WORKLOAD::Tensor &W, WORKLOAD::Tensor &O,
      std::vector<std::shared_ptr<WORKLOAD::Iterator>> &coupledVarVec,
      ARCH::Level &L, int spatialDimNum)
      : _I(I), _W(W), _O(O), _coupledVarVec(coupledVarVec), _L(L),
        _spatialDimNum(spatialDimNum), count(0) {
    assert(_coupledVarVec.size() >= _spatialDimNum);
  }

  void setMatrixAsOne(MAPPING::Transform &T, std::vector<int> &permute,
                      int start);
  void startAnalysis(std::vector<MAPPING::Transform> &Tvec,
                     MultLevelAnalyzer &multanalysis);

  void generateTransformMatrix(std::vector<int> &permute,
                               std::vector<MAPPING::Transform> &Tvec);
  void generateAllTransformMatrix();
};