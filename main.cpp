
#include "include/analysis/singleAnalysis.h"
#include "include/datastruct/arch.h"
#include "include/datastruct/mapping.h"
#include "include/datastruct/workload.h"
#include "include/util/debug.h"
#include "include/util/eigenUtil.h"
#include <assert.h>
#include <iostream>
#include <map>
#include <math.h>
#include <memory>
#include <set>
#include <string>
#include <vector>

int main() {

  // rank
  // c1 c2   c1 outer than c2
  // pedim only one
  // MAPPING::Transform T(4, std::make_shared<std::vector<int>>(
  //    std::vector<int>{1,0,0,0
  //    ,0,1,0,0
  //    ,1,1,1,0
  //    ,0,0,0,1}));
  // MAPPING::Transform T(3, std::make_shared<std::vector<int>>(
  //    std::vector<int>{1, 0, 0
  //    , 0, 1, 0
  //    , 1, 1, 1}));
  // MAPPING::Transform T(6, std::make_shared<std::vector<int>>(
  //    std::vector<int>{1, 0, 0, 0, 0, 0
  //    , 0, 1, 0, 0, 0, 0
  //    , 1, 1, 1, 0, 0, 0
  //    , 0, 0, 0, 1, 0, 0
  //    , 0, 0, 0, 0, 1, 0
  //    , 0, 0, 0, 0, 0, 1}));
  MAPPING::Transform T(
      6, std::make_shared<std::vector<int>>(std::vector<int>{
             1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0,
             0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1}));
  ARCH::Level L1(16);
  L1.appendArray(32, 16, 16);
  L1.appendBuffer(ARCH::REG, ARCH::INPUT, 128, 16);
  L1.appendBuffer(ARCH::REG, ARCH::WEIGHT, 128, 16);
  L1.appendBuffer(ARCH::REG, ARCH::OUTPUT, 128, 16);
  L1.appendNetworkGroup(32, 16, ARCH::INPUT, 16, {1, 0, 1});
  L1.appendNetworkGroup(32, 16, ARCH::WEIGHT, 16, {0, 1, 1});
  L1.appendNetworkGroup(32, 16, ARCH::OUTPUT, 16, {0, 0, 1});

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
  I[c1 * 4 + c2][y + p][x + q];
  W[k][c1 * 4 + c2][p][q];
  O[k][y][x];
  // I[c][y1 * 6 + y2 + p][x + q];
  // W[k][c][p][q];
  // O[k][y1 * 6 + y2][x];
  // I[c][y + p][x + q];
  // W[k][c][p][q];
  // O[k][y][x];
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> coupledVarVec;
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> upperVarVec;
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> lowerVarVec;
  // coupledVarVec.push_back(k);
  // coupledVarVec.push_back(x);
  // coupledVarVec.push_back(c2);
  // coupledVarVec.push_back(c1);

  // coupledVarVec.push_back(k);
  // coupledVarVec.push_back(x);
  // coupledVarVec.push_back(y);
  // coupledVarVec.push_back(q);
  // coupledVarVec.push_back(c2);
  // coupledVarVec.push_back(c1);

  coupledVarVec.push_back(k);
  coupledVarVec.push_back(x);
  coupledVarVec.push_back(y);
  coupledVarVec.push_back(c2);
  coupledVarVec.push_back(c1);
  coupledVarVec.push_back(q);

  // coupledVarVec.push_back(k);
  // coupledVarVec.push_back(x);
  // coupledVarVec.push_back(c);
  // coupledVarVec.push_back(p);
  // coupledVarVec.push_back(q);
  // coupledVarVec.push_back(y);

  lowerVarVec.push_back(y);
  lowerVarVec.push_back(q);

  getTimeLine(coupledVarVec, T, I, W, O);

  std::cout << I.getRange(1).first << ' ' << I.getRange(1).second << std::endl;
  I.bindVar(coupledVarVec);
  W.bindVar(coupledVarVec);
  O.bindVar(coupledVarVec);

  MAPPING::Access accessI = MAPPING::constructAccessMatrix(I, coupledVarVec);
  MAPPING::Access accessW = MAPPING::constructAccessMatrix(W, coupledVarVec);
  MAPPING::Access accessO = MAPPING::constructAccessMatrix(O, coupledVarVec);

  std::shared_ptr<std::vector<std::vector<int>>> reuseVecI =
      compReuseVec(T, accessI);
  std::shared_ptr<std::vector<std::vector<int>>> reuseVecW =
      compReuseVec(T, accessW);
  std::shared_ptr<std::vector<std::vector<int>>> reuseVecO =
      compReuseVec(T, accessO);
  int baseData[3] = {1, 1, 1};
  Analyzer analyzer(coupledVarVec, lowerVarVec, T, 1, baseData, I, W, O, L1,
                    accessI, accessW, accessO, reuseVecI, reuseVecW, reuseVecO);
  analyzer.oneAnalysis();
  return 0;
}