
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

class MultLevelAnalyzer {
private:
  std::vector<Analyzer> _analyzerSet;
  WORKLOAD::Tensor &_I;
  WORKLOAD::Tensor &_W;
  WORKLOAD::Tensor &_O;
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> _allCoupledVarVec;
  std::vector<Result> _resultSet;

public:
  MultLevelAnalyzer(WORKLOAD::Tensor &I, WORKLOAD::Tensor &W,
                    WORKLOAD::Tensor &O)
      : _I(I), _W(W), _O(O) {}
  void addLevel(std::vector<std::shared_ptr<WORKLOAD::Iterator>> &coupledVarVec,
                MAPPING::Transform &T, ARCH::Level &L,
                bool doubleBufferFlag = false) {
    for (auto var : coupledVarVec) {
      _allCoupledVarVec.push_back(var);
    }
    if (_analyzerSet.size() == 0) {
      _analyzerSet.push_back(
          Analyzer(coupledVarVec, T, _I, _W, _O, L, true, doubleBufferFlag));
    } else {
      _analyzerSet.push_back(
          Analyzer(coupledVarVec, T, _I, _W, _O, L, false, doubleBufferFlag));
    }
    _resultSet.push_back(Result());
  }
  void compRequiredDataSize(int level) {
    for (auto var : _allCoupledVarVec) {
      var->lock();
    }
    for (int i = 0; i <= level; i++) {
      std::vector<std::shared_ptr<WORKLOAD::Iterator>> &oneLevelCoupledVarVec =
          _analyzerSet[i].getCoupledVarVec();

      for (auto var : oneLevelCoupledVarVec) {
        var->unlock();
      }
    }

    _analyzerSet[level].compRequiredDataSize();

    for (auto var : _allCoupledVarVec) {
      var->unlock();
    }
  }

  void getSubLevelEdge(
      int level,
      std::map<std::shared_ptr<WORKLOAD::Iterator>,
               std::shared_ptr<WORKLOAD::Iterator>> &subLevelEdgeMap) {
    auto &curLevelCoupledVarVec = _analyzerSet[level].getCoupledVarVec();
    for (auto it = curLevelCoupledVarVec.rbegin();
         it != curLevelCoupledVarVec.rend(); it++) {
      if ((*it)->hasEdge()) {
        subLevelEdgeMap[(*it)->getCoupledIterator()] = *it;
      } else {
        if (subLevelEdgeMap.count((*it))) {
          subLevelEdgeMap.erase((*it));
        }
      }
    }
  }

  int getLevelNum() { return _analyzerSet.size(); }
  void oneAnalysis(int level) {
    if (level != 0) {
      std::map<std::shared_ptr<WORKLOAD::Iterator>,
               std::shared_ptr<WORKLOAD::Iterator>>
          subLevelEdgeMap;
      getSubLevelEdge(level, subLevelEdgeMap);
      if (subLevelEdgeMap.empty()) {
        oneAnalysis(level - 1);
        auto subLevelResult = _analyzerSet[level - 1].getResult();
        _analyzerSet[level].setBase(subLevelResult.delay,
                                    subLevelResult.requiredDataSize);
        _analyzerSet[level].oneAnalysis();
        compRequiredDataSize(level);
      } else {
        std::vector<std::shared_ptr<WORKLOAD::Iterator>> curSubCoupledVarVec;
        std::vector<std::vector<int>> state;
        std::map<int, Base> baseMap;
        for (auto &item : subLevelEdgeMap) {
          curSubCoupledVarVec.push_back(item.second);
        }
        WORKLOAD::generateEdgeState(state, curSubCoupledVarVec);
        int stateNum = state.size();
        int varNum = curSubCoupledVarVec.size();
        for (int i = 0; i < stateNum; i++) {
          for (int j = 0; j < varNum; j++) {
            if (state[i][j]) {
              curSubCoupledVarVec[j]->setEdge();
            }
          }
          oneAnalysis(level - 1);
          auto subLevelResult = _analyzerSet[level - 1].getResult();
          int tmp = 0;
          for (int j = 0; j < varNum; j++) {
            tmp *= 2;
            tmp += state[i][j];
          }
          baseMap[tmp] =
              Base(subLevelResult.delay, subLevelResult.requiredDataSize);
          for (int j = 0; j < varNum; j++) {
            if (state[i][j]) {
              curSubCoupledVarVec[j]->unsetEdge();
            }
          }
        }
      }
    } else {
      _analyzerSet[0].oneAnalysis();
      compRequiredDataSize(0);
    }
  }
};
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
  coupledVarVec1.push_back(c1);

  MAPPING::Transform T2(3, std::make_shared<std::vector<int>>(
                               std::vector<int>{1, 0, 0, 0, 1, 0, 1, 1, 1}));
  ARCH::Level L1(16);
  L1.appendArray(32, 16, 16);
  L1.appendBuffer(ARCH::REG, ARCH::INPUT, 128, 16);
  L1.appendBuffer(ARCH::REG, ARCH::WEIGHT, 128, 16);
  L1.appendBuffer(ARCH::REG, ARCH::OUTPUT, 128, 16);
  L1.appendNetworkGroup(32, 16, ARCH::INPUT, 16, {1, 0, 1});
  L1.appendNetworkGroup(32, 16, ARCH::WEIGHT, 16, {0, 1, 1});
  L1.appendNetworkGroup(32, 16, ARCH::OUTPUT, 16, {0, 0, 1});

  std::vector<std::shared_ptr<WORKLOAD::Iterator>> coupledVarVec2;
  coupledVarVec2.push_back(y);
  coupledVarVec2.push_back(p);
  coupledVarVec2.push_back(q);

  MAPPING::Transform T1(
      4, std::make_shared<std::vector<int>>(
             std::vector<int>{1, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1}));
  ARCH::Level L2(16);
  L2.appendArray(32, 16, 16);
  L2.appendBuffer(ARCH::REG, ARCH::INPUT, 128, 16);
  L2.appendBuffer(ARCH::REG, ARCH::WEIGHT, 128, 16);
  L2.appendBuffer(ARCH::REG, ARCH::OUTPUT, 128, 16);
  L2.appendNetworkGroup(32, 16, ARCH::INPUT, 16, {-1, 1, 0});
  L2.appendNetworkGroup(32, 16, ARCH::WEIGHT, 16, {1, 0, 1});
  L2.appendNetworkGroup(32, 16, ARCH::OUTPUT, 16, {0, 1, 0}, {0, 0, 1});

  std::cout << I.getRange(1).first << ' ' << I.getRange(1).second << std::endl;

  MultLevelAnalyzer multanalysis(I, W, O);
  multanalysis.addLevel(coupledVarVec1, T1, L1);
  multanalysis.addLevel(coupledVarVec2, T2, L2);
  multanalysis.oneAnalysis(multanalysis.getLevelNum() - 1);
  return 0;
}