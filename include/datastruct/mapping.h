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
  void print() {
    for (int i = 0; i < _colNum; i++) {
      for (int j = 0; j < _colNum; j++) {
        std::cout << (*this)(i, j) << ' ';
      }
      std::cout << std::endl;
    }
  }
};
class Transform : public Matrix2D {
private:
public:
  Transform(int dimNum,
            std::shared_ptr<std::vector<mappingValueType>> transformMatrix)
      : Matrix2D(dimNum, transformMatrix) {
    assert(check());
  }
  void addExtraSpatial() {
    _colNum++;
    int addElementNum = _colNum * _colNum - (_colNum - 1) * (_colNum - 1);
    for (int i = 0; i < addElementNum; i++) {
      _value->push_back(0);
    }
    for (int i = _colNum - 2; i >= 0; i--) {
      for (int j = _colNum - 2; j >= 0; j--) {

        (*_value)[(i + 1) * _colNum + 1 + j] = (*_value)[i * (_colNum - 1) + j];
      }
    }
    (*_value)[0] = 1;
    for (int i = 1; i < _colNum; i++) {
      (*_value)[i] = 0;
      (*_value)[i * _colNum] = 0;
    }
  }
  bool check() {
    if (_value->size() != _colNum * _colNum)
      return 0;
    for (int j = 0; j < _colNum; j++) {
      if ((*_value)[j] == 1 && (*_value)[j + _colNum] == 1)
        return 0;
    }
    int count;
    for (int i = 0; i < 2; i++) {
      count = 0;
      for (int j = 0; j < _colNum; j++) {
        if ((*_value)[j + i * _colNum] == 1) {
          count++;
        }
      }
      if (count > 1)
        return 0;
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