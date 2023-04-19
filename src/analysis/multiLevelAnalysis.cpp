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
  resultTreeRoot->occTimes = 1;
  std::queue<std::pair<int, std::shared_ptr<Result>>> _q;
  int curLevel = -1;
  _q.push(std::make_pair(0, resultTreeRoot));
  while (!_q.empty()) {
    auto front = _q.front();
    if (curLevel != front.first) {
      curLevel = front.first;
    }
    for (auto item : front.second->subLevelResultVec) {
      _q.push(std::make_pair(front.first + 1, item));
    }
    _resultSet[curLevel] += *(front.second);
    _q.pop();
  }
}

void MultLevelAnalyzer::oneAnalysis() {
  int levelNum = getLevelNum();
  recusiveAnalysis(levelNum - 1);
  auto resultTreeRoot = _analyzerSet[levelNum - 1].getResult();
  compMultiLevelReuslt(resultTreeRoot);
  outputCSV();
}

void MultLevelAnalyzer::outputCSV() {
  int levelNum = _analyzerSet.size();
  std::ofstream ofile;
  ofile.open("result.csv", std::ios::out);
  ofile << "level ,";
  ofile << "unique_output,";
  ofile << "reuse_output,";
  ofile << "total_output,";
  ofile << "bufferSize_output,";
  for (int j = 0; j < 2; j++) {
    ofile << std::string("unique_input_") + std::to_string(j) + ",";
    ofile << std::string("reuse_input_") + std::to_string(j) + ",";
    ofile << std::string("total_input_") + std::to_string(j) + ",";
    ofile << std::string("bufferSize_input_") + std::to_string(j) + ",";
  }
  ofile << "delay" << std::endl;

  for (int i = 0; i < levelNum; i++) {
    ofile << std::to_string(i) + ',';
    ofile << std::to_string(_resultSet[i].uniqueVolumn[2]) + ',';
    ofile << std::to_string(_resultSet[i].reuseVolumn[2]) + ',';
    ofile << std::to_string(_resultSet[i].totalVolumn[2]) + ',';
    ofile << std::to_string(_resultSet[i].requiredDataSize[2]) + ',';
    for (int j = 0; j < 2; j++) {
      ofile << std::to_string(_resultSet[i].uniqueVolumn[j]) + ',';
      ofile << std::to_string(_resultSet[i].reuseVolumn[j]) + ',';
      ofile << std::to_string(_resultSet[i].totalVolumn[j]) + ',';
      ofile << std::to_string(_resultSet[i].requiredDataSize[j]) + ',';
    }
    ofile << std::to_string(_resultSet[i].delay) << std::endl;
  }
  ofile.close();
}