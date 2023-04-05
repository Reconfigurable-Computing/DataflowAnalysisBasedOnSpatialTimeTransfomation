#pragma once

#include <assert.h>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
namespace WORKLOAD {
class Iterator {
private:
  int _lowBound;
  int _upBound;
  std::string _sym;
  bool _lock;
  bool _hasEdge;
  bool _edgeFlag;
  bool _isEdgeChild;
  int _edgeLowBound;
  int _edgeUpBound;
  int _cur;
  std::shared_ptr<Iterator> _coupledIterator;

public:
  Iterator() = default;
  Iterator(int lowBound, int upBound, std::string sym)
      : _lowBound(lowBound), _upBound(upBound), _sym(sym), _lock(false),
        _hasEdge(false), _edgeFlag(false), _edgeLowBound(0), _edgeUpBound(0),
        _cur(lowBound), _isEdgeChild(false) {}
  Iterator(int lowBound, int upBound, int edgeLowBound, int EdgeUpBound,
           std::shared_ptr<Iterator> coupledIterator, std::string sym)
      : _lowBound(lowBound), _upBound(upBound), _sym(sym), _lock(false),
        _hasEdge(true), _edgeFlag(false), _isEdgeChild(false),
        _edgeLowBound(edgeLowBound), _edgeUpBound(EdgeUpBound),
        _coupledIterator(coupledIterator), _cur(lowBound) {
    assert(!_isEdgeChild);
    _coupledIterator->setIsEdgeChild();
  }
  void setIsEdgeChild() { _isEdgeChild = true; }
  std::string to_string() {
    std::string ret;
    ret += "var:\t" + _sym + "\tlow\t" + std::to_string(_lowBound) + "\tup\t" +
           std::to_string(_upBound);
    if (_hasEdge) {
      ret += "\tedgeLow\t" + std::to_string(_edgeLowBound) + "\tedgeUp\t" +
             std::to_string(_edgeUpBound);
    }
    return ret;
  }

  void setBound(int lowBound, int upBound) {
    _lowBound = lowBound;
    _upBound = upBound;
  }

  std::string &getSym() { return _sym; }
  int getLowBound() {
    return _lock ? 0 : (_edgeFlag ? _upBound + 1 : _lowBound);
  }
  int getUpBound() { return _lock ? 0 : (_edgeFlag ? _upBound + 1 : _upBound); }
  void lock() { _lock = true; }
  void unlock() { _lock = false; }
  int getSize() {
    return _lock ? 0 : (_edgeFlag ? 1 : (_upBound - _lowBound + 1));
  }
  void edgeSwap() {
    int tmpLowBound = _coupledIterator->getLowBound();
    int tmpUpBound = _coupledIterator->getUpBound();
    _coupledIterator->setBound(_edgeLowBound, _edgeUpBound);
    _edgeLowBound = tmpLowBound;
    _edgeUpBound = tmpUpBound;
  }
  void setEdge() {
    _edgeFlag = true;
    edgeSwap();
  }
  void unsetEdge() {
    _edgeFlag = false;
    edgeSwap();
  }
  bool hasEdge() { return _hasEdge; }
  void reset() { _cur = 0; }
  void getNext() {
    if (isTop()) {
      _cur = _lowBound;
      if (_hasEdge) {
        unsetEdge();
      }
    } else {
      _cur++;
      if (_hasEdge && isTop()) {
        setEdge();
      }
    }
  }
  bool isTop() {
    if (_hasEdge)
      return _cur == _upBound + 1;
    else
      return _cur == _upBound;
  }
  int getCur() { return _cur; }

}; // end of Iterator

class Monomial {
private:
  std::shared_ptr<Iterator> _var;
  int _coef;

public:
  Monomial() = default;
  Monomial(std::shared_ptr<Iterator> var) : _var(var), _coef(1) {}
  Monomial(std::shared_ptr<Iterator> var, int coef) : _var(var), _coef(coef) {}
  std::string to_string() {
    std::string ret;
    ret += std::to_string(_coef) + " * " + _var->getSym();
    return ret;
  }
  std::pair<int, int> getRange() {
    std::pair<int, int> ret;
    if (_coef > 0) {
      ret.first = _coef * _var->getLowBound();
      ret.second = _coef * _var->getUpBound();
    } else {
      ret.second = _coef * _var->getLowBound();
      ret.first = _coef * _var->getUpBound();
    }
    return ret;
  }
  std::shared_ptr<Iterator> getVar() { return _var; }

  int getCoef() { return _coef; }
  int getCur() { return _coef * _var->getCur(); }
}; // end of Monomial

std::shared_ptr<Monomial> operator*(std::shared_ptr<Iterator> var, int coef);
std::shared_ptr<Monomial> operator*(int coef, std::shared_ptr<Iterator> var);

class Polynomial {
private:
  std::shared_ptr<std::vector<std::shared_ptr<Monomial>>> _monomialSet;

public:
  Polynomial() {
    _monomialSet = std::make_shared<std::vector<std::shared_ptr<Monomial>>>();
  }
  Polynomial(std::shared_ptr<Monomial> m) : Polynomial() {

    _monomialSet->push_back(m);
  }
  Polynomial(std::shared_ptr<Iterator> i) : Polynomial() {

    _monomialSet->push_back(std::make_shared<Monomial>(i));
  }
  Polynomial &operator+(std::shared_ptr<Polynomial> other) {
    for (auto item : *(other->_monomialSet)) {
      _monomialSet->push_back(item);
    }
    return *this;
  }
  Polynomial &operator+(std::shared_ptr<Monomial> other) {
    _monomialSet->push_back(other);
    return *this;
  }
  Polynomial &operator+(std::shared_ptr<Iterator> other) {
    _monomialSet->push_back(std::make_shared<Monomial>(other));
    return *this;
  }
  std::string to_string() {
    std::string ret;
    const int len = _monomialSet->size();
    for (int i = 0; i < len; i++) {
      ret += (*_monomialSet)[i]->to_string();
      if (i != len - 1)
        ret += " + ";
    }
    return ret;
  }
  std::pair<int, int> getRange() {
    std::pair<int, int> ret = {0, 0}, tmp;

    for (auto m : *_monomialSet) {
      tmp = m->getRange();
      ret.first += tmp.first;
      ret.second += tmp.second;
    }
    return ret;
  }
  int getSize() {
    std::pair<int, int> tmp = getRange();
    return tmp.second - tmp.first + 1;
  }

  int lookupVar(std::shared_ptr<Iterator> i) {
    for (auto m : *_monomialSet) {
      if (m->getVar() == i)
        return m->getCoef();
    }
    return 0;
  }
  int getCur() {
    int ret = 0;
    for (auto m : *_monomialSet) {
      ret += m->getCur();
    }
    return ret;
  }
}; // end of Polynomial
std::shared_ptr<Polynomial> operator+(std::shared_ptr<Monomial> var1,
                                      std::shared_ptr<Monomial> var2);
std::shared_ptr<Polynomial> operator+(std::shared_ptr<Iterator> var1,
                                      std::shared_ptr<Iterator> var2);
std::shared_ptr<Polynomial> operator+(std::shared_ptr<Monomial> var1,
                                      std::shared_ptr<Iterator> var2);
std::shared_ptr<Polynomial> operator+(std::shared_ptr<Iterator> var1,
                                      std::shared_ptr<Monomial> var2);

std::shared_ptr<Polynomial> operator+(std::shared_ptr<Polynomial> var1,
                                      std::shared_ptr<Iterator> var2);
std::shared_ptr<Polynomial> operator+(std::shared_ptr<Polynomial> var1,
                                      std::shared_ptr<Monomial> var2);
std::shared_ptr<Polynomial> operator+(std::shared_ptr<Polynomial> var1,
                                      std::shared_ptr<Polynomial> var2);

class Tensor {
private:
  std::shared_ptr<std::vector<std::shared_ptr<Polynomial>>> _dimensionTable;
  std::string _sym;
  std::shared_ptr<std::vector<int>> _coupled;

public:
  Tensor() {
    _dimensionTable =
        std::make_shared<std::vector<std::shared_ptr<Polynomial>>>();
  }
  Tensor(std::string sym) : _sym(sym) {
    _dimensionTable =
        std::make_shared<std::vector<std::shared_ptr<Polynomial>>>();
  }
  int getDimensionNum() { return _dimensionTable->size(); }
  int lookupVar(std::shared_ptr<Iterator> i, int dim) {
    return (*_dimensionTable)[dim]->lookupVar(i);
  }
  Tensor &operator[](std::shared_ptr<Polynomial> p) {
    _dimensionTable->push_back(p);
    return *this;
  }
  Tensor &operator[](std::shared_ptr<Iterator> i) {
    _dimensionTable->push_back(std::make_shared<Polynomial>(i));
    return *this;
  }
  Tensor &operator[](std::shared_ptr<Monomial> m) {
    _dimensionTable->push_back(std::make_shared<Polynomial>(m));
    return *this;
  }
  std::string to_string() {
    std::string ret;
    ret += _sym;
    for (auto item : *_dimensionTable) {
      ret += "[" + item->to_string() + "]";
    }
    return ret;
  }
  std::pair<int, int> getRange(int dimIndex) {
    return (*_dimensionTable)[dimIndex]->getRange();
  }
  void
  bindVar(std::vector<std::shared_ptr<WORKLOAD::Iterator>> &coupledVarVec) {
    const int dimNum = getDimensionNum();
    const int varNum = coupledVarVec.size();
    _coupled = std::make_shared<std::vector<int>>(dimNum, 0);
    for (int dimindex = 0; dimindex < dimNum; dimindex++) {
      for (int varIndex = 0; varIndex < varNum; varIndex++) {
        if (lookupVar(coupledVarVec[varIndex], dimindex)) {
          (*_coupled)[dimindex] = 1;
        }
      }
    }
  }

  bool checkDimCoupled(int dimIndex) { return (*_coupled)[dimIndex]; }
  std::vector<int> getCur() {
    std::vector<int> ret;
    for (auto p : *_dimensionTable) {
      ret.push_back(p->getCur());
    }
    return ret;
  }
}; // end of Tensor
} // namespace WORKLOAD