
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
namespace DSE {

struct TileCandidateCombine {
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> varVec;
  std::vector<int> sizeVec;
};

class TileSearchEngine {
private:
  WORKLOAD::Tensor _oriI;
  WORKLOAD::Tensor _oriW;
  WORKLOAD::Tensor _oriO;
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> _oriCoupledVarVec;

  WORKLOAD::Tensor _I;
  WORKLOAD::Tensor _W;
  WORKLOAD::Tensor _O;
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> _coupledVarVec;

  std::map<std::shared_ptr<WORKLOAD::Iterator>, std::vector<int>>
      _allIteratorCandidate;
  std::vector<ARCH::Level> _LVec;
  std::vector<std::shared_ptr<GroupSearchResult>> _groupSearchResult;

public:
  TileSearchEngine(WORKLOAD::Tensor &I, WORKLOAD::Tensor &W,
                   WORKLOAD::Tensor &O,
                   std::vector<std::shared_ptr<WORKLOAD::Iterator>> &varVec)
      : _oriI(I), _oriW(W), _oriO(O), _oriCoupledVarVec(varVec) {
    reset();
  }

  void addCancidate(std::shared_ptr<WORKLOAD::Iterator> iterator,
                    int candidate) {
    if (_allIteratorCandidate.count(iterator) == 0) {
      _allIteratorCandidate[iterator] = std::vector<int>();
    }
    _allIteratorCandidate[iterator].push_back(candidate);
  }

  void reset() {
    _coupledVarVec = _oriCoupledVarVec;
    _O = _oriO;
    _I = _oriI;
    _W = _oriW;
  }

  void split(std::shared_ptr<WORKLOAD::Iterator> iterator, int size) {
    int lowerBound = iterator->getLowBound();
    int upperBound = iterator->getUpBound();
    int iteratorRange = upperBound - lowerBound + 1;
    if (size == iteratorRange)
      return;
    int quotient = iteratorRange / size;
    int res = iteratorRange - size * quotient;
    std::shared_ptr<WORKLOAD::Iterator> outer;
    std::shared_ptr<WORKLOAD::Iterator> inner;
    if (res == 0) {
      outer = std::make_shared<WORKLOAD::Iterator>(
          WORKLOAD::Iterator(0, quotient - 1, iterator->getSym() + "_o"));
      inner = std::make_shared<WORKLOAD::Iterator>(
          WORKLOAD::Iterator(0, size - 1, iterator->getSym() + "_i"));
    } else {
      inner = std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(
          0, size - 1, 0, res - 1, iterator->getSym() + "_i"));
      outer = std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(
          0, quotient - 1, inner, iterator->getSym() + "_o"));
    }
    int coupledVarNum = _coupledVarVec.size();
    for (int i = 0; i < coupledVarNum; i++) {
      if (_coupledVarVec[i] == iterator) {
        _coupledVarVec[i] = inner;
        break;
      }
    }
    _coupledVarVec.push_back(outer);
    // change Tensor
    _I.splitIterator(iterator, outer, inner, size);
    _W.splitIterator(iterator, outer, inner, size);
    _O.splitIterator(iterator, outer, inner, size);
  }
  void combine(std::vector<TileCandidateCombine> &tileCandidateCombineVec,
               TileCandidateCombine &curCandidateCombine,
               std::vector<std::shared_ptr<WORKLOAD::Iterator>> &varVec,
               int index) {
    if (index == varVec.size()) {
      tileCandidateCombineVec.push_back(curCandidateCombine);
    } else {
      auto candidateVec = _allIteratorCandidate[varVec[index]];
      for (auto candidata : candidateVec) {
        curCandidateCombine.varVec.push_back(varVec[index]);
        curCandidateCombine.sizeVec.push_back(candidata);
        combine(tileCandidateCombineVec, curCandidateCombine, varVec,
                index + 1);
        curCandidateCombine.varVec.pop_back();
        curCandidateCombine.sizeVec.pop_back();
      }
    }
  }

  void oneSearch(std::ofstream &logFile, bool logFlag) {
    std::vector<TileCandidateCombine> tileCandidateCombineVec;
    TileCandidateCombine curCandidateCombine;
    std::vector<std::shared_ptr<WORKLOAD::Iterator>> varVec;
    for (auto &item : _allIteratorCandidate) {
      varVec.push_back(item.first);
    }

    combine(tileCandidateCombineVec, curCandidateCombine, varVec, 0);
    int num = varVec.size();
    for (auto tileCandidateCombine : tileCandidateCombineVec) {
      // for (int i = 0; i < num; i++)
      //{
      //    std::cout << tileCandidateCombine.varVec[i]->getSym() << ' ' <<
      //    tileCandidateCombine.sizeVec[i] << ' ';
      //}
      // std::cout << std::endl;
      reset();
      for (int i = 0; i < num; i++) {
        split(tileCandidateCombine.varVec[i], tileCandidateCombine.sizeVec[i]);
      }

      DSE::GroupSearchEngine groupSearchEngine(_I, _W, _O, _coupledVarVec);
      for (auto &L : _LVec) {
        groupSearchEngine.addLevel(L);
      }
      groupSearchEngine.oneSearch(logFile, logFlag);
      for (auto result : groupSearchEngine._groupSearchResult) {
        if (result->_multiLevelTransformSearchResult->_transformSearchResult[2]
                ->_result->delay > 10000000)
          continue;
        _groupSearchResult.push_back(result);
      }
      std::cout << groupSearchEngine._groupSearchResult.size() << std::endl;
      std::cout << _groupSearchResult.size() << std::endl;
      std::cout << _I.to_string() << ' ';
      std::cout << _W.to_string() << ' ';
      std::cout << _O.to_string() << ' ';
      std::cout << std::endl;
    }
  }
  void addLevel(ARCH::Level &L) { _LVec.emplace_back(L); }

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
      // long long unique1 = 0;
      // long long unique2 = 0;
      // for (int i = 0; i < levelSize; i++)
      //{
      //    for (int j = 0; j < 3; j++)
      //    {
      //        unique1 += tr1[i]->_result->uniqueVolumn[j];
      //        unique2 += tr2[i]->_result->uniqueVolumn[j];
      //    }
      //}
      // if (unique1 < unique2)
      //    return true;
      // else
      //    return false;
      long long unique1 = 0;
      long long unique2 = 0;

      for (int j = 0; j < 3; j++) {
        unique1 += tr1[1]->_result->requiredDataSize[j];
        unique2 += tr2[1]->_result->requiredDataSize[j];
      }
      if (unique1 < unique2)
        return true;
      else if (unique1 > unique2)
        return false;
      else {
        long long unique1 = 0;
        long long unique2 = 0;

        for (int j = 0; j < 3; j++) {
          unique1 += tr1[0]->_result->requiredDataSize[j];
          unique2 += tr2[0]->_result->requiredDataSize[j];
        }
        if (unique1 < unique2)
          return true;
        else
          return false;
      }
    } else
      return false;
  }
  void sortResult(int flag = 0) {
    if (flag == 0) {
      std::sort(_groupSearchResult.begin(), _groupSearchResult.end(),
                cmpResultByTotalCycle);
    }
    int levelSize =
        _groupSearchResult[0]
            ->_multiLevelTransformSearchResult->_transformSearchResult.size();
    long long curDelay = 0;
    for (int i = 0; i < _groupSearchResult.size(); i++) {
      if (curDelay == _groupSearchResult[i]
                          ->_multiLevelTransformSearchResult
                          ->_transformSearchResult[levelSize - 1]
                          ->_result->delay)
        continue;
      curDelay = _groupSearchResult[i]
                     ->_multiLevelTransformSearchResult
                     ->_transformSearchResult[levelSize - 1]
                     ->_result->delay;
      printf("%lld ", _groupSearchResult[i]
                          ->_multiLevelTransformSearchResult
                          ->_transformSearchResult[levelSize - 1]
                          ->_result->delay);

      // std::vector<std::shared_ptr<TransformSearchResult>> &tr1 =
      //     _groupSearchResult[i]
      //         ->_multiLevelTransformSearchResult->_transformSearchResult;
      // long long unique1 = 0;
      // for (int j = 0; j < 2; j++) {
      //   for (int k = 0; k < 3; k++) {
      //     unique1 += tr1[j]->_result->uniqueVolumn[k];
      //   }
      // }
      // printf("%lld\n", unique1);
      // printf("%lld\n", tr1[0]->_result->requiredDataSize[0] +
      // tr1[0]->_result->requiredDataSize[1] +
      // tr1[0]->_result->requiredDataSize[2]);
    }
  }
  void outputTopResult(std::ofstream &ofile, std::ofstream &ofile2,
                       int num = 100) {
    int levelSize =
        _groupSearchResult[0]
            ->_multiLevelTransformSearchResult->_transformSearchResult.size();
    sortResult();
    std::cout << _groupSearchResult.size() << std::endl;
    ofile << "{\n";
    long long curDelay = 0;
    int count = 0;
    for (int i = 0; i < _groupSearchResult.size(); i++) {
      if (curDelay == _groupSearchResult[i]
                          ->_multiLevelTransformSearchResult
                          ->_transformSearchResult[levelSize - 1]
                          ->_result->delay)
        continue;
      if (i != 0)
        ofile << ",\n";
      curDelay = _groupSearchResult[i]
                     ->_multiLevelTransformSearchResult
                     ->_transformSearchResult[levelSize - 1]
                     ->_result->delay;
      _groupSearchResult[i]->outputLog(ofile, i, _LVec.size());
      count++;
      if (count == num)
        break;
    }
    ofile << "}";
    ofile2 << "{\n";
    for (int i = 0; i < _groupSearchResult.size(); i++) {
      _groupSearchResult[i]->outputLog(ofile2, i, _LVec.size());
    }
    ofile2 << "}";
  }
};
} // namespace DSE
int main() {

  std::shared_ptr<WORKLOAD::Iterator> k_out =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(96, "k_out"));
  std::shared_ptr<WORKLOAD::Iterator> k_in =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(4, "k_in"));
  std::shared_ptr<WORKLOAD::Iterator> k =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(384, "k"));
  std::shared_ptr<WORKLOAD::Iterator> c =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(192, "c"));
  std::shared_ptr<WORKLOAD::Iterator> c_in =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(12, "c"));
  std::shared_ptr<WORKLOAD::Iterator> c_out =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(16, "c_out"));
  std::shared_ptr<WORKLOAD::Iterator> w =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(13, "w"));
  std::shared_ptr<WORKLOAD::Iterator> h =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(13, "h"));
  std::shared_ptr<WORKLOAD::Iterator> r =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(5, "r"));
  std::shared_ptr<WORKLOAD::Iterator> s =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(5, "s"));
  std::shared_ptr<WORKLOAD::Iterator> n =
      std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(4, "n"));
  WORKLOAD::Tensor I("I");
  WORKLOAD::Tensor W("W");
  WORKLOAD::Tensor O("O");

  // I[c][y + p][x + q];
  // W[k][c][p][q];
  // O[k][y][x];

  // I[c][y_out * 16 + y_in + p][x_out * 16 + x_in + q];
  // W[k_out * 16 + k_in][c][p][q];
  // O[k_out * 16 + k_in][y_out * 16 + y_in][x_out * 16 + x_in];

  // I[n][c][y + p][x + q];
  // W[k_out * 16 + k_in][c][p][q];
  // O[n][k_out * 16 + k_in][y][x];
  //
  // I[n][c][y + p][x + q];
  // W[k_out * 4 + k_in][c][p][q];
  // O[n][k_out * 4 + k_in][y][x];

  // I[n][c_out * 12 + c_in][h + r][w + s];
  // W[k][c_out * 12 + c_in][r][s];
  // O[n][k][h][w];

  I[n][c][h + r][w + s];
  W[k][c][r][s];
  O[n][k][h][w];

  std::vector<std::shared_ptr<WORKLOAD::Iterator>> coupledVarVec1;
  coupledVarVec1.push_back(w);

  MAPPING::Transform T1(
      coupledVarVec1.size(),
      std::make_shared<std::vector<int>>(std::vector<int>{1}));
  ARCH::Level L1(4, 16, true);
  L1.appendBuffer(ARCH::SRAM, ARCH::INPUT, 512);
  L1.appendBuffer(ARCH::SRAM, ARCH::WEIGHT, 512);
  L1.appendBuffer(ARCH::SRAM, ARCH::OUTPUT, 512);
  L1.appendNetworkGroup(ARCH::INPUT, 128, {0, 0, 0});
  L1.appendNetworkGroup(ARCH::WEIGHT, 128, {0, 1, 0});
  L1.appendNetworkGroup(ARCH::OUTPUT, 128, {0, 0, 1});

  std::vector<std::shared_ptr<WORKLOAD::Iterator>> coupledVarVec2;
  coupledVarVec2.push_back(k);
  coupledVarVec2.push_back(h);
  coupledVarVec2.push_back(s);
  coupledVarVec2.push_back(c_in);
  coupledVarVec2.push_back(c_out);
  coupledVarVec2.push_back(n);
  coupledVarVec2.push_back(r);

  MAPPING::Transform T2(coupledVarVec2.size(),
                        std::make_shared<std::vector<int>>(std::vector<int>{
                            1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1,
                            0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0,
                            0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1,

                        }));
  ARCH::Level L2(12, 16, 16);
  L2.appendBuffer(ARCH::SRAM, ARCH::TOTAL, 128000000);
  L2.appendNetworkGroup(ARCH::INPUT, 256, {1, 0, 0});
  L2.appendNetworkGroup(ARCH::WEIGHT, 256, {0, 1, 0});
  L2.appendNetworkGroup(ARCH::OUTPUT, 256, {0, 0, 1});

  ARCH::Level L3(1, 1, 16);
  L3.appendBuffer(ARCH::DRAM, ARCH::TOTAL, 128000000);
  L3.appendNetworkGroup(ARCH::INPUT, 256, {0, 0, 0});
  L3.appendNetworkGroup(ARCH::WEIGHT, 256, {0, 0, 0});
  L3.appendNetworkGroup(ARCH::OUTPUT, 256, {0, 0, 0});
  //   MultLevelAnalyzer multanalysis(I, W, O);
  //   multanalysis.addLevel(coupledVarVec1, T1, L1);
  //   multanalysis.addLevel(coupledVarVec2, T2, L2);

  //   std::cout << multanalysis.constraintCheck() << std::endl;
  //   std::cout << multanalysis.checkRequiredDataSize() << std::endl;
  //   multanalysis.oneAnalysis();

  /*   std::vector<std::shared_ptr<WORKLOAD::Iterator>> coupledVarVec;
     coupledVarVec.push_back(n);
     coupledVarVec.push_back(k);
     coupledVarVec.push_back(c_in);
     coupledVarVec.push_back(c_out);
     coupledVarVec.push_back(h);
     coupledVarVec.push_back(w);
     coupledVarVec.push_back(r);
     coupledVarVec.push_back(s);
     DSE::GroupSearchEngine groupSearchEngine(I, W, O, coupledVarVec);
     groupSearchEngine.addLevel(L1);
     groupSearchEngine.addLevel(L2);
     std::ofstream logFile;
     logFile.open("mlsrlog.json", std::ios::out);
     groupSearchEngine.oneSearch(logFile, true);
     logFile.close();
     std::ofstream ofile;
     ofile.open("groupresult.json", std::ios::out);
     groupSearchEngine.outputTopResult(ofile, 10000);
     ofile.close();
     printf("%lld\n", DSE::TransformSearchEngine::totalCount);*/

  std::vector<std::shared_ptr<WORKLOAD::Iterator>> coupledVarVec;
  coupledVarVec.push_back(n);
  coupledVarVec.push_back(k);
  coupledVarVec.push_back(c);
  coupledVarVec.push_back(h);
  coupledVarVec.push_back(w);
  coupledVarVec.push_back(r);
  coupledVarVec.push_back(s);
  DSE::TileSearchEngine tileSearchEngine(I, W, O, coupledVarVec);
  // tileSearchEngine.addCancidate(k, 8);
  tileSearchEngine.addCancidate(k, 12);
  tileSearchEngine.addCancidate(k, 24);
  // tileSearchEngine.addCancidate(k, 48);
  tileSearchEngine.addCancidate(k, 16);
  tileSearchEngine.addCancidate(k, 32);
  // tileSearchEngine.addCancidate(k, 64);
  // tileSearchEngine.addCancidate(k, 96);
  // tileSearchEngine.addCancidate(k, 128);
  // tileSearchEngine.addCancidate(k, 192);
  // tileSearchEngine.addCancidate(k, 384);
  // tileSearchEngine.addCancidate(c, 8);
  tileSearchEngine.addCancidate(c, 12);
  tileSearchEngine.addCancidate(c, 24);
  // tileSearchEngine.addCancidate(c, 48);
  tileSearchEngine.addCancidate(c, 16);
  tileSearchEngine.addCancidate(c, 32);
  // tileSearchEngine.addCancidate(c, 64);
  // tileSearchEngine.addCancidate(c, 96);
  // tileSearchEngine.addCancidate(c, 192);
  tileSearchEngine.addLevel(L1);
  tileSearchEngine.addLevel(L2);
  tileSearchEngine.addLevel(L3);
  std::ofstream logFile;
  logFile.open("mlsrlog.json", std::ios::out);
  tileSearchEngine.oneSearch(logFile, false);
  logFile.close();
  std::ofstream ofile;
  ofile.open("groupresulttop.json", std::ios::out);
  std::ofstream ofile2;
  ofile2.open("groupresult.json", std::ios::out);
  tileSearchEngine.outputTopResult(ofile, ofile2, 1000);
  ofile.close();
  ofile2.close();
  printf("%lld\n", DSE::TransformSearchEngine::totalCount);
  return 0;
}
