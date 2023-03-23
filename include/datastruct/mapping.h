#pragma once
#include "include/datastruct/workload.h"
#include <assert.h>
#include <iostream>
#include <memory>
#include <vector>
namespace MAPPING {
typedef int mappingValueType;
class Matrix2D {
protected:
  std::shared_ptr<std::vector<mappingValueType>> _value;
  int _colNum;

public:
  Matrix2D(int colNum, std::shared_ptr<std::vector<mappingValueType>> value)
      : _value(value), _colNum(colNum) {}
  bool check() {
    if (_value->size() % _colNum != 0)
      return 0;
    else
      return 1;
  }
  mappingValueType operator()(int i, int j) {
    return (*_value)[i * _colNum + j];
  }
  int getColNum() { return _colNum; }
  int getRowNum() { return _value->size() / _colNum; }
  std::shared_ptr<std::vector<mappingValueType>> getMatrix() { return _value; }
};
class Transform : public Matrix2D {
private:
public:
  Transform(int dimNum,
            std::shared_ptr<std::vector<mappingValueType>> transformMatrix)
      : Matrix2D(dimNum, transformMatrix) {
    assert(check());
  }
  bool check() {
    if (_value->size() != _colNum * _colNum)
      return 0;
    for (int j = 0; j < _colNum; j++) {
      if ((*_value)[j] == 1 && (*_value)[j + _colNum] == 1)
        return 0;
    }
    int countSpatialOne;
    int countSpatialAndTemporalOne;
    for (int i = 0; i < 2; i++) {
      countSpatialOne = 0;
      countSpatialAndTemporalOne = 0;
      for (int k = 0; k < _colNum; k++) {
        if ((*_value)[_colNum * i + k] == 1)
          countSpatialOne++;
      }
      if (countSpatialOne > 1) {
        for (int j = 2; j < _colNum; j++) {
          for (int k = 0; k < _colNum; k++) {
            if ((*_value)[_colNum * i + k] == 1 &&
                (*_value)[_colNum * j + k] == 1)
              return 0;
          }
        }
      }
    }
    return 1;
  }

}; // end of Transform
class Access : public Matrix2D {
private:
public:
  Access(int colNum,
         std::shared_ptr<std::vector<mappingValueType>> accessMatrix)
      : Matrix2D(colNum, accessMatrix) {
    assert(check());
  }
}; // end of Access
MAPPING::Access constructAccessMatrix(
    WORKLOAD::Tensor &tensor,
    std::vector<std::shared_ptr<WORKLOAD::Iterator>> &coupledVarVec);
} // namespace MAPPING