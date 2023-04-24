#include "include/analysis/singleLevelAnalysis.h"

void Analyzer::setEdge(std::shared_ptr<WORKLOAD::Iterator> curI) {
  curI->setEdge();
  if (_curSubCoupledVarSet.count(curI)) {
    changeBase();
  }
}

void Analyzer::unsetEdge(std::shared_ptr<WORKLOAD::Iterator> curI) {
  curI->unsetEdge();
  if (_curSubCoupledVarSet.count(curI)) {
    changeBase();
  }
}

void Analyzer::changeBase() {
  int curSubCoupledVarNum = _curSubCoupledVarVec.size();
  int tmp = 0;
  for (int i = 0; i < curSubCoupledVarNum; i++) {
    tmp *= 2;
    tmp += _curSubCoupledVarVec[i]->isEdge();
  }
  _curBaseIndex = tmp;
}

void Analyzer::oneAnalysis() {
  _result = std::make_shared<Result>(Result());
  std::vector<int> innerTimeVec;
  std::vector<int> outerTimeVec;
  constructInnerOuterTimeVec(innerTimeVec, outerTimeVec);
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> outerVarVec;
  generateVarVec(outerTimeVec, outerVarVec);
  std::vector<std::vector<int>> state;
  WORKLOAD::generateEdgeState(state, outerVarVec);

  int stateNum = state.size();
  int varNum = outerVarVec.size();
  if (stateNum == 0) {
    delayAnalysis(innerTimeVec, outerTimeVec);
    accessAnalysis(innerTimeVec, outerTimeVec);
  } else {
    for (int i = 0; i < stateNum; i++) {
      changeEdgeByState(1, varNum, i, state, outerVarVec);
      delayAnalysis(innerTimeVec, outerTimeVec);
      accessAnalysis(innerTimeVec, outerTimeVec);
      changeEdgeByState(0, varNum, i, state, outerVarVec);
    }
  }
}

void Analyzer::delayAnalysis(std::vector<int> &innerTimeVec,
                             std::vector<int> &outerTimeVec) {
  int outerTimeSize = compOneStateTimeSize(outerTimeVec);
  int coupleVarNum = _coupledVarVec.size();
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> innerVarVec;
  generateVarVec(innerTimeVec, innerVarVec);
  std::vector<std::vector<int>> state;
  WORKLOAD::generateEdgeState(state, innerVarVec);
  int initDelay = 0;
  int delay = 0;
  int compCycle = 0;
  int activePEMultTimeNum = 0;
  int stateNum = state.size();
  int innerVarNum = innerVarVec.size();
  int ret = 0;
  if (stateNum == 0) {
    compOneStateDelay(innerTimeVec, delay, compCycle, initDelay,
                      activePEMultTimeNum);
  } else {
    for (int i = 0; i < stateNum; i++) {
      changeEdgeByState(1, innerVarNum, i, state, innerVarVec);
      compOneStateDelay(innerTimeVec, delay, compCycle, initDelay,
                        activePEMultTimeNum);
      changeEdgeByState(0, innerVarNum, i, state, innerVarVec);
    }
  }
  if (_lowestFlag || !_doubleBufferFlag) {
    delay += initDelay;
  } else // doublebuffer
  {
    delay = std::max(initDelay, delay);
  }
  _result->delay += outerTimeSize * delay;
  _result->compCycle += outerTimeSize * compCycle;
  _result->activePEMultTimeNum += outerTimeSize * activePEMultTimeNum;
  _result->totalPEMultTimeNum += outerTimeSize * delay * _L.getPENum();
  _result->initTimes += outerTimeSize;
}

int Analyzer::compOneStateStableDelay() {
  int inputStableDelay =
      _L.getStableDelay(ARCH::INPUT, _baseSet[_curBaseIndex].baseData[0]);
  int weightStableDelay =
      _L.getStableDelay(ARCH::WEIGHT, _baseSet[_curBaseIndex].baseData[1]);
  int outputStableDelay =
      _L.getStableDelay(ARCH::OUTPUT, _baseSet[_curBaseIndex].baseData[2]);
  if (_curBaseIndex == 0) {
    _result->stableDelay[ARCH::INPUT] =
        std::max(_result->stableDelay[ARCH::INPUT], inputStableDelay);
    _result->stableDelay[ARCH::WEIGHT] =
        std::max(_result->stableDelay[ARCH::WEIGHT], weightStableDelay);
    _result->stableDelay[ARCH::OUTPUT] =
        std::max(_result->stableDelay[ARCH::OUTPUT], outputStableDelay);
    _result->stableDelay[3] = std::max(_result->stableDelay[3],
                                       _baseSet[_curBaseIndex].baseCompCycle);
  }
  return std::max(std::max(std::max(inputStableDelay, weightStableDelay),
                           outputStableDelay),
                  _baseSet[_curBaseIndex].baseCompCycle);
}

int Analyzer::compOneStateInitDelay(std::pair<int, int> &PEXRange,
                                    std::pair<int, int> &PEYRange) {
  int inputInitDelay = _L.getInitOrOutDelay(
      ARCH::INPUT, _baseSet[_curBaseIndex].baseData[0], PEXRange, PEYRange);
  int weightInitDelay = _L.getInitOrOutDelay(
      ARCH::WEIGHT, _baseSet[_curBaseIndex].baseData[1], PEXRange, PEYRange);
  int outputOutDelay = _L.getInitOrOutDelay(
      ARCH::OUTPUT, _baseSet[_curBaseIndex].baseData[2], PEXRange, PEYRange);
  if (_curBaseIndex == 0) {
    _result->initDelay[ARCH::INPUT] =
        std::max(_result->initDelay[ARCH::INPUT], inputInitDelay);
    _result->initDelay[ARCH::WEIGHT] =
        std::max(_result->initDelay[ARCH::WEIGHT], weightInitDelay);
    _result->initDelay[ARCH::OUTPUT] =
        std::max(_result->initDelay[ARCH::OUTPUT], outputOutDelay);
  }
  return std::max(std::max(inputInitDelay, weightInitDelay), outputOutDelay);
}

void Analyzer::compOneStateDelay(std::vector<int> &innerTimeVec, int &delay,
                                 int &compCycle, int &initDelay,
                                 int &activePEMultTimeNum) {
  std::pair<int, int> PEXRange = compTRange(0);
  std::pair<int, int> PEYRange = compTRange(1);
  int curDelay = compOneStateStableDelay() * compOneStateTimeSize(innerTimeVec);
  delay += curDelay;
  PEX->lock();
  PEY->lock();
  int curCompCycle = _baseSet[_curBaseIndex].baseCompCycle *
                     compOneStateTimeSize(innerTimeVec);
  compCycle += curCompCycle;
  activePEMultTimeNum += (PEYRange.second - PEYRange.first + 1) *
                         (PEXRange.second - PEXRange.first + 1) * curCompCycle;
  PEX->unlock();
  PEY->unlock();
  initDelay = std::max(compOneStateInitDelay(PEXRange, PEYRange), initDelay);
}

void Analyzer::accessAnalysis(std::vector<int> &innerTimeVec,
                              std::vector<int> &outerTimeVec) {
  PEX->lock();
  PEY->lock();
  int outerTimeSize = compOneStateTimeSize(outerTimeVec);
  PEX->unlock();
  PEY->unlock();
  compOneDataVolumn(ARCH::OUTPUT, _accessO, innerTimeVec, outerTimeSize);
  compOneDataVolumn(ARCH::INPUT, _accessI, innerTimeVec, outerTimeSize);
  compOneDataVolumn(ARCH::WEIGHT, _accessW, innerTimeVec, outerTimeSize);
}

std::pair<int, int> Analyzer::compTRange(int row) {
  int colNumT = _T.getColNum();
  std::shared_ptr<WORKLOAD::Polynomial> dim =
      std::make_shared<WORKLOAD::Polynomial>();
  for (int i = 0; i < colNumT; i++) {
    if (_T(row, i) == 1)
      dim = dim + _coupledVarVec[i];
  }
  std::pair<int, int> range = dim->getRange();
  return range;
}

void Analyzer::compOneStateVolumn(int &uniqueVolumn, int &totalVolumn,
                                  std::vector<int> &innerTimeVec,
                                  ARCH::DATATYPE dataType) {
  std::pair<int, int> PEXRange = compTRange(0);
  std::pair<int, int> PEYRange = compTRange(1);
  ARCH::NETWORKTYPE networkType = _L.getNetworkType(dataType);
  PEX->lock();
  PEY->lock();
  int timeSize = compOneStateTimeSize(innerTimeVec);
  PEX->unlock();
  PEY->unlock();
  totalVolumn += timeSize * _baseSet[_curBaseIndex].baseData[dataType] *
                 (PEYRange.second - PEYRange.first + 1) *
                 (PEXRange.second - PEXRange.first + 1);
  if (_L.checkIfStationary(dataType)) {
    if (networkType == ARCH::SYSTOLIC || networkType == ARCH::MULTICAST) {
      uniqueVolumn = std::max(
          uniqueVolumn,
          _baseSet[_curBaseIndex].baseData[dataType] *
              _L.getActiveAccessPointNum(dataType, PEXRange, PEYRange));
    } else {
      uniqueVolumn =
          std::max(uniqueVolumn, _baseSet[_curBaseIndex].baseData[dataType] *
                                     (PEYRange.second - PEYRange.first + 1) *
                                     (PEXRange.second - PEXRange.first + 1));
    }
  } else {
    if (networkType == ARCH::SYSTOLIC || networkType == ARCH::MULTICAST) {
      uniqueVolumn += timeSize * _baseSet[_curBaseIndex].baseData[dataType] *
                      _L.getActiveAccessPointNum(dataType, PEXRange, PEYRange);
    } else {
      uniqueVolumn += timeSize * _baseSet[_curBaseIndex].baseData[dataType] *
                      (PEYRange.second - PEYRange.first + 1) *
                      (PEXRange.second - PEXRange.first + 1);
    }
  }
}

void Analyzer::compOneDataVolumn(ARCH::DATATYPE dataType,
                                 MAPPING::Access &access,
                                 std::vector<int> &innerTimeVec,
                                 int outerTimeSize) {
  ARCH::NETWORKTYPE networkType = _L.getNetworkType(dataType);
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> innerVarVec;
  generateVarVec(innerTimeVec, innerVarVec);
  std::vector<std::vector<int>> state;
  WORKLOAD::generateEdgeState(state, innerVarVec);
  int uniqueVolumn = 0;
  int totalVolumn = 0;
  int stateNum = state.size();
  int innerVarNum = innerVarVec.size();
  if (stateNum == 0) {
    compOneStateVolumn(uniqueVolumn, totalVolumn, innerTimeVec, dataType);
  } else {
    for (int i = 0; i < stateNum; i++) {
      changeEdgeByState(1, innerVarNum, i, state, innerVarVec);
      compOneStateVolumn(uniqueVolumn, totalVolumn, innerTimeVec, dataType);
      changeEdgeByState(0, innerVarNum, i, state, innerVarVec);
    }
  }
  uniqueVolumn *= outerTimeSize;
  totalVolumn *= outerTimeSize;
  _result->totalVolumn[dataType] += totalVolumn;
  _result->uniqueVolumn[dataType] += uniqueVolumn;
  _result->reuseVolumn[dataType] += totalVolumn - uniqueVolumn;
}

void Analyzer::generateVarVec(
    std::vector<int> &timeVec,
    std::vector<std::shared_ptr<WORKLOAD::Iterator>> &varVec) {
  int colNumT = _T.getColNum();
  for (auto row : timeVec) {
    for (int i = 0; i < colNumT; i++) {
      if (_T(row, i) == 1)
        varVec.push_back(_coupledVarVec[i]);
    }
  }
}

bool Analyzer::checkValidInnerDim(int varIndex, ARCH::DATATYPE dataType) {
  std::shared_ptr<std::vector<std::vector<int>>> reuseVecSet =
      _reuseVecMap[dataType];
  int m = (*reuseVecSet).size();
  int n = (*reuseVecSet)[0].size();
  for (auto reuseVecItem : (*reuseVecSet)) {
    int flag = 1;
    for (int j = 0; j < n; j++) {
      if (j == varIndex) {
        if (reuseVecItem[j] != 1) {
          flag = 0;
        }
      } else {
        if (reuseVecItem[j] == 1) {
          flag = 0;
        }
      }
    }
    if (flag) {
      return true;
    }
  }
  return false;
}
void Analyzer::constructSingleDataTypeTimeSet(int flag,
                                              std::set<int> &singleDataTypeSet,
                                              ARCH::DATATYPE dataType) {
  int coupleVarNum = _coupledVarVec.size();
  if (flag) {
    for (int i = 2; i < coupleVarNum; i++) {
      if (checkValidInnerDim(i, dataType)) {
        singleDataTypeSet.insert(i);
      } else {
        break;
      }
    }
  } else {
    for (int i = 2; i < coupleVarNum; i++) {
      singleDataTypeSet.insert(i);
    }
  }
}

void Analyzer::constructInnerOuterTimeVec(std::vector<int> &innerTimeVec,
                                          std::vector<int> &outerTimeVec) {
  int coupleVarNum = _coupledVarVec.size();
  int inputIfStationary = _L.checkIfStationary(ARCH::INPUT);
  int weightIfStationary = _L.checkIfStationary(ARCH::WEIGHT);
  int outputIfStationary = _L.checkIfStationary(ARCH::OUTPUT);
  std::set<int> inputSet;
  std::set<int> weightSet;
  std::set<int> outputSet;
  constructSingleDataTypeTimeSet(inputIfStationary, inputSet, ARCH::INPUT);
  constructSingleDataTypeTimeSet(weightIfStationary, weightSet, ARCH::WEIGHT);
  constructSingleDataTypeTimeSet(outputIfStationary, outputSet, ARCH::OUTPUT);
  std::set<int> innerTimeSet;
  std::set<int> outerTimeSet;
  for (int i = 2; i < coupleVarNum; i++) {
    if (inputSet.count(i) && weightSet.count(i) && outputSet.count(i)) {
      innerTimeSet.insert(i);
    } else {
      break;
    }
  }

  for (int i = 2; i < coupleVarNum; i++) {
    if (!innerTimeSet.count(i)) {
      outerTimeSet.insert(i);
    }
  }
  for (auto it = innerTimeSet.rbegin(); it != innerTimeSet.rend(); it++) {
    innerTimeVec.push_back(*it);
  }
  for (auto it = outerTimeSet.rbegin(); it != outerTimeSet.rend(); it++) {
    outerTimeVec.push_back(*it);
  }
}

int Analyzer::compOneStateTimeSize(std::vector<int> &timeVec) {
  std::pair<int, int> timeRange;
  int timeSize = 1;
  for (auto timeIndex : timeVec) {
    timeRange = compTRange(timeIndex);
    timeSize *= timeRange.second - timeRange.first + 1;
  }
  return timeSize;
}

void Analyzer::compRequiredDataSize() {
  _result->requiredDataSize[ARCH::OUTPUT] = _O.getVolumn();
  _result->requiredDataSize[ARCH::INPUT] = _I.getVolumn();
  _result->requiredDataSize[ARCH::WEIGHT] = _W.getVolumn();
}

int Analyzer::compTotalBandWidth(ARCH::DATATYPE dataType) {
  return _L.getBufferBandWidth(dataType);
}

std::shared_ptr<Result> Analyzer::getResult() { return _result; }
int Analyzer::getOccTimes() {
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> varVec;
  for (auto &var : _coupledVarVec) {
    if (!_curSubCoupledVarSet.count(var)) {
      varVec.push_back(var);
    }
  }
  std::vector<std::vector<int>> state;
  WORKLOAD::generateEdgeState(state, varVec);
  int stateNum = state.size();
  int varNum = varVec.size();

  std::vector<int> timeVec;
  int TColNum = _T.getColNum();
  for (int i = 2; i < TColNum; i++) {
    timeVec.push_back(i);
  }
  int ret = 0;
  if (stateNum == 0) {
    PEX->lock();
    PEY->lock();
    ret += compOneStateTimeSize(timeVec);
    PEX->unlock();
    PEY->unlock();
  } else {
    for (int i = 0; i < stateNum; i++) {
      changeEdgeByState(1, varNum, i, state, varVec);
      std::pair<int, int> PEXRange = compTRange(0);
      std::pair<int, int> PEYRange = compTRange(1);
      PEX->lock();
      PEY->lock();
      ret += compOneStateTimeSize(timeVec) *
             (PEYRange.second - PEYRange.first + 1) *
             (PEXRange.second - PEXRange.first + 1);
      PEX->unlock();
      PEY->unlock();
      changeEdgeByState(0, varNum, i, state, varVec);
    }
  }
  return ret;
}
void Analyzer::setSubLevelResultVec(
    std::vector<std::shared_ptr<Result>> &subLevelResultVec) {
  _result->subLevelResultVec = subLevelResultVec;
}
void Analyzer::changeEdgeByState(
    bool flag, int varNum, int i, std::vector<std::vector<int>> &state,
    std::vector<std::shared_ptr<WORKLOAD::Iterator>> &varVec) {
  for (int j = 0; j < varNum; j++) {
    if (state[i][j]) {
      if (flag) {
        setEdge(varVec[j]);
      } else {
        unsetEdge(varVec[j]);
      }
    }
  }
}
void Analyzer::setCurSubCoupledVarVec(
    std::vector<std::shared_ptr<WORKLOAD::Iterator>> curSubCoupledVarVec) {
  _curSubCoupledVarVec = curSubCoupledVarVec;
  for (auto &item : _curSubCoupledVarVec) {
    _curSubCoupledVarSet.insert(item);
  }
}

void Analyzer::setBase(std::vector<Base> baseSet) {
  _baseSet = baseSet;
  _curBaseIndex = 0;
}

std::vector<std::shared_ptr<WORKLOAD::Iterator>> &Analyzer::getCoupledVarVec() {
  return _coupledVarVec;
}