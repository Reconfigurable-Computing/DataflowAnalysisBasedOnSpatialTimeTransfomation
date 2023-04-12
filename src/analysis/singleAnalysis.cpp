#include "include/analysis/singleAnalysis.h"

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
  getComputationDelay();
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
      for (int j = 0; j < varNum; j++) {
        if (state[i][j]) {
          setEdge(outerVarVec[j]);
        }
      }
      delayAnalysis(innerTimeVec, outerTimeVec);
      accessAnalysis(innerTimeVec, outerTimeVec);
      for (int j = 0; j < varNum; j++) {
        if (state[i][j]) {
          unsetEdge(outerVarVec[j]);
        }
      }
    }
  }
}

void Analyzer::delayAnalysis(std::vector<int> &innerTimeVec,
                             std::vector<int> &outerTimeVec) {
  int outerTimeSize = compOneStateTimeSize(outerTimeVec);

  std::pair<int, int> PEXRange = compTRange(0);
  std::pair<int, int> PEYRange = compTRange(1);
  if (PEX->hasEdge())
    PEXRange.second = PEXRange.second + 1;
  if (PEY->hasEdge())
    PEYRange.second = PEYRange.second + 1;
  int inputInitDelay = 0;
  int weightInitDelay = 0;
  int outputOutDelay = 0;
  int coupleVarNum = _coupledVarVec.size();
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> innerVarVec;
  generateVarVec(innerTimeVec, innerVarVec);
  std::vector<std::vector<int>> state;
  WORKLOAD::generateEdgeState(state, innerVarVec);
  int delay = 0;

  int stateNum = state.size();
  int innerVarNum = innerVarVec.size();
  int ret = 0;
  if (stateNum == 0) {
    delay += compOneStateStableDelay(innerTimeVec);
    inputInitDelay = std::max(
        inputInitDelay,
        _L.getInitOrOutDelay(ARCH::INPUT, _baseSet[_curBaseIndex].baseData[0],
                             16, PEXRange, PEYRange));
    weightInitDelay = std::max(
        weightInitDelay,
        _L.getInitOrOutDelay(ARCH::WEIGHT, _baseSet[_curBaseIndex].baseData[1],
                             16, PEXRange, PEYRange));
    outputOutDelay = std::max(
        outputOutDelay,
        _L.getInitOrOutDelay(ARCH::OUTPUT, _baseSet[_curBaseIndex].baseData[2],
                             16, PEXRange, PEYRange));
  } else {
    for (int i = 0; i < stateNum; i++) {
      for (int j = 0; j < innerVarNum; j++) {
        if (state[i][j]) {
          setEdge(innerVarVec[j]);
        }
      }
      delay += compOneStateStableDelay(innerTimeVec);
      inputInitDelay = std::max(
          inputInitDelay,
          _L.getInitOrOutDelay(ARCH::INPUT, _baseSet[_curBaseIndex].baseData[0],
                               16, PEXRange, PEYRange));
      weightInitDelay =
          std::max(weightInitDelay,
                   _L.getInitOrOutDelay(ARCH::WEIGHT,
                                        _baseSet[_curBaseIndex].baseData[1], 16,
                                        PEXRange, PEYRange));
      outputOutDelay =
          std::max(outputOutDelay,
                   _L.getInitOrOutDelay(ARCH::OUTPUT,
                                        _baseSet[_curBaseIndex].baseData[2], 16,
                                        PEXRange, PEYRange));

      for (int j = 0; j < innerVarNum; j++) {
        if (state[i][j]) {
          unsetEdge(innerVarVec[j]);
        }
      }
    }
  }
  if (_lowestFlag || !_doubleBufferFlag) {
    delay +=
        std::max(std::max(inputInitDelay, weightInitDelay), outputOutDelay);
  } else // doublebuffer
  {
    delay = std::max(
        std::max(std::max(inputInitDelay, weightInitDelay), outputOutDelay),
        delay);
  }
  _result->delay += outerTimeSize * delay;
}

void Analyzer::accessAnalysis(std::vector<int> &innerTimeVec,
                              std::vector<int> &outerTimeVec) {
  PEX->lock();
  PEY->lock();
  int outerTimeSize = compOneStateTimeSize(outerTimeVec);
  PEX->unlock();
  PEY->unlock();
  oneStateAccessAnalysis(innerTimeVec, outerTimeSize);
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

int Analyzer::getActivePENum() {
  std::pair<int, int> PEXRange = compTRange(0);
  std::pair<int, int> PEYRange = compTRange(1);
  return (PEYRange.second - PEYRange.first + 1) *
         (PEXRange.second - PEXRange.first + 1);
}

int Analyzer::getComputationDelay() {
  int TDimNum = _T.getColNum();
  auto Tmatrix = _T.getMatrix();
  int totalActiveNum = 1;
  for (auto var : _coupledVarVec) {
    totalActiveNum *= var->getSize();
  }
  int activePENum = getActivePENum();

  return totalActiveNum / activePENum;
}

void Analyzer::compOneStatePerPEVolumn(int &perPEUniqueVolumn, int &perPEVolumn,
                                       std::vector<int> &innerTimeVec,
                                       ARCH::DATATYPE dataType) {
  PEX->lock();
  PEY->lock();

  int timeSize = compOneStateTimeSize(innerTimeVec);
  perPEVolumn += timeSize * _baseSet[_curBaseIndex].baseData[dataType];
  if (_L.checkIfStationary(dataType)) {
    perPEUniqueVolumn =
        std::max(_baseSet[_curBaseIndex].baseData[dataType], perPEUniqueVolumn);
  } else {
    perPEUniqueVolumn = perPEVolumn;
  }
  PEX->unlock();
  PEY->unlock();
}

void Analyzer::compInnerUniqueAccess(ARCH::DATATYPE dataType,
                                     MAPPING::Access &access,
                                     std::vector<int> &innerTimeVec,
                                     int outerTimeSize) {
  ARCH::NETWORKTYPE networkType = _L.getNetworkType(dataType);
  std::pair<int, int> PEXRange = compTRange(0);
  std::pair<int, int> PEYRange = compTRange(1);
  if (PEX->hasEdge())
    PEXRange.second = PEXRange.second + 1;
  if (PEY->hasEdge())
    PEYRange.second = PEYRange.second + 1;

  std::vector<std::shared_ptr<WORKLOAD::Iterator>> innerVarVec;
  generateVarVec(innerTimeVec, innerVarVec);
  std::vector<std::vector<int>> state;
  WORKLOAD::generateEdgeState(state, innerVarVec);
  int stateNum = state.size();
  int innerVarNum = innerVarVec.size();
  int perPEUniqueVolumn = 0;
  int perPEVolumn = 0;
  if (stateNum == 0) {
    compOneStatePerPEVolumn(perPEUniqueVolumn, perPEVolumn, innerTimeVec,
                            dataType);
  } else {
    for (int i = 0; i < stateNum; i++) {
      for (int j = 0; j < innerVarNum; j++) {
        if (state[i][j]) {
          setEdge(innerVarVec[j]);
        }
      }
      compOneStatePerPEVolumn(perPEUniqueVolumn, perPEVolumn, innerTimeVec,
                              dataType);
      for (int j = 0; j < innerVarNum; j++) {
        if (state[i][j]) {
          unsetEdge(innerVarVec[j]);
        }
      }
    }
  }
  int uniqueVolumn;
  if (networkType == ARCH::SYSTOLIC || networkType == ARCH::MULTICAST) {
    int activeAccessPointNum =
        _L.getActiveAccessPointNum(dataType, PEXRange, PEYRange);
    uniqueVolumn = perPEUniqueVolumn * activeAccessPointNum;

  } else {
    uniqueVolumn = (PEYRange.second - PEYRange.first + 1) *
                   (PEXRange.second - PEXRange.first + 1) * perPEUniqueVolumn;
  }
  uniqueVolumn *= outerTimeSize;
  int totalVolumn = (PEYRange.second - PEYRange.first + 1) *
                    (PEXRange.second - PEXRange.first + 1) * perPEVolumn *
                    outerTimeSize;
  _result->totalVolumn[dataType] += totalVolumn;
  _result->uniqueVolumn[dataType] += uniqueVolumn;
  _result->reuseVolumn[dataType] += totalVolumn - uniqueVolumn;
}

void Analyzer::oneStateAccessAnalysis(std::vector<int> &innerTimeVec,
                                      int outerTimeSize) {
  compInnerUniqueAccess(ARCH::OUTPUT, _accessO, innerTimeVec, outerTimeSize);
  compInnerUniqueAccess(ARCH::INPUT, _accessI, innerTimeVec, outerTimeSize);
  compInnerUniqueAccess(ARCH::WEIGHT, _accessW, innerTimeVec, outerTimeSize);
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

int Analyzer::compOneStateStableDelay(std::vector<int> innerTimeVec) {

  int inputStableDelay =
      _L.getStableDelay(ARCH::INPUT, _baseSet[_curBaseIndex].baseData[0], 16);
  int weightStableDelay =
      _L.getStableDelay(ARCH::WEIGHT, _baseSet[_curBaseIndex].baseData[1], 16);
  int outputStableDelay =
      _L.getStableDelay(ARCH::OUTPUT, _baseSet[_curBaseIndex].baseData[2], 16);

  int stableDelay =
      std::max(std::max(std::max(inputStableDelay, weightStableDelay),
                        outputStableDelay),
               _baseSet[_curBaseIndex].baseCompCycle);
  int timeSize = compOneStateTimeSize(innerTimeVec);
  if (_lowestFlag || !_doubleBufferFlag) {
    return timeSize * stableDelay;
  } else // doublebuffer
  {
    return timeSize * stableDelay;
  }
}
