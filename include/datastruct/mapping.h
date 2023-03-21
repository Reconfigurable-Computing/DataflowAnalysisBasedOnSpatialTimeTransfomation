#pragma once
#include "include/datastruct/workload.h"
#include <iostream>
#include <memory>
#include <vector>

namespace MAPPING {
typedef double valueType;
class Transform {
private:
  std::shared_ptr<std::vector<valueType>> _transformMatrix;
  int _dimNum;

public:
  Transform(int dimNum, std::shared_ptr<std::vector<valueType>> transformMatrix)
      : _transformMatrix(transformMatrix), _dimNum(dimNum) {}
  int getDimNum() { return _dimNum; }
  std::shared_ptr<std::vector<valueType>> getMatrix() {
    return _transformMatrix;
  }
}; // end of Transform
class Access {
private:
  std::shared_ptr<std::vector<valueType>> _accessMatrix;
  int _colNum;

public:
  Access(int colNum, std::shared_ptr<std::vector<valueType>> accessMatrix)
      : _accessMatrix(accessMatrix), _colNum(colNum) {}
  int getColNum() { return _colNum; }
  std::shared_ptr<std::vector<valueType>> getMatrix() { return _accessMatrix; }
}; // end of Access
MAPPING::Access constructAccessMatrix(
    WORKLOAD::Tensor &tensor,
    std::vector<std::shared_ptr<WORKLOAD::Iterator>> &coupledVarVec);
} // namespace MAPPING