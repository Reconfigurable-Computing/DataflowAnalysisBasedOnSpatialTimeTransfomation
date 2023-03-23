
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
typedef double valueType;

class Analyzer {
private:
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> &_coupledVarVec;
  MAPPING::Transform &_T;
  int _base;
  WORKLOAD::Tensor &_I;
  WORKLOAD::Tensor &_W;
  WORKLOAD::Tensor &_O;
  ARCH::Level &_L1;

public:
  Analyzer(std::vector<std::shared_ptr<WORKLOAD::Iterator>> &coupledVarVec,
           MAPPING::Transform &T, int base, WORKLOAD::Tensor &I,
           WORKLOAD::Tensor &W, WORKLOAD::Tensor &O, ARCH::Level &L1)
      : _base(base), _coupledVarVec(coupledVarVec), _T(T), _I(I), _W(W), _O(O),
        _L1(L1) {}
  int getMappingRange(int row) {
    int ret = 0;
    int TDimNum = _T.getColNum();
    for (int i = 0; i < TDimNum; i++) {
      if (_T(row, i)) {
        ret += _coupledVarVec[i]->getRange();
      }
    }
    return ret;
  }

  int getActivePENum() { return getMappingRange(0) * getMappingRange(1); }

  int getComputationDelay() {
    int TDimNum = _T.getColNum();
    auto Tmatrix = _T.getMatrix();
    int totalActiveNum = 1;
    for (auto var : _coupledVarVec) {
      totalActiveNum *= var->getRange();
    }
    int activePENum = getActivePENum();

    return totalActiveNum / activePENum;
  }
  int getTotalVolumn(WORKLOAD::Tensor &curTensor) { return 0; }

  int getUniqueVolumn() {
    std::vector<std::pair<int, int>> ret;
    _L1.getNetworkType(ARCH::INPUT);
    _L1.getAccessPoint(ARCH::INPUT, ret);
    int rowNumT = _T.getRowNum();
    int colNumT = _T.getColNum();
    std::vector<std::vector<int>> TSpatial;
    std::vector<int> tmp;
    for (int i = 0; i < rowNumT - 1; i++) {
      tmp.clear();
      for (int j = 0; j < colNumT; j++) {
        tmp.push_back(_T(i, j));
      }
      TSpatial.push_back(tmp);
    }
    auto solution = solvingLinearEquations(TSpatial, 0);
    solution->pop_back();
    std::shared_ptr<WORKLOAD::Iterator> PEX =
        std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 31, {'x'}));
    std::shared_ptr<WORKLOAD::Iterator> PEY =
        std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 31, {'x'}));

    return 0;
  }
};

// std::shared_ptr<std::vector<int>>

int main() {

  MAPPING::Transform T(3, std::make_shared<std::vector<int>>(
                              std::vector<int>{1, 0, 0, 0, 1, 0, 1, 1, 1}));
  ARCH::Level L1;
  L1.appendArray(32, 32, 16);
  L1.appendBuffer(ARCH::REG, ARCH::INPUT, 128, 16);
  L1.appendBuffer(ARCH::REG, ARCH::WEIGHT, 128, 16);
  L1.appendBuffer(ARCH::REG, ARCH::OUTPUT, 128, 16);
  L1.appendNetworkGroup(32, 32, ARCH::INPUT, {1, 0, 1});
  L1.appendNetworkGroup(32, 32, ARCH::WEIGHT, {0, 1, 1});
  L1.appendNetworkGroup(32, 32, ARCH::OUTPUT, {0, 0, 1});

  std::shared_ptr<WORKLOAD::Iterator> k =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 3, {'k'}));
  std::shared_ptr<WORKLOAD::Iterator> c =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 15, {'c'}));
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

  std::cout << I.getRange(1).first << ' ' << I.getRange(1).second << std::endl;
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> coupledVarVec;
  coupledVarVec.push_back(k);
  coupledVarVec.push_back(x);
  coupledVarVec.push_back(c);
  y->lock();
  p->lock();
  q->lock();
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
  Analyzer analyzer(coupledVarVec, T, 1, I, W, O, L1);
  analyzer.getComputationDelay();
  analyzer.getUniqueVolumn();
  return 0;
}