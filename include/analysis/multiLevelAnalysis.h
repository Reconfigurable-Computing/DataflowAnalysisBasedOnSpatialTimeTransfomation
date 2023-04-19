#pragma once
#include "include/analysis/singleLevelAnalysis.h"
#include <fstream>
#include <queue>
#include <set>
#include <vector>
class MultLevelAnalyzer {
private:
  std::vector<Analyzer> _analyzerSet;
  WORKLOAD::Tensor &_I;
  WORKLOAD::Tensor &_W;
  WORKLOAD::Tensor &_O;
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> _allCoupledVarVec;
  std::vector<Result> _resultSet;
  void compRequiredDataSize(int level);

  void getSubLevelEdge(
      int level,
      std::map<std::shared_ptr<WORKLOAD::Iterator>,
               std::shared_ptr<WORKLOAD::Iterator>> &subLevelEdgeMap);
  int compBaseIndex(int varNum, std::vector<std::vector<int>> &state, int i);
  int getLevelNum();
  void
  changeEdgeByState(bool flag, int varNum, int i,
                    std::vector<std::vector<int>> &state,
                    std::vector<std::shared_ptr<WORKLOAD::Iterator>> &varVec);
  void generateSublevelBaseResult(
      int level, std::vector<std::shared_ptr<Result>> &subLevelResultVec,
      std::map<std::shared_ptr<WORKLOAD::Iterator>,
               std::shared_ptr<WORKLOAD::Iterator>> &subLevelEdgeMap,
      std::vector<Base> &baseVec);
  void recusiveAnalysis(int level);
  void compMultiLevelReuslt(std::shared_ptr<Result> resultTreeRoot);

public:
  MultLevelAnalyzer(WORKLOAD::Tensor &I, WORKLOAD::Tensor &W,
                    WORKLOAD::Tensor &O)
      : _I(I), _W(W), _O(O) {}
  void addLevel(std::vector<std::shared_ptr<WORKLOAD::Iterator>> &coupledVarVec,
                MAPPING::Transform &T, ARCH::Level &L, int spatialDimNum = 2,
                bool doubleBufferFlag = false);

  void oneAnalysis();
  void outputCSV();
};