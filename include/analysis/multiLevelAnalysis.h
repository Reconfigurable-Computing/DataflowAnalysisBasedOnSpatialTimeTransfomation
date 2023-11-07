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
  std::vector<std::vector<std::shared_ptr<WORKLOAD::Iterator>>>
      _coupledVarVecVec;
  std::vector<std::shared_ptr<AnalyzerResult>> _resultSet;
  std::vector<bool> _validFlags;

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
  void extendCoupledVar(
      std::vector<std::shared_ptr<WORKLOAD::Iterator>> &coupledVarVec,
      int spatialDimNum);

public:
  bool compAndCheckRequiredDataSize(int level);
  bool checkRequiredDataSize();
  MultLevelAnalyzer(WORKLOAD::Tensor &I, WORKLOAD::Tensor &W,
                    WORKLOAD::Tensor &O)
      : _I(I), _W(W), _O(O) {}
  void addLevel(std::vector<std::shared_ptr<WORKLOAD::Iterator>> coupledVarVec,
                MAPPING::Transform &T, ARCH::Level &L,
                bool doubleBufferFlag = true);
  void addLevel(std::vector<std::shared_ptr<WORKLOAD::Iterator>> coupledVarVec,
                ARCH::Level &L, bool doubleBufferFlag = true);
  bool changeT(int level,
               std::vector<std::shared_ptr<WORKLOAD::Iterator>> &coupledVarVec,
               int spatialDimNum, MAPPING::Transform &T, bool checkFlag) {
    assert(level < _analyzerSet.size());
    if (checkFlag)
      extendT(coupledVarVec, spatialDimNum, T);
    _analyzerSet[level].changeT(T);
    if (!_analyzerSet[level].constraintCheckAndBuildAnalyzer()) {
      _validFlags[level] = false;
      return false;
    } else {
      _validFlags[level] = true;
      return true;
    }
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
  void outputLog(std::ofstream &logFile);
  void constructSearchResult(
      std::vector<std::shared_ptr<MultiLevelTransformSearchResult>>
          &mltsResult) {
    if (_resultSet.size() == 0)
      return;
    mltsResult.push_back(std::make_shared<MultiLevelTransformSearchResult>());
    int levelNum = _analyzerSet.size();
    int mltsResultSize = mltsResult.size();
    for (int i = 0; i < levelNum; i++) {
      mltsResult[mltsResultSize - 1]->addResult(_analyzerSet[i].getT(),
                                                _resultSet[i]);
    }
  }
  void getTimeLine(int level) { _analyzerSet[level].getTimeLine(); }
};