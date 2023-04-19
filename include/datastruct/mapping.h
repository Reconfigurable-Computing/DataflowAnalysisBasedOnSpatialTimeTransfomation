#pragma once
#include "include/datastruct/workload.h"
#include "include/util/debug.h"
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
  std::string to_string() {
    std::string ret;
    for (int i = 0; i < _colNum; i++) {
      for (int j = 0; j < _colNum; j++) {
        ret += std::to_string((*this)(i, j)) + ' ';
      }
      ret += '\n';
    }
    return ret;
  }
};
class Transform : public Matrix2D {
private:
public:
  Transform(int dimNum,
            std::shared_ptr<std::vector<mappingValueType>> transformMatrix)
      : Matrix2D(dimNum, transformMatrix) {
    DEBUG::checkError(check(), DEBUG::TMATRIXERROR, to_string());
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
    int PEX, PEY;
    for (int j = 0; j < _colNum; j++) {

      if (operator()(0, j) == 1 && operator()(1, j) == 1)
        return 0;
      else if (operator()(0, j) == 1)
        PEX = j;
      else if (operator()(1, j) == 1)
        PEY = j;
    }
    std::vector<int> rowOnesCount(_colNum, 0);
    for (int i = 0; i < _colNum; i++) {
      for (int j = 0; j < _colNum; j++) {
        if (operator()(i, j) != 1 && operator()(i, j) != 0)
          return 0;
        else if (operator()(i, j) == 1) {
          rowOnesCount[i]++;
        }
      }
    }
    if (rowOnesCount[0] > 1 || rowOnesCount[1] > 1)
      return 0;
    int count = 0;
    int index;
    for (int i = 2; i < _colNum; i++) {
      if (rowOnesCount[i] > 1) {
        count++;
        if (count > 1)
          return 0;
        index = i;
      } else if (rowOnesCount[i] == 0)
        return 0;
    }
    count = 0;
    for (int j = 2; j < _colNum; j++) {
      if (operator()(index, j) != 1) {
        count++;
        if (count > 1)
          return 0;
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
      : Matrix2D(colNum, accessMatrix) {}
}; // end of Access
MAPPING::Access constructAccessMatrix(
    WORKLOAD::Tensor &tensor,
    std::vector<std::shared_ptr<WORKLOAD::Iterator>> &coupledVarVec);
} // namespace MAPPING