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
  void startAnalysis(MultLevelAnalyzer &multanalysis,
                     std::vector<MultiLevelTransformSearchResult> &mltsResult,
                     int count, std::ofstream &ofile) {
    int levelNum = _transformSearchEngineSet.size();
    for (int i = 0; i < levelNum; i++) {
      auto &transformSearchEngine = _transformSearchEngineSet[i];
      transformSearchEngine.changeT(i, multanalysis);
    }
    std::cout << multanalysis.constraintCheck() << std::endl;
    multanalysis.oneAnalysis();
    multanalysis.constructSearchResult(mltsResult);
    ofile << "ANSWER:" << std::to_string(count) << "\n";
    multanalysis.outputTxt(ofile);
  }
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
  std::vector<MultiLevelTransformSearchResult> _mltsResult;

public:
  MultiLevelTransformSearchEngine(WORKLOAD::Tensor &I, WORKLOAD::Tensor &W,
                                  WORKLOAD::Tensor &O)
      : _I(I), _W(W), _O(O), _countCoupledVar(0) {}

  void addLevel(std::vector<std::shared_ptr<WORKLOAD::Iterator>> &coupledVarVec,
                ARCH::Level &L, int spatialDimNum) {
    _transformSearchEngineSet.emplace_back(coupledVarVec, L, spatialDimNum,
                                           _countCoupledVar);
    _countCoupledVar += coupledVarVec.size();
  }

  void oneSearch() {
    MultLevelAnalyzer multanalysis(_I, _W, _O);
    int levelNum = _transformSearchEngineSet.size();

    for (int i = 0; i < levelNum; i++) {
      auto &transformSearchEngine = _transformSearchEngineSet[i];
      transformSearchEngine.addLevel(multanalysis);
      transformSearchEngine.generateAllTransformMatrix(i, multanalysis);
    }
    if (!multanalysis.checkRequiredDataSize())
      return;
    Generator generator(_transformSearchEngineSet);
    if (!generator.isValid())
      return;
    std::ofstream ofile;
    ofile.open("mlsrlog.txt", std::ios::out);
    int count = 0;
    while (!generator.isEnd()) {
      generator.startAnalysis(multanalysis, _mltsResult, count++, ofile);
      generator.getNext();
    }
    generator.startAnalysis(multanalysis, _mltsResult, count++, ofile);
    ofile.close();
  }
};

} // namespace DSE