#include "include/datastruct/mapping.h"
#include "include/datastruct/workload.h"
typedef double valueType;
namespace MAPPING {
MAPPING::Access constructAccessMatrix(
    WORKLOAD::Tensor &tensor,
    std::vector<std::shared_ptr<WORKLOAD::Iterator>> &coupledVarVec) {
  const int varNum = coupledVarVec.size();
  std::shared_ptr<std::vector<valueType>> tmp;
  const int dimNum = tensor.getDimensionNum();
  tmp = std::make_shared<std::vector<valueType>>();
  for (int dimIndex = 0; dimIndex < dimNum; dimIndex++) {
    if (tensor.checkDimCoupled(dimIndex)) {
      for (int varIndex = 0; varIndex < varNum; varIndex++) {
        tmp->push_back(tensor.lookupVar(coupledVarVec[varIndex], dimIndex));
      }
    }
  }
  return MAPPING::Access(varNum, tmp);
}
} // namespace MAPPING
