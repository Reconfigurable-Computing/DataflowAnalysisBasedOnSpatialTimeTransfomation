
#include "Eigen/Core"
#include "Eigen/Dense"
#include "include/datastruct/arch.h"
#include "include/datastruct/mapping.h"
#include "include/datastruct/workload.h"
#include "include/util/eigenUtil.h"
#include <assert.h>
#include <iostream>
#include <map>
#include <math.h>
#include <memory>
#include <string>
#include <vector>
// using Eigen::MatrixXd;
using namespace Eigen;
using namespace Eigen::internal;
using namespace Eigen::Architecture;

int main() {

  MAPPING::Transform T(3,
                       std::make_shared<std::vector<valueType>>(
                           std::vector<valueType>{1, 0, 0, 0, 1, 0, 1, 1, 1}));
  ARCH::Level L1;
  L1.appendArray(3, 4, 16);
  L1.appendBuffer(ARCH::REG, ARCH::INPUT, 128, 16);
  L1.appendBuffer(ARCH::REG, ARCH::WEIGHT, 128, 16);
  L1.appendBuffer(ARCH::REG, ARCH::OUTPUT, 128, 16);
  L1.appendNetworkGroup(3, 4, ARCH::INPUT, {1, 0, 0});
  L1.appendNetworkGroup(3, 4, ARCH::WEIGHT, {0, 1, 0});
  L1.appendNetworkGroup(3, 4, ARCH::OUTPUT, {0, 0, 1});

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
  WORKLOAD::Tensor I("I");
  WORKLOAD::Tensor W("W");
  WORKLOAD::Tensor O("O");
  I[c][p][x + q];
  W[k][c][p][q];
  O[k][y][x];

  std::cout << I.getRange(2).first << ' ' << I.getRange(2).second << std::endl;
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> coupledVarVec;
  coupledVarVec.push_back(k);
  coupledVarVec.push_back(c);
  coupledVarVec.push_back(x);
  y->lock();
  p->lock();
  q->lock();
  std::cout << I.getRange(2).first << ' ' << I.getRange(2).second << std::endl;
  I.bindVar(coupledVarVec);
  W.bindVar(coupledVarVec);
  O.bindVar(coupledVarVec);

  MAPPING::Access accessI = MAPPING::constructAccessMatrix(I, coupledVarVec);
  MAPPING::Access accessW = MAPPING::constructAccessMatrix(W, coupledVarVec);
  MAPPING::Access accessO = MAPPING::constructAccessMatrix(O, coupledVarVec);
  std::shared_ptr<std::vector<std::vector<int>>> ret = compReuseVec(T, accessI);

  return 0;
}