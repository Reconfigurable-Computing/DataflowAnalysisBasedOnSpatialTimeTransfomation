#pragma once
#include "include/analysis/multiLevelAnalysis.h"
#include "include/datastruct/arch.h"
#include "include/datastruct/mapping.h"
#include "include/datastruct/workload.h"
#include <algorithm>
#include <numeric>

namespace DSE {
class TransformSearchEngine {
private:
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> &_coupledVarVec;
  ARCH::Level &_L;
  int _spatialDimNum;

  std::vector<MAPPING::Transform> _TVec;
  int _TVecIndex;

public:
  static long long totalCount;
  TransformSearchEngine(
      std::vector<std::shared_ptr<WORKLOAD::Iterator>> &coupledVarVec,
      ARCH::Level &L, int spatialDimNum, int preCoupledNum)
      : _coupledVarVec(coupledVarVec), _L(L), _spatialDimNum(spatialDimNum)

  {
    assert(_coupledVarVec.size() >= _spatialDimNum);
    int dimNum = _coupledVarVec.size();

    _TVecIndex = 0;
    _TVec.clear();
  }

  void addLevel(MultLevelAnalyzer &multanalysis) {
    multanalysis.addLevel(_coupledVarVec, _L, _spatialDimNum);
  }
  void setMatrixAsOne(MAPPING::Transform &T, std::vector<int> &permute,
                      int start);

  void generateAllTransformMatrix(int level, MultLevelAnalyzer &multanalysis);

  void generateTransformMatrix(std::vector<int> &permute,
                               std::vector<MAPPING::Transform> &Tvec);
  bool isTop() { return _TVec.size() - 1 == _TVecIndex; }
  void getNext() {
    if (isTop()) {
      _TVecIndex = 0;
    }

    else {
      _TVecIndex++;
    }
  }
  void changeT(int level, MultLevelAnalyzer &multanalysis) {
    multanalysis.changeT(level, _coupledVarVec, _spatialDimNum,
                         _TVec[_TVecIndex], false);
  }
  bool isEmpty() { return _TVec.size() == 0; }
};
class Generator {
  std::vector<TransformSearchEngine> &_transformSearchEngineSet;

public:
  Generator(std::vector<TransformSearchEngine> &transformSearchEngineSet)
      : _transformSearchEngineSet(transformSearchEngineSet) {}
  bool isEnd() {
    for (auto &transformSearchEngine : _transformSearchEngineSet) {
      if (!transformSearchEngine.isTop()) {
        return false;
      }
    }
    return true;
  }
  void getNext() {
    for (auto &transformSearchEngine : _transformSearchEngineSet) {
      if (transformSearchEngine.isTop()) {
        transformSearchEngine.getNext();
      } else {
        transformSearchEngine.getNext();
        break;
      }
    }
  }
  void startAnalysis(
      MultLevelAnalyzer &multanalysis,
      std::vector<std::shared_ptr<MultiLevelTransformSearchResult>> &mltsResult,
      int count, std::ofstream &logFile, bool logFlag, bool firstFlag);
  bool isValid() {
    for (auto &transformSearchEngine : _transformSearchEngineSet) {
      if (transformSearchEngine.isEmpty())
        return false;
    }
    return true;
  }
};

class MultiLevelTransformSearchEngine {
  WORKLOAD::Tensor &_I;
  WORKLOAD::Tensor &_W;
  WORKLOAD::Tensor &_O;
  std::vector<TransformSearchEngine> _transformSearchEngineSet;
  int _countCoupledVar;
  std::vector<std::shared_ptr<MultiLevelTransformSearchResult>> _mltsResult;

public:
  MultiLevelTransformSearchEngine(WORKLOAD::Tensor &I, WORKLOAD::Tensor &W,
                                  WORKLOAD::Tensor &O)
      : _I(I), _W(W), _O(O), _countCoupledVar(0) {}

  void addLevel(std::vector<std::shared_ptr<WORKLOAD::Iterator>> &coupledVarVec,
                ARCH::Level &L) {
    int spatialDimNum = L.getSpatialDimNum();
    _transformSearchEngineSet.emplace_back(coupledVarVec, L, spatialDimNum,
                                           _countCoupledVar);
    _countCoupledVar += coupledVarVec.size();
  }

  void oneSearch(std::ofstream &logFile, bool logFlag) {

    MultLevelAnalyzer multanalysis(_I, _W, _O);
    int levelNum = _transformSearchEngineSet.size();

    for (int i = 0; i < levelNum; i++) {
      auto &transformSearchEngine = _transformSearchEngineSet[i];
      transformSearchEngine.addLevel(multanalysis);
    }
    if (!multanalysis.checkRequiredDataSize())
      return;
    for (int i = 0; i < levelNum; i++) {
      auto &transformSearchEngine = _transformSearchEngineSet[i];
      transformSearchEngine.generateAllTransformMatrix(i, multanalysis);
    }

    Generator generator(_transformSearchEngineSet);
    if (!generator.isValid())
      return;

    int count = 0;
    int firstFlag = true;
    while (!generator.isEnd()) {
      generator.startAnalysis(multanalysis, _mltsResult, count++, logFile,
                              logFlag, firstFlag);
      generator.getNext();
      firstFlag = false;
    }
    generator.startAnalysis(multanalysis, _mltsResult, count++, logFile,
                            logFlag, firstFlag);
  }

  static bool
  cmpResultByTotalCycle(std::shared_ptr<MultiLevelTransformSearchResult> &r1,
                        std::shared_ptr<MultiLevelTransformSearchResult> &r2) {
    int levelSize = r1->_transformSearchResult.size();
    if (r1->_transformSearchResult[levelSize - 1]->_result->delay <
        r2->_transformSearchResult[levelSize - 1]->_result->delay)
      return true;
    else
      return false;
  }
  void sortResult(int flag = 0) {
    if (flag == 0) {
      std::sort(_mltsResult.begin(), _mltsResult.end(), cmpResultByTotalCycle);
    }
  }
  void outputTopResult(std::ofstream &ofile, int num = 10) {
    sortResult();
    num = std::min(num, int(_mltsResult.size()));
    ofile << "{\n";
    for (int i = 0; i < num; i++) {
      if (i != 0)
        ofile << ",\n";
      ofile << "\"ANSWER:" << std::to_string(i) << "\":{";
      _mltsResult[i]->outputLog(ofile);
      ofile << "}";
    }
    ofile << "}";
  }
  void constructGroupSearchResult(
      std::vector<std::shared_ptr<GroupSearchResult>> &groupSearchResult,
      std::vector<std::vector<std::shared_ptr<WORKLOAD::Iterator>>>
          &coupledVarVecVec) {
    int resultNum = _mltsResult.size();
    for (int i = 0; i < resultNum; i++) {
      groupSearchResult.push_back(std::make_shared<GroupSearchResult>(
          coupledVarVecVec, _mltsResult[i]));
    }
  }
};

} // namespace DSE