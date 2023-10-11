#pragma once
#include "include/analysis/singleLevelAnalysis.h"
#include "include/datastruct/result.h"
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
  std::vector<AnalyzerResult> _resultSet;
  std::vector<bool> _validFlags;
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
      int level,
      std::vector<std::shared_ptr<AnalyzerResult>> &subLevelResultVec,
      std::map<std::shared_ptr<WORKLOAD::Iterator>,
               std::shared_ptr<WORKLOAD::Iterator>> &subLevelEdgeMap,
      std::vector<Base> &baseVec);
  void recusiveAnalysis(int level);
  void compMultiLevelReuslt(std::shared_ptr<AnalyzerResult> resultTreeRoot);
  void compMultiLevelReusltDFS(std::shared_ptr<AnalyzerResult> node, int level);
  void extendT(std::vector<std::shared_ptr<WORKLOAD::Iterator>> &coupledVarVec,
               int spatialDimNum, MAPPING::Transform &T);

public:
  MultLevelAnalyzer(WORKLOAD::Tensor &I, WORKLOAD::Tensor &W,
                    WORKLOAD::Tensor &O)
      : _I(I), _W(W), _O(O), _validFlags(false) {}
  void addLevel(std::vector<std::shared_ptr<WORKLOAD::Iterator>> &coupledVarVec,
                MAPPING::Transform &T, ARCH::Level &L, int spatialDimNum = 2,
                bool doubleBufferFlag = true);
  void addLevel(std::vector<std::shared_ptr<WORKLOAD::Iterator>> &coupledVarVec,
                ARCH::Level &L, int spatialDimNum = 2,
                bool doubleBufferFlag = true);
  void changeT(int level,
               std::vector<std::shared_ptr<WORKLOAD::Iterator>> &coupledVarVec,
               int spatialDimNum, MAPPING::Transform &T) {
    assert(level < _analyzerSet.size());
    extendT(coupledVarVec, spatialDimNum, T);
    _analyzerSet[level].changeT(T);
    if (!_analyzerSet[level].constraintCheckAndBuildAnalyzer())
      _validFlags[level] = false;
    else
      _validFlags[level] = true;
  }
  bool constraintCheck() {
    bool ret = true;
    for (auto flag : _validFlags) {
      if (!flag)
        ret = false;
    }
    return ret;
  }
  void oneAnalysis();
  void outputCSV();
};