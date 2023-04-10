#include "include/analysis/singleAnalysis.h"

void Analyzer::oneAnalysis() {
  _result.reset();
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
          outerVarVec[j]->setEdge();
        }
      }
      delayAnalysis(innerTimeVec, outerTimeVec);
      accessAnalysis(innerTimeVec, outerTimeVec);
      for (int j = 0; j < varNum; j++) {
        if (state[i][j]) {
          outerVarVec[j]->unsetEdge();
        }
      }
    }
  }
}

void Analyzer::delayAnalysis(std::vector<int> &innerTimeVec,
                             std::vector<int> &outerTimeVec) {
  int outerTimeSize = compOneStateTimeSize(outerTimeVec);
  _result.delay += outerTimeSize * compOneStateStableDelay(innerTimeVec);
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
int Analyzer::compPerPEVolumn(std::vector<int> &innerTimeVec) {
  PEX->lock();
  PEY->lock();
  int ret = compTimeSize(innerTimeVec);
  PEX->unlock();
  PEY->unlock();
  return ret;
}
void Analyzer::compInnerUniqueAccess(ARCH::DATATYPE dataType,
                                     MAPPING::Access &access,
                                     std::vector<int> &innerTimeVec,
                                     int outerTimeSize) {
  ARCH::NETWORKTYPE networkType = _L.getNetworkType(dataType);
  std::pair<int, int> PEXRange = compTRange(0);
  std::pair<int, int> PEYRange = compTRange(1);
  int perPEVolumn = compPerPEVolumn(innerTimeVec);
  perPEVolumn *= _result.base.baseData[dataType];
  int perPEUniqueVolumn;
  if (_L.checkIfStationary(dataType)) {
    perPEUniqueVolumn = _result.base.baseData[dataType];
  } else {
    perPEUniqueVolumn = perPEVolumn;
  }
  int uniqueVolumn;
  if (networkType == ARCH::SYSTOLIC || networkType == ARCH::MULTICAST) {
    int activeAccessPointNum =
        _L.getActiveAccessPointNum(dataType, PEXRange, PEYRange);
    ;
    uniqueVolumn = perPEUniqueVolumn * activeAccessPointNum;

  } else {
    uniqueVolumn = (PEYRange.second - PEYRange.first + 1) *
                   (PEXRange.second - PEXRange.first + 1) * perPEUniqueVolumn;
  }
  uniqueVolumn *= outerTimeSize;
  int totalVolumn = (PEYRange.second - PEYRange.first + 1) *
                    (PEXRange.second - PEXRange.first + 1) * perPEVolumn *
                    outerTimeSize;
  _result.totalVolumn[dataType] += totalVolumn;
  _result.uniqueVolumn[dataType] += uniqueVolumn;
  _result.reuseVolumn[dataType] += totalVolumn - uniqueVolumn;
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

int Analyzer::compTimeSize(std::vector<int> &timeVec) {
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> innerVarVec;
  generateVarVec(timeVec, innerVarVec);
  std::vector<std::vector<int>> state;
  WORKLOAD::generateEdgeState(state, innerVarVec);

  int stateNum = state.size();
  int innerVarNum = innerVarVec.size();
  int ret = 0;
  if (stateNum == 0)
    return 1;
  for (int i = 0; i < stateNum; i++) {
    for (int j = 0; j < innerVarNum; j++) {
      if (state[i][j]) {
        innerVarVec[j]->setEdge();
      }
    }

    ret += compOneStateTimeSize(timeVec);

    for (int j = 0; j < innerVarNum; j++) {
      if (state[i][j]) {
        innerVarVec[j]->unsetEdge();
      }
    }
  }
  return ret;
}

int Analyzer::compOneStateStableDelay(std::vector<int> innerTimeVec) {
  std::pair<int, int> PEXRange = compTRange(0);
  std::pair<int, int> PEYRange = compTRange(1);
  int inputStableDelay =
      _L.getStableDelay(ARCH::INPUT, _result.base.baseData[0], 16);
  int weightStableDelay =
      _L.getStableDelay(ARCH::WEIGHT, _result.base.baseData[1], 16);
  int outputStableDelay =
      _L.getStableDelay(ARCH::OUTPUT, _result.base.baseData[2], 16);
  int inputInitDelay = _L.getInitOrOutDelay(
      ARCH::INPUT, _result.base.baseData[0], 16, PEXRange, PEYRange);
  int weightInitDelay = _L.getInitOrOutDelay(
      ARCH::WEIGHT, _result.base.baseData[1], 16, PEXRange, PEYRange);
  int outputOutDelay = _L.getInitOrOutDelay(
      ARCH::OUTPUT, _result.base.baseData[2], 16, PEXRange, PEYRange);
  int stableDelay =
      std::max(std::max(std::max(inputStableDelay, weightStableDelay),
                        outputStableDelay),
               _result.base.baseCompCycle);
  int coupleVarNum = _coupledVarVec.size();
  int innerTimeSize = compTimeSize(innerTimeVec);
  if (_lowestFlag || !_doubleBufferFlag) {
    return innerTimeSize * stableDelay +
           std::max(std::max(inputInitDelay, weightInitDelay), outputOutDelay);
  } else // doublebuffer
  {
    return std::max(
        std::max(std::max(inputInitDelay, weightInitDelay), outputOutDelay),
        innerTimeSize * stableDelay);
  }
}
