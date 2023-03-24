
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
  MAPPING::Access &_accessI;
  MAPPING::Access &_accessW;
  MAPPING::Access &_accessO;

public:
  Analyzer(std::vector<std::shared_ptr<WORKLOAD::Iterator>> &coupledVarVec,
           MAPPING::Transform &T, int base, WORKLOAD::Tensor &I,
           WORKLOAD::Tensor &W, WORKLOAD::Tensor &O, ARCH::Level &L1,
           MAPPING::Access &accessI, MAPPING::Access &accessW,
           MAPPING::Access &accessO)
      : _base(base), _coupledVarVec(coupledVarVec), _T(T), _I(I), _W(W), _O(O),
        _L1(L1), _accessI(accessI), _accessW(accessW), _accessO(accessO) {}
  std::pair<int, int> getTRange(int row) {
    int colNumT = _T.getColNum();
    std::shared_ptr<WORKLOAD::Polynomial> dim =
        std::make_shared<WORKLOAD::Polynomial>();
    for (int i = 0; i < colNumT; i++) {
      if (_T(row, i) == 1)
        dim = dim + _coupledVarVec[i];
    }
    std::pair<int, int> range = dim->getRange();
    return range;
  }

  int getActivePENum() {
    std::pair<int, int> PEXRange = getTRange(0);
    std::pair<int, int> PEYRange = getTRange(1);
    return (PEYRange.second - PEYRange.first + 1) *
           (PEXRange.second - PEXRange.first + 1);
  }

  int getComputationDelay() {
    int TDimNum = _T.getColNum();
    auto Tmatrix = _T.getMatrix();
    int totalActiveNum = 1;
    for (auto var : _coupledVarVec) {
      totalActiveNum *= var->getSize();
    }
    int activePENum = getActivePENum();

    return totalActiveNum / activePENum;
  }

  int getPerPEVolumn(MAPPING::Access &access) {
    int rowNum = access.getRowNum();
    int colNum = access.getColNum();
    int oneAccessPointUniqueVolumn = 1;
    // std::vector<std::shared_ptr<WORKLOAD::Polynomial>> dimSet;
    int totalUniqueVolumn = 0;
    for (int i = 0; i < rowNum; i++) {
      std::shared_ptr<WORKLOAD::Polynomial> dim =
          std::make_shared<WORKLOAD::Polynomial>();
      for (int j = 0; j < colNum; j++) {
        if (access(i, j) && _T(0, j) == 0 && _T(1, j) == 0)
          dim = dim + access(i, j) * _coupledVarVec[j];
      }
      // dimSet.push_back(dim);
      oneAccessPointUniqueVolumn *= dim->getSize();
    }
    return oneAccessPointUniqueVolumn;
  }

  void accessAnalysis(ARCH::DATATYPE dataType, MAPPING::Access &access) {
    int uniqueVolumn;
    int totalVolumn = 1;
    for (auto var : _coupledVarVec) {
      totalVolumn *= var->getSize();
    }
    int reuseVolumn;
    ARCH::NETWORKTYPE networkType = _L1.getNetworkType(dataType);
    int perPEVolumn = getPerPEVolumn(access);
    std::pair<int, int> PEXRange = getTRange(0);
    std::pair<int, int> PEYRange = getTRange(1);
    std::vector<std::pair<int, int>> accessPointSet;
    if (networkType == ARCH::SYSTOLIC || networkType == ARCH::MULTICAST) {
      _L1.getAccessPoint(dataType, accessPointSet);
      int activeAccessPointNum = 0;
      for (auto accessPoint : accessPointSet) {
        if (accessPoint.first >= PEXRange.first &&
            accessPoint.first <= PEXRange.second &&
            accessPoint.second >= PEYRange.first &&
            accessPoint.second <= PEYRange.second) {
          activeAccessPointNum++;
          ;
        }
      }
      uniqueVolumn = perPEVolumn * activeAccessPointNum;
      reuseVolumn = totalVolumn - uniqueVolumn;
    } else if (networkType == ARCH::STATIONARY) {
      uniqueVolumn = (PEYRange.second - PEYRange.first + 1) *
                     (PEXRange.second - PEXRange.first + 1) * perPEVolumn;
      reuseVolumn = totalVolumn - uniqueVolumn;
    } else {
      uniqueVolumn = (PEYRange.second - PEYRange.first + 1) *
                     (PEXRange.second - PEXRange.first + 1) * perPEVolumn;
      reuseVolumn = totalVolumn - uniqueVolumn;
    }
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
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 255, {'c'}));
  std::shared_ptr<WORKLOAD::Iterator> y =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 31, {'y'}));
  std::shared_ptr<WORKLOAD::Iterator> x =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 15, {'x'}));
  std::shared_ptr<WORKLOAD::Iterator> p =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 3, {'p'}));
  std::shared_ptr<WORKLOAD::Iterator> q =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 3, {'q'}));
  WORKLOAD::Tensor I("I");
  WORKLOAD::Tensor W("W");
  WORKLOAD::Tensor O("O");
  I[c][y + p][x + q];
  W[k][c][p][q];
  O[k][y][x];

  std::cout << I.getRange(1).first << ' ' << I.getRange(1).second << std::endl;
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> coupledVarVec;
  coupledVarVec.push_back(k);
  coupledVarVec.push_back(c);
  coupledVarVec.push_back(x);
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
  Analyzer analyzer(coupledVarVec, T, 1, I, W, O, L1, accessI, accessW,
                    accessO);
  analyzer.getComputationDelay();
  analyzer.accessAnalysis(ARCH::OUTPUT, accessO);
  analyzer.accessAnalysis(ARCH::INPUT, accessI);
  return 0;
}