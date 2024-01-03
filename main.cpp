
#include "include/analysis/costAnalysis.h"
#include "include/analysis/multiLevelAnalysis.h"
#include "include/analysis/singleLevelAnalysis.h"
#include "include/datastruct/arch.h"
#include "include/datastruct/mapping.h"
#include "include/datastruct/workload.h"
#include "include/searchEngine/groupSearchEngine.h"
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

long long DSE::TransformSearchEngine::totalCount = 0;
long long DSE::GroupSearchEngine::totalCount = 0;
long long DSE::MultiLevelTransformSearchEngine::_resultCount = 0;
extern COST::COSTDADA _Cost;
int main() {
  // std::shared_ptr<WORKLOAD::Iterator> k =
  //    std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(96, "k"));
  std::shared_ptr<WORKLOAD::Iterator> k_in =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(16, "k_in"));
  std::shared_ptr<WORKLOAD::Iterator> k_out =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(6, "k_out"));
  std::shared_ptr<WORKLOAD::Iterator> c =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(3, "c"));
  // std::shared_ptr<WORKLOAD::Iterator> oy =
  //    std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(55, "oy"));
  std::shared_ptr<WORKLOAD::Iterator> oy_in =
      std::make_shared<WORKLOAD::Iterator>(
          WORKLOAD::Iterator(0, 13, 0, 12, "oy_in"));
  std::shared_ptr<WORKLOAD::Iterator> oy_out =
      std::make_shared<WORKLOAD::Iterator>(
          WORKLOAD::Iterator(0, 2, oy_in, "oy_out"));

  std::shared_ptr<WORKLOAD::Iterator> ox =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(55, "ox"));
  std::shared_ptr<WORKLOAD::Iterator> r =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(11, "r"));
  std::shared_ptr<WORKLOAD::Iterator> s =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(11, "s"));
  std::shared_ptr<WORKLOAD::Iterator> n =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(4, "n"));
  WORKLOAD::Tensor I("I");
  WORKLOAD::Tensor W("W");
  WORKLOAD::Tensor O("O");

  I[n][c][56 * oy_out + 4 * oy_in + r][4 * ox + s];
  W[k_out * 16 + k_in][c][r][s];
  O[n][k_out * 16 + k_in][14 * oy_out + oy_in][ox];

  std::vector<std::shared_ptr<WORKLOAD::Iterator>> coupledVarVec1;
  coupledVarVec1.push_back(s);
  coupledVarVec1.push_back(k_in);
  MAPPING::Transform T1(
      coupledVarVec1.size(),
      std::make_shared<std::vector<int>>(std::vector<int>{1, 0, 0, 1}));
  ARCH::Level L1(16, true);
  L1.appendBuffer(ARCH::SRAM, ARCH::INPUT, 24);
  L1.appendBuffer(ARCH::SRAM, ARCH::WEIGHT, 448);
  L1.appendBuffer(ARCH::SRAM, ARCH::OUTPUT, 48);
  L1.appendNetworkGroup(ARCH::INPUT, 16, {0, 0, 0});
  L1.appendNetworkGroup(ARCH::WEIGHT, 16, {0, 0, 0});
  L1.appendNetworkGroup(ARCH::OUTPUT, 16, {0, 0, 0});

  std::vector<std::shared_ptr<WORKLOAD::Iterator>> coupledVarVec2;
  coupledVarVec2.push_back(r);
  coupledVarVec2.push_back(oy_in);
  coupledVarVec2.push_back(ox);

  MAPPING::Transform T2(coupledVarVec2.size(),
                        std::make_shared<std::vector<int>>(std::vector<int>{
                            0,
                            1,
                            0,
                            1,
                            0,
                            0,
                            0,
                            0,
                            1,
                        }));

  ARCH::Level L2(12, 14, 16);
  L2.appendBuffer(ARCH::SRAM, ARCH::TOTAL, 108 * 1024);
  L2.appendNetworkGroup(ARCH::INPUT, 27 * 64, {0, 0, 0});
  L2.appendNetworkGroup(ARCH::WEIGHT, 27 * 64, {1, 0, 0}, {0, 0, 1});
  L2.appendNetworkGroup(ARCH::OUTPUT, 27 * 64, {0, 1, 0});

  std::vector<std::shared_ptr<WORKLOAD::Iterator>> coupledVarVec3;
  coupledVarVec3.push_back(oy_out);
  coupledVarVec3.push_back(c);
  coupledVarVec3.push_back(k_out);
  coupledVarVec3.push_back(n);

  MAPPING::Transform T3(coupledVarVec3.size(),
                        std::make_shared<std::vector<int>>(std::vector<int>{
                            1,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            1,
                            0,
                            0,
                            1,
                            0,
                            0,
                            1,
                            0,
                            0,

                        }));
  ARCH::Level L3(16);
  L3.appendBuffer(ARCH::SRAM, ARCH::TOTAL, 8192 * 1024 * 2);
  L3.appendNetworkGroup(ARCH::INPUT, 64, {0, 0, 0});
  L3.appendNetworkGroup(ARCH::WEIGHT, 64, {0, 0, 1});
  L3.appendNetworkGroup(ARCH::OUTPUT, 64, {0, 0, 0});

  MultLevelAnalyzer multanalysis(I, W, O);
  multanalysis.addLevel(coupledVarVec1, T1, L1, false);
  multanalysis.addLevel(coupledVarVec2, T2, L2);
  multanalysis.addLevel(coupledVarVec3, T3, L3);

  std::cout << multanalysis.constraintCheck() << std::endl;
  std::cout << multanalysis.checkRequiredDataSize() << std::endl;
  multanalysis.oneAnalysis();
  return 0;
}