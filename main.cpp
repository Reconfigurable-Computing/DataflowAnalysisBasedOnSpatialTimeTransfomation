
#include "include/analysis/multiLevelAnalysis.h"
#include "include/analysis/singleLevelAnalysis.h"
#include "include/datastruct/arch.h"
#include "include/datastruct/mapping.h"
#include "include/datastruct/workload.h"
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

namespace DSE {

class Group {
public:
  std::vector<int> _indexVec;
  std::vector<Group> _subGroupVec;
  Group() {}
  Group(std::vector<int> indexVec) : _indexVec(indexVec) {}
};
class GroupSearchEngine {
  WORKLOAD::Tensor &_I;
  WORKLOAD::Tensor &_W;
  WORKLOAD::Tensor &_O;
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> _varVec;
  std::vector<int> _spatialNumVec;
  std::vector<ARCH::Level> _LVec;
  bool _firstFlag;
  std::vector<std::shared_ptr<GroupSearchResult>> _groupSearchResult;

public:
  static int totalCount;
  GroupSearchEngine(WORKLOAD::Tensor &I, WORKLOAD::Tensor &W,
                    WORKLOAD::Tensor &O,
                    std::vector<std::shared_ptr<WORKLOAD::Iterator>> &varVec)
      : _I(I), _W(W), _O(O), _varVec(varVec), _firstFlag(false) {}
  void addLevel(ARCH::Level &L) {
    _LVec.emplace_back(L);
    _spatialNumVec.push_back(L.getSpatialDimNum());
  }

  void combine(int n, int k, std::vector<Group> &groupVec) {
    std::vector<int> temp;
    for (int i = 1; i <= k; ++i) {
      temp.push_back(i);
    }
    temp.push_back(n + 1);

    int j = 0;
    while (j < k) {
      groupVec.emplace_back(std::vector<int>(temp.begin(), temp.begin() + k));
      j = 0;
      while (j < k && temp[j] + 1 == temp[j + 1]) {
        temp[j] = j + 1;
        ++j;
      }
      ++temp[j];
    }
  }

  void constructGroup(std::vector<int> &perGroupNum, int levelIndex, int varNum,
                      Group &rootGroup, std::vector<int> &candidate) {
    int maxLevelIndex = perGroupNum.size();
    combine(varNum, perGroupNum[levelIndex], rootGroup._subGroupVec);
    for (auto &group : rootGroup._subGroupVec) {
      std::set<int> notSubCandidate;
      for (int i = 0; i < perGroupNum[levelIndex]; i++) {
        group._indexVec[i] = candidate[group._indexVec[i] - 1];
        notSubCandidate.insert(group._indexVec[i]);
      }
      std::vector<int> subCandidate;
      for (auto num : candidate) {
        if (notSubCandidate.count(num) == 0)
          subCandidate.push_back(num);
      }
      if (levelIndex < maxLevelIndex - 1)
        constructGroup(perGroupNum, levelIndex + 1,
                       varNum - perGroupNum[levelIndex], group, subCandidate);
    }
  }

  void buildCoupleVarVec(
      Group &rootGroup, int levelIndex,
      std::vector<std::vector<std::shared_ptr<WORKLOAD::Iterator>>>
          &coupledVarVecVec,
      std::ofstream &logFile, bool logFlag) {
    if (levelIndex == _LVec.size()) {
      int levelNum = _LVec.size();
      if (logFlag) {
        if (!_firstFlag)
          logFile << ",\n";
        else
          _firstFlag = false;

        logFile << " \"Group Search: ";
        for (int i = 0; i < levelNum; i++) {
          auto vec = coupledVarVecVec[i];
          logFile << "Level " + std::to_string(i) << ' ';
          for (auto num : vec) {
            logFile << num->getSym() << ' ';
          }
        }
        logFile << "\":{" << std::endl;
      }
      totalCount += 1;
      DSE::MultiLevelTransformSearchEngine multiLevelTransformSearchEngine(
          _I, _W, _O);

      for (int i = 0; i < levelNum; i++) {
        multiLevelTransformSearchEngine.addLevel(coupledVarVecVec[i], _LVec[i]);
      }
      multiLevelTransformSearchEngine.oneSearch(logFile, logFlag);
      if (logFlag)
        logFile << "}" << std::endl;
      multiLevelTransformSearchEngine.constructGroupSearchResult(
          _groupSearchResult, coupledVarVecVec);

    } else {
      for (auto &group : rootGroup._subGroupVec) {
        std::vector<std::shared_ptr<WORKLOAD::Iterator>> coupledVarVec;
        for (auto index : group._indexVec)
          coupledVarVec.push_back(_varVec[index]);
        coupledVarVecVec.push_back(coupledVarVec);
        buildCoupleVarVec(group, levelIndex + 1, coupledVarVecVec, logFile,
                          logFlag);
        coupledVarVecVec.pop_back();
      }
    }
  }

  void recusiveCompPerGroupNum(std::vector<int> &perGroupNum, int varNum,
                               int levelNum, std::ofstream &logFile,
                               bool logFlag) {
    if (levelNum == 1) {
      // if (perGroupNum.size() != 0 && perGroupNum[perGroupNum.size() - 1] >
      // varNum)
      //    return;
      if (perGroupNum.size() != 0 &&
          varNum < std::max(1, _spatialNumVec[perGroupNum.size()]))
        return;
      perGroupNum.push_back(varNum);
      for (auto num : perGroupNum) {
        std::cout << num << ' ';
      }
      std::cout << std::endl;
      Group rootGroup;
      std::vector<int> candidate(_varVec.size(), 0);
      std::iota(candidate.begin(), candidate.end(), 0);
      constructGroup(perGroupNum, 0, _varVec.size(), rootGroup, candidate);
      std::vector<std::vector<std::shared_ptr<WORKLOAD::Iterator>>>
          coupledVarVecVec;
      buildCoupleVarVec(rootGroup, 0, coupledVarVecVec, logFile, logFlag);
      perGroupNum.pop_back();
    } else {
      // for (int i = perGroupNum.size() == 0 ? 1 :
      // perGroupNum[perGroupNum.size() - 1];
      //    i <= varNum - levelNum + 1; i++)
      for (int i = std::max(1, _spatialNumVec[perGroupNum.size()]);
           i <= varNum - levelNum + 1; i++) {
        perGroupNum.push_back(i);
        recusiveCompPerGroupNum(perGroupNum, varNum - i, levelNum - 1, logFile,
                                logFlag);
        perGroupNum.pop_back();
      }
    }
  }

  void oneSearch(std::ofstream &logFile, bool logFlag) {
    if (_varVec.size() < _LVec.size())
      return;
    MultiLevelTransformSearchEngine multiLevelTransformSearchEngine(_I, _W, _O);
    std::vector<int> perGroupNum;
    _firstFlag = true;
    if (logFlag)
      logFile << "{" << std::endl;
    recusiveCompPerGroupNum(perGroupNum, _varVec.size(), _LVec.size(), logFile,
                            logFlag);
    if (logFlag)
      logFile << "}" << std::endl;
  }

  static bool cmpResultByTotalCycle(std::shared_ptr<GroupSearchResult> &r1,
                                    std::shared_ptr<GroupSearchResult> &r2) {
    int levelSize =
        r1->_multiLevelTransformSearchResult->_transformSearchResult.size();
    std::vector<std::shared_ptr<TransformSearchResult>> &tr1 =
        r1->_multiLevelTransformSearchResult->_transformSearchResult;
    std::vector<std::shared_ptr<TransformSearchResult>> &tr2 =
        r2->_multiLevelTransformSearchResult->_transformSearchResult;
    if (tr1[levelSize - 1]->_result->delay < tr2[levelSize - 1]->_result->delay)
      return true;
    else if (tr1[levelSize - 1]->_result->delay ==
             tr2[levelSize - 1]->_result->delay) {
      long long unique1 = 0;
      long long unique2 = 0;
      for (int i = 0; i < levelSize; i++) {
        for (int j = 0; j < 3; j++) {
          unique1 += tr1[i]->_result->uniqueVolumn[j];
          unique2 += tr2[i]->_result->uniqueVolumn[j];
        }
      }
      if (unique1 < unique2)
        return true;
      else
        return false;
    } else
      return false;
  }
  void sortResult(int flag = 0) {
    if (flag == 0) {
      std::sort(_groupSearchResult.begin(), _groupSearchResult.end(),
                cmpResultByTotalCycle);
    }
    for (int i = 0; i < _groupSearchResult.size(); i++) {
      // printf("%lld ",
      // _groupSearchResult[i]->_multiLevelTransformSearchResult->_transformSearchResult[1]->_result->delay);

      std::vector<std::shared_ptr<TransformSearchResult>> &tr1 =
          _groupSearchResult[i]
              ->_multiLevelTransformSearchResult->_transformSearchResult;
      long long unique1 = 0;
      for (int j = 0; j < 2; j++) {
        for (int k = 0; k < 3; k++) {
          unique1 += tr1[j]->_result->uniqueVolumn[k];
        }
      }
      // printf("%lld\n", unique1);
      printf("%lld\n", tr1[0]->_result->requiredDataSize[0] +
                           tr1[0]->_result->requiredDataSize[1] +
                           tr1[0]->_result->requiredDataSize[2]);
    }
  }
  void outputTopResult(std::ofstream &ofile, int num = 100) {
    sortResult();
    num = std::min(num, int(_groupSearchResult.size()));
    ofile << "{\n";
    for (int i = 0; i < num; i++) {
      if (i != 0)
        ofile << ",\n";
      _groupSearchResult[i]->outputLog(ofile, i, _LVec.size());
    }
    ofile << "}";
  }
};
} // namespace DSE

namespace DSE {}
long long DSE::TransformSearchEngine::totalCount = 0;
int DSE::GroupSearchEngine::totalCount = 0;
int main() {
  std::shared_ptr<WORKLOAD::Iterator> k =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(256, "k"));
  std::shared_ptr<WORKLOAD::Iterator> k_in =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(16, "k_in"));
  std::shared_ptr<WORKLOAD::Iterator> k_out =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(16, "k_out"));
  std::shared_ptr<WORKLOAD::Iterator> c =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(48, "c"));
  std::shared_ptr<WORKLOAD::Iterator> c_out =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(24, "c_out"));
  std::shared_ptr<WORKLOAD::Iterator> c_in =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(2, "c_in"));
  std::shared_ptr<WORKLOAD::Iterator> oy =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(27, "oy"));

  std::shared_ptr<WORKLOAD::Iterator> ox =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(27, "ox"));
  std::shared_ptr<WORKLOAD::Iterator> r =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(5, "r"));
  std::shared_ptr<WORKLOAD::Iterator> s =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(5, "s"));
  std::shared_ptr<WORKLOAD::Iterator> n =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(4, "n"));
  WORKLOAD::Tensor I("I");
  WORKLOAD::Tensor W("W");
  WORKLOAD::Tensor O("O");

  I[n][c][oy + r][ox + s];
  W[k][c][r][s];
  O[n][k][oy][ox];

  std::vector<std::shared_ptr<WORKLOAD::Iterator>> coupledVarVec1;
  coupledVarVec1.push_back(k);
  coupledVarVec1.push_back(ox);
  coupledVarVec1.push_back(s);
  coupledVarVec1.push_back(r);
  coupledVarVec1.push_back(c);
  coupledVarVec1.push_back(oy);
  coupledVarVec1.push_back(n);

  MAPPING::Transform T1(coupledVarVec1.size(),
                        std::make_shared<std::vector<int>>(std::vector<int>{
                            1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1,
                            0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0,
                            0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1,
                        }));
  ARCH::Level L1(16, 16, 16);
  L1.appendBuffer(ARCH::REG, ARCH::INPUT);
  L1.appendBuffer(ARCH::REG, ARCH::WEIGHT);
  L1.appendBuffer(ARCH::REG, ARCH::OUTPUT);
  L1.appendNetworkGroup(ARCH::INPUT, 16, {1, 0, 0}, {0, 1, 0});
  L1.appendNetworkGroup(ARCH::WEIGHT, 16, {0, 0, 0});
  L1.appendNetworkGroup(ARCH::OUTPUT, 16, {0, 0, 0});

  MultLevelAnalyzer multanalysis(I, W, O);
  multanalysis.addLevel(coupledVarVec1, T1, L1);
  std::cout << multanalysis.constraintCheck() << std::endl;
  std::cout << multanalysis.checkRequiredDataSize() << std::endl;
  multanalysis.oneAnalysis();

  // MultLevelAnalyzer multanalysis(I, W, O);
  // multanalysis.addLevel(coupledVarVec1, T1, L1);
  // multanalysis.addLevel(coupledVarVec2, T2, L2);
  // multanalysis.addLevel(coupledVarVec3, T3, L3);
  // std::cout << multanalysis.constraintCheck() << std::endl;
  // std::cout << multanalysis.checkRequiredDataSize() << std::endl;
  // multanalysis.oneAnalysis();

  // std::vector<std::shared_ptr<WORKLOAD::Iterator>> coupledVarVec;
  // coupledVarVec.push_back(n);
  // coupledVarVec.push_back(k);
  // coupledVarVec.push_back(c_out);
  // coupledVarVec.push_back(c_in);
  // coupledVarVec.push_back(ox);
  // coupledVarVec.push_back(oy);
  // coupledVarVec.push_back(r);
  // coupledVarVec.push_back(s);
  // DSE::GroupSearchEngine groupSearchEngine(I, W, O, coupledVarVec);
  // groupSearchEngine.addLevel(L1);
  // groupSearchEngine.addLevel(L2);
  // groupSearchEngine.addLevel(L3);
  // std::ofstream logFile;
  // logFile.open("mlsrlog.json", std::ios::out);
  // groupSearchEngine.oneSearch(logFile, true);
  // logFile.close();
  // std::ofstream ofile;
  // ofile.open("groupresult.json", std::ios::out);
  // groupSearchEngine.outputTopResult(ofile);
  // ofile.close();
  // std::cout << DSE::GroupSearchEngine::totalCount << std::endl;
  // std::cout << DSE::TransformSearchEngine::totalCount << std::endl;

  // std::ofstream logFile;
  // logFile.open("mlsrlog.json", std::ios::out);
  // DSE::MultiLevelTransformSearchEngine multileveltransformsearchengine(I, W,
  // O); multileveltransformsearchengine.addLevel(coupledVarVec1, L1);
  // multileveltransformsearchengine.addLevel(coupledVarVec2, L2);
  // multileveltransformsearchengine.oneSearch(logFile, true);
  // logFile.close();
  // std::ofstream ofile;
  // ofile.open("mlsrresult.json", std::ios::out);
  // multileveltransformsearchengine.outputTopResult(ofile);
  // ofile.close();
  // std::cout << DSE::TransformSearchEngine::totalCount << std::endl;
  //

  return 0;
}