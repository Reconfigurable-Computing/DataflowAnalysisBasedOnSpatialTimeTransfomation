#pragma once

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
  int _lock;

public:
  Iterator() = default;
  Iterator(int lowBound, int upBound, std::string sym)
      : _lowBound(lowBound), _upBound(upBound), _sym(sym), _lock(0) {}
  std::string to_string() {
    std::string ret;
    ret += "var:\t" + _sym + "\tlow\t" + std::to_string(_lowBound) + "\tup\t" +
           std::to_string(_upBound);
    return ret;
  }
  void setBound(int lowBound, int upBound) {
    _lowBound = lowBound;
    _upBound = upBound;
  }

  std::string &getSym() { return _sym; }
  int getLowBound() { return _lock ? 0 : _lowBound; }
  int getUpBound() { return _lock ? 0 : _upBound; }
  void lock() { _lock = 1; }
  void unlock() { _lock = 0; }
  int getRange() { return _lock ? 0 : _upBound - _lowBound + 1; }
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

  int lookupVar(std::shared_ptr<Iterator> i) {
    for (auto m : *_monomialSet) {
      if (m->getVar() == i)
        return m->getCoef();
    }
    return 0;
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
}; // end of Tensor
} // namespace WORKLOAD
