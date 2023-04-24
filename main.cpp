
#include "include/analysis/multiLevelAnalysis.h"
#include "include/analysis/singleLevelAnalysis.h"
#include "include/datastruct/arch.h"
#include "include/datastruct/mapping.h"
#include "include/datastruct/workload.h"
#include "include/util/debug.h"
#include "include/util/eigenUtil.h"
#include "include/util/timeline.h"
#include <assert.h>
#include <iostream>
#include <map>
#include <math.h>
#include <memory>
#include <set>
#include <string>
#include <vector>

int main() {

  std::shared_ptr<WORKLOAD::Iterator> k =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 3, "k"));
  std::shared_ptr<WORKLOAD::Iterator> c =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 10, "c"));
  std::shared_ptr<WORKLOAD::Iterator> c2 = std::make_shared<WORKLOAD::Iterator>(
      WORKLOAD::Iterator(0, 3, 0, 2, "c2"));
  std::shared_ptr<WORKLOAD::Iterator> c1 =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 1, c2, "c1"));
  std::shared_ptr<WORKLOAD::Iterator> y =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 10, "y"));
  std::shared_ptr<WORKLOAD::Iterator> y2 = std::make_shared<WORKLOAD::Iterator>(
      WORKLOAD::Iterator(0, 5, 0, 4, "y2"));
  std::shared_ptr<WORKLOAD::Iterator> y1 =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 5, y2, "y1"));
  std::shared_ptr<WORKLOAD::Iterator> x =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 2, "x"));
  std::shared_ptr<WORKLOAD::Iterator> p =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 3, "p"));
  std::shared_ptr<WORKLOAD::Iterator> q =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 3, "q"));
  WORKLOAD::Tensor I("I");
  WORKLOAD::Tensor W("W");
  WORKLOAD::Tensor O("O");

  // I[c][y + p][x + q];
  // W[k][c][p][q];
  // O[k][y][x];
  I[c1 * 4 + c2][y + p][x + q];
  W[k][c1 * 4 + c2][p][q];
  O[k][y][x];

  std::vector<std::shared_ptr<WORKLOAD::Iterator>> coupledVarVec1;
  coupledVarVec1.push_back(k);
  coupledVarVec1.push_back(x);
  coupledVarVec1.push_back(c2);

  MAPPING::Transform T1(3, std::make_shared<std::vector<int>>(
                               std::vector<int>{1, 0, 0, 0, 1, 0, 0, 1, 1}));
  ARCH::Level L1(3, 4, 16);
  L1.appendBuffer(ARCH::REG, ARCH::INPUT, 128);
  L1.appendBuffer(ARCH::REG, ARCH::WEIGHT, 128);
  L1.appendBuffer(ARCH::REG, ARCH::OUTPUT, 128);
  L1.appendNetworkGroup(ARCH::INPUT, 16, {1, 0, 0});
  L1.appendNetworkGroup(ARCH::WEIGHT, 16, {0, 1, 1});
  L1.appendNetworkGroup(ARCH::OUTPUT, 16, {0, 0, 1});

  std::vector<std::shared_ptr<WORKLOAD::Iterator>> coupledVarVec2;
  coupledVarVec2.push_back(y);
  coupledVarVec2.push_back(p);
  coupledVarVec2.push_back(q);
  coupledVarVec2.push_back(c1);

  MAPPING::Transform T2(
      4, std::make_shared<std::vector<int>>(
             std::vector<int>{1, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1}));
  ARCH::Level L2(32, 16, 16);
  L2.appendBuffer(ARCH::REG, ARCH::INPUT, 128);
  L2.appendBuffer(ARCH::REG, ARCH::WEIGHT, 128);
  L2.appendBuffer(ARCH::REG, ARCH::OUTPUT, 128);
  L2.appendNetworkGroup(ARCH::INPUT, 16, {-1, 1, 0});
  L2.appendNetworkGroup(ARCH::WEIGHT, 16, {1, 0, 1});
  L2.appendNetworkGroup(ARCH::OUTPUT, 16, {0, 1, 0}, {0, 0, 1});

  std::cout << I.getRange(1).first << ' ' << I.getRange(1).second << std::endl;

  MultLevelAnalyzer multanalysis(I, W, O);
  multanalysis.addLevel(coupledVarVec1, T1, L1);
  multanalysis.addLevel(coupledVarVec2, T2, L2);
  multanalysis.oneAnalysis();
  return 0;
}