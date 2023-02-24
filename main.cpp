#include "datastruct/workload.cpp"
#include <Eigen/Dense>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
// using Eigen::MatrixXd;
using namespace Eigen;
using namespace Eigen::internal;
using namespace Eigen::Architecture;

int main() {
  std::shared_ptr<WORKLOAD::Iterator> k =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 3, {'k'}));
  std::shared_ptr<WORKLOAD::Iterator> c =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 63, {'c'}));
  std::shared_ptr<WORKLOAD::Iterator> y =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 31, {'y'}));
  std::shared_ptr<WORKLOAD::Iterator> x =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 31, {'x'}));
  std::shared_ptr<WORKLOAD::Iterator> p =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 3, {'p'}));
  std::shared_ptr<WORKLOAD::Iterator> q =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 3, {'q'}));
  WORKLOAD::Monomial tmp1 = k;
  std::shared_ptr<WORKLOAD::Monomial> tmp2 = p * 4;
  std::shared_ptr<WORKLOAD::Polynomial> tmp3 = k + p;

  std::shared_ptr<WORKLOAD::Polynomial> tmp4 = 2 * k + p;

  k->setBound(2, 3);
  std::cout << tmp3->getRange().first << ' ' << tmp3->getRange().second
            << std::endl;
  std::cout << tmp4->getRange().first << ' ' << tmp4->getRange().second
            << std::endl;
  k->setBound(1, 2);
  std::cout << tmp3->getRange().first << ' ' << tmp3->getRange().second
            << std::endl;
  std::cout << tmp4->getRange().first << ' ' << tmp4->getRange().second
            << std::endl;
  WORKLOAD::Tensor A("A");
  A[p + x][4 * p + 3 * x];
  std::cout << A.to_string() << std::endl;
  return 0;
}