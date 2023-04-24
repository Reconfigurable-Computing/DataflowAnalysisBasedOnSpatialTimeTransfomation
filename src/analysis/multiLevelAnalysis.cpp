#include "include/analysis/multiLevelAnalysis.h"

void MultLevelAnalyzer::addLevel(
    std::vector<std::shared_ptr<WORKLOAD::Iterator>> &coupledVarVec,
    MAPPING::Transform &T, ARCH::Level &L, int spatialDimNum,
    bool doubleBufferFlag) {
  if (spatialDimNum == 1) {
    T.addExtraSpatial();
    coupledVarVec.insert(coupledVarVec.begin(),
                         std::make_shared<WORKLOAD::Iterator>(
                             WORKLOAD::Iterator(0, 0, "tmpPEX")));
  } else if (spatialDimNum == 0) {
    T.addExtraSpatial();
    coupledVarVec.insert(coupledVarVec.begin(),
                         std::make_shared<WORKLOAD::Iterator>(
                             WORKLOAD::Iterator(0, 0, "tmpPEY")));
    T.addExtraSpatial();
    coupledVarVec.insert(coupledVarVec.begin(),
                         std::make_shared<WORKLOAD::Iterator>(
                             WORKLOAD::Iterator(0, 0, "tmpPEX")));
  }

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

void MultLevelAnalyzer::compRequiredDataSize(int level) {
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
void MultLevelAnalyzer::getSubLevelEdge(
    int level, std::map<std::shared_ptr<WORKLOAD::Iterator>,
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
int MultLevelAnalyzer::compBaseIndex(int varNum,
                                     std::vector<std::vector<int>> &state,
                                     int i) {
  int tmp = 0;
  for (int j = 0; j < varNum; j++) {
    tmp *= 2;
    tmp += state[i][j];
  }
  return tmp;
}
int MultLevelAnalyzer::getLevelNum() { return _analyzerSet.size(); }

void MultLevelAnalyzer::changeEdgeByState(
    bool flag, int varNum, int i, std::vector<std::vector<int>> &state,
    std::vector<std::shared_ptr<WORKLOAD::Iterator>> &varVec) {
  for (int j = 0; j < varNum; j++) {
    if (state[i][j]) {
      if (flag) {
        varVec[j]->setEdge();
      } else {
        varVec[j]->unsetEdge();
      }
    }
  }
}

void MultLevelAnalyzer::generateSublevelBaseResult(
    int level, std::vector<std::shared_ptr<Result>> &subLevelResultVec,
    std::map<std::shared_ptr<WORKLOAD::Iterator>,
             std::shared_ptr<WORKLOAD::Iterator>> &subLevelEdgeMap,
    std::vector<Base> &baseVec) {
  if (subLevelEdgeMap.empty()) {
    recusiveAnalysis(level - 1);
    auto subLevelResult = _analyzerSet[level - 1].getResult();
    subLevelResult->occTimes = _analyzerSet[level].getOccTimes();
    subLevelResultVec.push_back(subLevelResult);
    baseVec.push_back(
        Base(subLevelResult->delay, subLevelResult->requiredDataSize));
  } else {
    std::vector<std::shared_ptr<WORKLOAD::Iterator>> curSubCoupledVarVec;
    for (auto &item : subLevelEdgeMap) {
      curSubCoupledVarVec.push_back(item.second);
    }
    _analyzerSet[level].setCurSubCoupledVarVec(curSubCoupledVarVec);
    baseVec = std::vector<Base>(1 << subLevelEdgeMap.size());

    std::vector<std::vector<int>> state;
    WORKLOAD::generateEdgeState(state, curSubCoupledVarVec);
    int stateNum = state.size();
    int varNum = curSubCoupledVarVec.size();
    for (int i = 0; i < stateNum; i++) {
      changeEdgeByState(1, varNum, i, state, curSubCoupledVarVec);
      recusiveAnalysis(level - 1);
      auto subLevelResult = _analyzerSet[level - 1].getResult();
      subLevelResult->occTimes = _analyzerSet[level].getOccTimes();
      subLevelResultVec.push_back(subLevelResult);
      baseVec[compBaseIndex(varNum, state, i)] =
          Base(subLevelResult->delay, subLevelResult->requiredDataSize);
      changeEdgeByState(0, varNum, i, state, curSubCoupledVarVec);
    }
  }
}

void MultLevelAnalyzer::recusiveAnalysis(int level) {
  if (level != 0) {
    std::vector<std::shared_ptr<Result>> subLevelResultVec;
    std::map<std::shared_ptr<WORKLOAD::Iterator>,
             std::shared_ptr<WORKLOAD::Iterator>>
        subLevelEdgeMap;
    getSubLevelEdge(level, subLevelEdgeMap);
    std::vector<Base> baseVec;
    generateSublevelBaseResult(level, subLevelResultVec, subLevelEdgeMap,
                               baseVec);
    _analyzerSet[level].setBase(baseVec);
    _analyzerSet[level].oneAnalysis();
    _analyzerSet[level].setSubLevelResultVec(subLevelResultVec);
    compRequiredDataSize(level);
  } else {
    std::vector<Base> baseVec;
    baseVec.push_back(Base());
    _analyzerSet[0].setBase(baseVec);
    _analyzerSet[0].oneAnalysis();
    compRequiredDataSize(0);
  }
}

void MultLevelAnalyzer::compMultiLevelReuslt(
    std::shared_ptr<Result> resultTreeRoot) {
  compMultiLevelReusltDFS(resultTreeRoot, _analyzerSet.size() - 1);
  int levelNum = _analyzerSet.size();
  for (auto &result : _resultSet) {
  }
  for (int i = 0; i < levelNum; i++) {
    Result &result = _resultSet[i];
    result.compRate = result.compCycle / result.compRate;
    result.PEUtilRate =
        float(result.activePEMultTimeNum) / result.totalPEMultTimeNum;
    result.totalBandWidth[ARCH::INPUT] =
        _analyzerSet[i].compTotalBandWidth(ARCH::INPUT);
    result.totalBandWidth[ARCH::WEIGHT] =
        _analyzerSet[i].compTotalBandWidth(ARCH::WEIGHT);
    result.totalBandWidth[ARCH::OUTPUT] =
        _analyzerSet[i].compTotalBandWidth(ARCH::OUTPUT);
  }
}

void MultLevelAnalyzer::compMultiLevelReusltDFS(std::shared_ptr<Result> node,
                                                int level) {
  int occTimes = node->occTimes;
  _resultSet[level] += *node;
  for (auto item : node->subLevelResultVec) {
    item->occTimes *= occTimes;
    compMultiLevelReusltDFS(item, level - 1);
  }
}

void MultLevelAnalyzer::oneAnalysis() {
  int levelNum = getLevelNum();
  recusiveAnalysis(levelNum - 1);
  auto resultTreeRoot = _analyzerSet[levelNum - 1].getResult();
  resultTreeRoot->occTimes = 1;
  compMultiLevelReuslt(resultTreeRoot);

  outputCSV();
}

void MultLevelAnalyzer::outputCSV() {
  int levelNum = _analyzerSet.size();
  std::ofstream ofile;
  ofile.open("result.csv", std::ios::trunc);
  ofile << "level ,";
  ofile << "unique_output,";
  ofile << "reuse_output,";
  ofile << "total_output,";
  ofile << "reuseRate_output,";

  for (int j = 0; j < 2; j++) {
    ofile << std::string("unique_input_") + std::to_string(j) + ",";
    ofile << std::string("reuse_input_") + std::to_string(j) + ",";
    ofile << std::string("total_input_") + std::to_string(j) + ",";
    ofile << std::string("reuseRate_input_") + std::to_string(j) + ",";
  }
  ofile << "bufferSize_output,";
  for (int j = 0; j < 2; j++) {
    ofile << std::string("bufferSize_input_") + std::to_string(j) + ",";
  }
  ofile << "totalBandWidth_output,";
  for (int j = 0; j < 2; j++) {
    ofile << std::string("totalBandWidth_input_") + std::to_string(j) + ",";
  }
  ofile << "maxInitDelay_output,";
  for (int j = 0; j < 2; j++) {
    ofile << std::string("maxInitDelay_input_") + std::to_string(j) + ",";
  }
  ofile << "maxInitTimes,";
  ofile << "maxStableDelay_output,";
  for (int j = 0; j < 2; j++) {
    ofile << std::string("maxStableDelay_input_") + std::to_string(j) + ",";
  }
  ofile << std::string("maxStableCompDelay") + ",";
  ofile << std::string("delay") + ",";
  ofile << std::string("compCycleRate") + ",";
  ofile << "PEUtilRate" << std::endl;
  for (int i = 0; i < levelNum; i++) {
    ofile << std::to_string(i) + ',';
    ofile << std::to_string(_resultSet[i].uniqueVolumn[2]) + ",";
    ofile << std::to_string(_resultSet[i].reuseVolumn[2]) + ",";
    ofile << std::to_string(_resultSet[i].totalVolumn[2]) + ",";
    ofile << std::to_string(float(_resultSet[i].reuseVolumn[2]) /
                            _resultSet[i].totalVolumn[2]) +
                 ",";
    for (int j = 0; j < 2; j++) {
      ofile << std::to_string(_resultSet[i].uniqueVolumn[j]) + ",";
      ofile << std::to_string(_resultSet[i].reuseVolumn[j]) + ",";
      ofile << std::to_string(_resultSet[i].totalVolumn[j]) + ",";
      ofile << std::to_string(float(_resultSet[i].reuseVolumn[j]) /
                              _resultSet[i].totalVolumn[j]) +
                   ",";
    }
    ofile << std::to_string(_resultSet[i].requiredDataSize[2]) + ",";
    for (int j = 0; j < 2; j++) {
      ofile << std::to_string(_resultSet[i].requiredDataSize[j]) + ",";
    }
    ofile << std::to_string(_resultSet[i].totalBandWidth[2]) + ",";
    for (int j = 0; j < 2; j++) {
      ofile << std::to_string(_resultSet[i].totalBandWidth[j]) + ",";
    }
    ofile << std::to_string(_resultSet[i].initDelay[2]) + ",";
    for (int j = 0; j < 2; j++) {
      ofile << std::to_string(_resultSet[i].initDelay[j]) + ",";
    }
    ofile << std::to_string(_resultSet[i].initTimes) + ",";
    ofile << std::to_string(_resultSet[i].stableDelay[2]) + ",";
    for (int j = 0; j < 2; j++) {
      ofile << std::to_string(_resultSet[i].stableDelay[j]) + ",";
    }
    ofile << std::to_string(_resultSet[i].stableDelay[3]) + ",";
    ofile << std::to_string(_resultSet[i].delay) + ",";
    ofile << std::to_string(_resultSet[i].compRate) + ",";
    ofile << std::to_string(_resultSet[i].PEUtilRate) << std::endl;
  }
  ofile.close();
}