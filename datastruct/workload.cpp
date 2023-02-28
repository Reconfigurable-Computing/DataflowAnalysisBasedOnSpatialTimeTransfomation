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

public:
  Iterator() = default;
  Iterator(int lowBound, int upBound, std::string sym)
      : _lowBound(lowBound), _upBound(upBound), _sym(sym) {}
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
  int getLowBound() { return _lowBound; }
  int getUpBound() { return _upBound; }
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
}; // end of Monomial

std::shared_ptr<Monomial> operator*(std::shared_ptr<Iterator> var, int coef) {
  return std::make_shared<Monomial>(var, coef);
}
std::shared_ptr<Monomial> operator*(int coef, std::shared_ptr<Iterator> var) {
  return std::make_shared<Monomial>(var, coef);
}

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
}; // end of Polynomial
std::shared_ptr<Polynomial> operator+(std::shared_ptr<Monomial> var1,
                                      std::shared_ptr<Monomial> var2) {
  std::shared_ptr<Polynomial> ret = std::make_shared<Polynomial>(var1);
  *ret + var2;
  return ret;
}
std::shared_ptr<Polynomial> operator+(std::shared_ptr<Iterator> var1,
                                      std::shared_ptr<Iterator> var2) {
  std::shared_ptr<Polynomial> ret = std::make_shared<Polynomial>(var1);
  *ret + var2;
  return ret;
}
std::shared_ptr<Polynomial> operator+(std::shared_ptr<Monomial> var1,
                                      std::shared_ptr<Iterator> var2) {
  std::shared_ptr<Polynomial> ret = std::make_shared<Polynomial>(var1);
  *ret + var2;
  return ret;
}
std::shared_ptr<Polynomial> operator+(std::shared_ptr<Iterator> var1,
                                      std::shared_ptr<Monomial> var2) {
  std::shared_ptr<Polynomial> ret = std::make_shared<Polynomial>(var1);
  *ret + var2;
  return ret;
}

std::shared_ptr<Polynomial> operator+(std::shared_ptr<Polynomial> var1,
                                      std::shared_ptr<Iterator> var2) {

  *var1 + var2;
  return var1;
}
std::shared_ptr<Polynomial> operator+(std::shared_ptr<Polynomial> var1,
                                      std::shared_ptr<Monomial> var2) {

  *var1 + var2;
  return var1;
}
std::shared_ptr<Polynomial> operator+(std::shared_ptr<Polynomial> var1,
                                      std::shared_ptr<Polynomial> var2) {
  *var1 + var2;
  return var1;
}

class Tensor {
private:
  std::shared_ptr<std::vector<std::shared_ptr<Polynomial>>> _dimensionTable;
  std::string _sym;
  std::vector<std::pair<int, int>> _dimensionSize;

public:
  Tensor() {
    _dimensionTable =
        std::make_shared<std::vector<std::shared_ptr<Polynomial>>>();
  }
  Tensor(std::string sym) : _sym(sym) {
    _dimensionTable =
        std::make_shared<std::vector<std::shared_ptr<Polynomial>>>();
  }
  void compDimensionSize() {}
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
}; // end of Tensor
} // namespace WORKLOAD

// int main() {
//   std::shared_ptr<WORKLOAD::Iterator> k =
//       std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 3, {'k'}));
//   std::shared_ptr<WORKLOAD::Iterator> c =
//       std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 63, {'c'}));
//   std::shared_ptr<WORKLOAD::Iterator> y =
//       std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 31, {'y'}));
//   std::shared_ptr<WORKLOAD::Iterator> x =
//       std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 31, {'x'}));
//   std::shared_ptr<WORKLOAD::Iterator> p =
//       std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 3, {'p'}));
//   std::shared_ptr<WORKLOAD::Iterator> q =
//       std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 3, {'q'}));
//   WORKLOAD::Monomial tmp1 = k;
//   std::shared_ptr<WORKLOAD::Monomial> tmp2 = p * 4;
//   std::shared_ptr<WORKLOAD::Polynomial> tmp3 = k + p;

//   std::shared_ptr<WORKLOAD::Polynomial> tmp4 = 2 * k + p;

//   k->setBound(2, 3);
//   std::cout << tmp3->getRange().first << ' ' << tmp3->getRange().second
//             << std::endl;
//   std::cout << tmp4->getRange().first << ' ' << tmp4->getRange().second
//             << std::endl;
//   k->setBound(1, 2);
//   std::cout << tmp3->getRange().first << ' ' << tmp3->getRange().second
//             << std::endl;
//   std::cout << tmp4->getRange().first << ' ' << tmp4->getRange().second
//             << std::endl;
//   WORKLOAD::Tensor A("A");
//   A[p + x][4 * p + 3 * x];
//   std::cout << A.to_string() << std::endl;
//   return 0;
// }