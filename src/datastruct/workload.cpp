#include "include/datastruct/workload.h"

namespace WORKLOAD {
std::shared_ptr<Monomial> operator*(std::shared_ptr<Iterator> var, int coef) {
  return std::make_shared<Monomial>(var, coef);
}
std::shared_ptr<Monomial> operator*(int coef, std::shared_ptr<Iterator> var) {
  return std::make_shared<Monomial>(var, coef);
}
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
} // namespace WORKLOAD