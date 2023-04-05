#include "include/analysis/singleAnalysis.h"

void Analyzer::oneAnalysis() {
  Result result;
  getComputationDelay();
  accessAnalysis(result);
  _result.delay += getStableDelay(1);
}

Analyzer::Analyzer(
    std::vector<std::shared_ptr<WORKLOAD::Iterator>> &coupledVarVec,
    std::vector<std::shared_ptr<WORKLOAD::Iterator>> &lowerVarVec,
    MAPPING::Transform &T, int baseCycle, int baseData[3], WORKLOAD::Tensor &I,
    WORKLOAD::Tensor &W, WORKLOAD::Tensor &O, ARCH::Level &L1,
    MAPPING::Access &accessI, MAPPING::Access &accessW,
    MAPPING::Access &accessO,
    std::shared_ptr<std::vector<std::vector<int>>> reuseVecI,
    std::shared_ptr<std::vector<std::vector<int>>> reuseVecW,
    std::shared_ptr<std::vector<std::vector<int>>> reuseVecO)
    : _coupledVarVec(coupledVarVec), _T(T), _I(I), _W(W), _O(O), _L1(L1),
      _accessI(accessI), _accessW(accessW), _accessO(accessO),
      _lowerVarVec(lowerVarVec), _reuseVecI(reuseVecI), _reuseVecO(reuseVecO),
      _reuseVecW(reuseVecW) {
  int colNum = _T.getColNum();
  for (int i = 0; i < colNum; i++) {
    if (_T(0, i) == 1)
      PEX = _coupledVarVec[i];
    if (_T(1, i) == 1)
      PEY = _coupledVarVec[i];
  }
  _result.baseCycle = baseCycle;
  for (int i = 0; i < 3; i++) {
    _result.baseData[i] = baseData[i];
  }
  _reuseVecMap[ARCH::INPUT] = _reuseVecI;
  _reuseVecMap[ARCH::WEIGHT] = _reuseVecW;
  _reuseVecMap[ARCH::OUTPUT] = _reuseVecO;

  int coupleVarNum = _coupledVarVec.size();
}

std::pair<int, int> Analyzer::getTRange(int row) {
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
  std::pair<int, int> PEXRange = getTRange(0);
  std::pair<int, int> PEYRange = getTRange(1);
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
int Analyzer::getPerPEVolumn(std::vector<int> &innerTimeVec) {
  PEX->lock();
  PEY->lock();
  int ret = getTimeSize(innerTimeVec);
  PEX->unlock();
  PEY->unlock();
  return ret;
}
void Analyzer::getInnerUniqueAccess(ARCH::DATATYPE dataType,
                                    MAPPING::Access &access,
                                    std::vector<int> &innerTimeVec,
                                    Result &result, int outerTimeSize) {
  ARCH::NETWORKTYPE networkType = _L1.getNetworkType(dataType);
  std::pair<int, int> PEXRange = getTRange(0);
  std::pair<int, int> PEYRange = getTRange(1);
  int perPEVolumn = getPerPEVolumn(innerTimeVec);
  int uniqueVolumn;
  if (networkType == ARCH::SYSTOLIC || networkType == ARCH::MULTICAST) {

    std::vector<std::pair<int, int>> accessPointSet;
    _L1.getAccessPoint(dataType, accessPointSet);
    int activeAccessPointNum = 0;
    for (auto accessPoint : accessPointSet) {
      if (accessPoint.first >= PEXRange.first &&
          accessPoint.first <= PEXRange.second &&
          accessPoint.second >= PEYRange.first &&
          accessPoint.second <= PEYRange.second) {
        activeAccessPointNum++;
      }
    }
    uniqueVolumn = perPEVolumn * activeAccessPointNum;

  } else if (networkType == ARCH::STATIONARY) {
    uniqueVolumn = (PEYRange.second - PEYRange.first + 1) *
                   (PEXRange.second - PEXRange.first + 1) * 1;

  } else {
    uniqueVolumn = (PEYRange.second - PEYRange.first + 1) *
                   (PEXRange.second - PEXRange.first + 1) * perPEVolumn;
  }
  uniqueVolumn *= outerTimeSize;
  int totalVolumn = (PEYRange.second - PEYRange.first + 1) *
                    (PEXRange.second - PEXRange.first + 1) * perPEVolumn *
                    outerTimeSize;
  result.totalVolumn[dataType] += totalVolumn;
  result.uniqueVolumn[dataType] += uniqueVolumn;
  result.reuseVolumn[dataType] += totalVolumn - uniqueVolumn;
}

void Analyzer::oneStateAccessAnalysis(Result &result,
                                      std::vector<int> &innerTimeVec,
                                      int outerTimeSize) {

  getInnerUniqueAccess(ARCH::OUTPUT, _accessO, innerTimeVec, result,
                       outerTimeSize);
  getInnerUniqueAccess(ARCH::INPUT, _accessI, innerTimeVec, result,
                       outerTimeSize);
  getInnerUniqueAccess(ARCH::WEIGHT, _accessW, innerTimeVec, result,
                       outerTimeSize);
}
void Analyzer::generateVarIndexVec(std::vector<int> &timeVec,
                                   std::vector<int> &varIndexVec) {
  int colNumT = _T.getColNum();
  for (auto row : timeVec) {
    for (int i = 0; i < colNumT; i++) {
      if (_T(row, i) == 1)
        varIndexVec.push_back(i);
    }
  }
}
void Analyzer::accessAnalysis(Result &result) {
  std::vector<int> innerTimeVec;
  std::vector<int> outerTimeVec;
  getInnerOuterTimeVec(innerTimeVec, outerTimeVec);

  std::vector<int> outerVarIndexVec;
  generateVarIndexVec(outerTimeVec, outerVarIndexVec);
  std::vector<std::vector<int>> state;
  generateEdgeState(state, outerVarIndexVec);

  int stateNum = state.size();
  int innerVarNum = outerVarIndexVec.size();
  if (stateNum == 0) {
    int outerTimeSize = 1;
    oneStateAccessAnalysis(result, innerTimeVec, outerTimeSize);
  } else {
    for (int i = 0; i < stateNum; i++) {
      for (int j = 0; j < innerVarNum; j++) {
        if (state[i][j]) {
          _coupledVarVec[outerVarIndexVec[j]]->setEdge();
        }
      }
      int outerTimeSize = getOneStateTimeSize(outerTimeVec);
      oneStateAccessAnalysis(result, innerTimeVec, outerTimeSize);
      for (int j = 0; j < innerVarNum; j++) {
        if (state[i][j]) {
          _coupledVarVec[outerVarIndexVec[j]]->unsetEdge();
        }
      }
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
void Analyzer::constructSingleDataTypeSet(int flag,
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

void Analyzer::getInnerOuterTimeVec(std::vector<int> &innerTimeVec,
                                    std::vector<int> &outerTimeVec) {
  int coupleVarNum = _coupledVarVec.size();
  int inputIfStationary = _L1.checkIfStationary(ARCH::INPUT);
  int weightIfStationary = _L1.checkIfStationary(ARCH::WEIGHT);
  int outputIfStationary = _L1.checkIfStationary(ARCH::OUTPUT);
  std::set<int> inputSet;
  std::set<int> weightSet;
  std::set<int> outputSet;
  constructSingleDataTypeSet(inputIfStationary, inputSet, ARCH::INPUT);
  constructSingleDataTypeSet(weightIfStationary, weightSet, ARCH::WEIGHT);
  constructSingleDataTypeSet(outputIfStationary, outputSet, ARCH::OUTPUT);
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
void Analyzer::generateEdgeState(std::vector<std::vector<int>> &state,
                                 std::vector<int> varIndexVec) {
  for (auto varIndex : varIndexVec) {
    int len = state.size();
    if (_coupledVarVec[varIndex]->hasEdge()) {
      if (len == 0) {
        state.push_back({0});
        state.push_back({1});
      } else {
        for (int i = 0; i < len; i++) {
          state.push_back(state[i]);
        }
        for (int i = 0; i < len; i++) {
          state[i].push_back(1);
        }
        for (int i = len; i < 2 * len; i++) {
          state[i].push_back(0);
        }
      }
    } else {
      if (len == 0) {
        state.push_back({0});
      } else {
        for (int i = 0; i < len; i++) {
          state[i].push_back(0);
        }
      }
    }
  }
}

int Analyzer::getOneStateTimeSize(std::vector<int> &timeVec) {
  std::pair<int, int> timeRange;
  int timeSize = 1;
  for (auto timeIndex : timeVec) {
    timeRange = getTRange(timeIndex);
    timeSize *= timeRange.second - timeRange.first + 1;
  }
  return timeSize;
}

int Analyzer::getTimeSize(std::vector<int> &timeVec) {
  std::vector<int> innerVarIndexVec;
  generateVarIndexVec(timeVec, innerVarIndexVec);
  std::vector<std::vector<int>> state;
  generateEdgeState(state, innerVarIndexVec);

  int stateNum = state.size();
  int innerVarNum = innerVarIndexVec.size();
  int ret = 0;
  for (int i = 0; i < stateNum; i++) {
    for (int j = 0; j < innerVarNum; j++) {
      if (state[i][j]) {
        _coupledVarVec[innerVarIndexVec[j]]->setEdge();
      }
    }

    ret += getOneStateTimeSize(timeVec);

    for (int j = 0; j < innerVarNum; j++) {
      if (state[i][j]) {
        _coupledVarVec[innerVarIndexVec[j]]->unsetEdge();
      }
    }
  }
  return ret;
}

bool Analyzer::checkThasEdge(int row) {
  int colNumT = _T.getColNum();
  for (int i = 0; i < colNumT; i++) {
    if (_T(row, i) == 1 && _coupledVarVec[i]->hasEdge())
      return true;
  }
  return false;
}

int Analyzer::getStableDelay(int compDelay) {
  int doubleBufferFlag = 1;
  int lowestFlag = 1;
  std::pair<int, int> PEXRange = getTRange(0);
  std::pair<int, int> PEYRange = getTRange(1);
  int inputStableDelay = _L1.getStableDelay(ARCH::INPUT, 1, 16);
  int weightStableDelay = _L1.getStableDelay(ARCH::WEIGHT, 1, 16);
  int outputStableDelay = _L1.getStableDelay(ARCH::OUTPUT, 1, 16);
  int inputInitDelay =
      _L1.getInitOrOutDelay(ARCH::INPUT, 1, 16, PEXRange, PEYRange);
  int weightInitDelay =
      _L1.getInitOrOutDelay(ARCH::WEIGHT, 1, 16, PEXRange, PEYRange);
  int outputOutDelay =
      _L1.getInitOrOutDelay(ARCH::OUTPUT, 1, 16, PEXRange, PEYRange);
  int stableDelay =
      std::max(std::max(std::max(inputStableDelay, weightStableDelay),
                        outputStableDelay),
               compDelay);
  int coupleVarNum = _coupledVarVec.size();
  std::vector<int> innerTimeVec;
  std::vector<int> outerTimeVec;
  getInnerOuterTimeVec(innerTimeVec, outerTimeVec);
  int innerTimeSize = getTimeSize(innerTimeVec);
  int outerTimeSize = getTimeSize(outerTimeVec);
  int delay;
  if (lowestFlag == 1 || doubleBufferFlag == 0) {
    delay = innerTimeSize * stableDelay +
            std::max(std::max(inputInitDelay, weightInitDelay), outputOutDelay);
  } else // doublebuffer
  {
    delay = std::max(
        std::max(std::max(inputInitDelay, weightInitDelay), outputOutDelay),
        innerTimeSize * stableDelay);
  }
  delay *= outerTimeSize;
  return delay;
}

void Analyzer::compSubTensorSize() {}
