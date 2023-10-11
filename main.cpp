
#include "include/analysis/multiLevelAnalysis.h"
#include "include/analysis/singleLevelAnalysis.h"
#include "include/datastruct/arch.h"
#include "include/datastruct/mapping.h"
#include "include/datastruct/workload.h"
#include "include/searchEngine/transformSearchEngine.h"
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
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 2, "c"));
  std::shared_ptr<WORKLOAD::Iterator> y =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 223, "y"));
  std::shared_ptr<WORKLOAD::Iterator> x =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 223, "x"));
  std::shared_ptr<WORKLOAD::Iterator> p =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 2, "p"));
  std::shared_ptr<WORKLOAD::Iterator> q =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 2, "q"));
  WORKLOAD::Tensor I("I");
  WORKLOAD::Tensor W("W");
  WORKLOAD::Tensor O("O");

  // I[c][y + p][x + q];
  // W[k][c][p][q];
  // O[k][y][x];
  I[c][y + p][x + q];
  W[k][c][p][q];
  O[k][y][x];

  std::vector<std::shared_ptr<WORKLOAD::Iterator>> coupledVarVec1;
  coupledVarVec1.push_back(k);
  coupledVarVec1.push_back(c);
  coupledVarVec1.push_back(x);
  coupledVarVec1.push_back(y);
  coupledVarVec1.push_back(p);
  coupledVarVec1.push_back(q);

  MAPPING::Transform T1(
      coupledVarVec1.size(),
      std::make_shared<std::vector<int>>(std::vector<int>{
          1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0,
          0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1}));
  ARCH::Level L1(3, 16, 16);
  L1.appendBuffer(ARCH::REG, ARCH::INPUT, 128);
  L1.appendBuffer(ARCH::REG, ARCH::WEIGHT, 128);
  L1.appendBuffer(ARCH::REG, ARCH::OUTPUT, 128);
  L1.appendNetworkGroup(ARCH::INPUT, 16, {1, 0, 1});
  L1.appendNetworkGroup(ARCH::WEIGHT, 16, {0, 0, 1});
  L1.appendNetworkGroup(ARCH::OUTPUT, 16, {0, 1, 1});

  TransformSearchEngine transformSearchEngine(I, W, O, coupledVarVec1, L1, 2);
  transformSearchEngine.generateAllTransformMatrix();
  //   MultLevelAnalyzer multanalysis(I, W, O);
  //   multanalysis.addLevel(coupledVarVec1, T1, L1);
  //   std::cout << multanalysis.constraintCheck() << std::endl;
  //   multanalysis.oneAnalysis();
  return 0;
}
