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
  _result = std::make_shared<AnalyzerResult>(AnalyzerResult());
  _result->requiredDataSize[ARCH::INPUT] = _requiredDataSize[ARCH::INPUT];
  _result->requiredDataSize[ARCH::WEIGHT] = _requiredDataSize[ARCH::WEIGHT];
  _result->requiredDataSize[ARCH::OUTPUT] = _requiredDataSize[ARCH::OUTPUT];
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
  long long initDelay = 0;
  long long delay = 0;
  long long compCycle = 0;
  long long activePEMultTimeNum = 0;
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
  if (0) {
    // delay = std::max(delay, initDelay);
    //_result->delay += ((long long)outerTimeSize - 1) * delay + initDelay;
    //_result->compCycle += (long long)outerTimeSize * compCycle;
    //_result->activePEMultTimeNum += (long long)outerTimeSize *
    //activePEMultTimeNum; _result->totalPEMultTimeNum += (((long
    //long)outerTimeSize - 1) * delay + initDelay) * _L.getPENum();
    //_result->initTimes += outerTimeSize;
  } else {
    delay += initDelay;
    _result->delay += (long long)outerTimeSize * delay;
    _result->compCycle += (long long)outerTimeSize * compCycle;
    _result->activePEMultTimeNum +=
        (long long)outerTimeSize * activePEMultTimeNum;
    _result->totalPEMultTimeNum +=
        (long long)outerTimeSize * delay * _L.getPENum();
    _result->initTimes += outerTimeSize;
  }
}

int Analyzer::compStableVolumn(ARCH::DATATYPE dataType,
                               int innerCoupledDimIndex, int coef) {
  int ret;
  if (innerCoupledDimIndex == -1 ||
      coef >= _baseSet[_curBaseIndex].baseData[dataType][innerCoupledDimIndex])
    ret = std::accumulate(_baseSet[_curBaseIndex].baseData[dataType].begin(),
                          _baseSet[_curBaseIndex].baseData[dataType].end(), 1,
                          std::multiplies<long long>());
  else {
    ret = 1;
    for (int i = 0; i < _baseSet[_curBaseIndex].baseData[dataType].size(); i++)
      if (i != innerCoupledDimIndex)
        ret *= _baseSet[_curBaseIndex].baseData[dataType][i];
      else
        ret *= coef;
  }
  return ret;
}

int Analyzer::compOneStateStableDelay() {
  int innerCoupledDimIndexInput = _I.getCoupledDimIndex(INNERTIME);
  int coefInput = _I.getCoupledDimCoef(INNERTIME, innerCoupledDimIndexInput);
  int innerCoupledDimIndexWeight = _W.getCoupledDimIndex(INNERTIME);
  int coefWeight = _W.getCoupledDimCoef(INNERTIME, innerCoupledDimIndexWeight);
  int innerCoupledDimIndexOutput = _O.getCoupledDimIndex(INNERTIME);
  int coefOutput = _O.getCoupledDimCoef(INNERTIME, innerCoupledDimIndexOutput);
  std::pair<int, int> PEXRange = compTRange(0);
  std::pair<int, int> PEYRange = compTRange(1);
  long long inputStableDelay = _L.getStableDelay(
      ARCH::INPUT,
      compStableVolumn(ARCH::INPUT, innerCoupledDimIndexInput, coefInput),
      PEXRange, PEYRange);
  long long weightStableDelay = _L.getStableDelay(
      ARCH::WEIGHT,
      compStableVolumn(ARCH::WEIGHT, innerCoupledDimIndexWeight, coefWeight),
      PEXRange, PEYRange);
  long long outputStableDelay = _L.getStableDelay(
      ARCH::OUTPUT,
      compStableVolumn(ARCH::OUTPUT, innerCoupledDimIndexOutput, coefOutput),
      PEXRange, PEYRange);
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
  long long inputAndWeightStableDelay;
  if (_L.ifInputWeightSharedBW())
    inputAndWeightStableDelay = inputStableDelay + weightStableDelay;
  else
    inputAndWeightStableDelay = std::max(inputStableDelay, weightStableDelay);
  if (_doubleBufferFlag) {
    return std::max(std::max(inputAndWeightStableDelay, outputStableDelay),
                    _baseSet[_curBaseIndex].baseCompCycle);
  } else {
    return std::max(inputAndWeightStableDelay, outputStableDelay) +
           _baseSet[_curBaseIndex].baseCompCycle;
  }
}

long long Analyzer::compOneStateInitDelay(std::pair<int, int> &PEXRange,
                                          std::pair<int, int> &PEYRange) {

  long long inputInitDelay = _L.getInitOrOutDelay(
      ARCH::INPUT,
      std::accumulate(_baseSet[_curBaseIndex].baseData[ARCH::INPUT].begin(),
                      _baseSet[_curBaseIndex].baseData[ARCH::INPUT].end(), 1,
                      std::multiplies<long long>()),
      PEXRange, PEYRange);
  long long weightInitDelay = _L.getInitOrOutDelay(
      ARCH::WEIGHT,
      std::accumulate(_baseSet[_curBaseIndex].baseData[ARCH::WEIGHT].begin(),
                      _baseSet[_curBaseIndex].baseData[ARCH::WEIGHT].end(), 1,
                      std::multiplies<long long>()),
      PEXRange, PEYRange);
  long long outputOutDelay = _L.getInitOrOutDelay(
      ARCH::OUTPUT,
      std::accumulate(_baseSet[_curBaseIndex].baseData[ARCH::OUTPUT].begin(),
                      _baseSet[_curBaseIndex].baseData[ARCH::OUTPUT].end(), 1,
                      std::multiplies<long long>()),
      PEXRange, PEYRange);
  if (_curBaseIndex == 0) {
    _result->initDelay[ARCH::INPUT] =
        std::max(_result->initDelay[ARCH::INPUT], inputInitDelay);
    _result->initDelay[ARCH::WEIGHT] =
        std::max(_result->initDelay[ARCH::WEIGHT], weightInitDelay);
    _result->initDelay[ARCH::OUTPUT] =
        std::max(_result->initDelay[ARCH::OUTPUT], outputOutDelay);
  }
  long long inputAndWeightInitDelay;
  if (_L.ifInputWeightSharedBW())
    inputAndWeightInitDelay = inputInitDelay + weightInitDelay;
  else
    inputAndWeightInitDelay = std::max(inputInitDelay, weightInitDelay);
  return inputAndWeightInitDelay + outputOutDelay +
         _baseSet[_curBaseIndex].baseCompCycle;
}

void Analyzer::compOneStateDelay(std::vector<int> &innerTimeVec,
                                 long long &delay, long long &compCycle,
                                 long long &initDelay,
                                 long long &activePEMultTimeNum) {
  std::pair<int, int> PEXRange = compTRange(0);
  std::pair<int, int> PEYRange = compTRange(1);
  delay += (long long)compOneStateStableDelay() *
           ((long long)compOneStateTimeSize(innerTimeVec) - 1);

  PEX->lock();
  PEY->lock();
  long long curCompCycle = (long long)_baseSet[_curBaseIndex].baseCompCycle *
                           compOneStateTimeSize(innerTimeVec);
  compCycle += curCompCycle;
  activePEMultTimeNum += ((long long)PEYRange.second - PEYRange.first + 1) *
                         ((long long)PEXRange.second - PEXRange.first + 1) *
                         curCompCycle;
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
  compOneDataVolumn(ARCH::OUTPUT, _accessO, innerTimeVec, outerTimeSize, _O);
  compOneDataVolumn(ARCH::INPUT, _accessI, innerTimeVec, outerTimeSize, _I);
  compOneDataVolumn(ARCH::WEIGHT, _accessW, innerTimeVec, outerTimeSize, _W);
}

std::pair<long long, long long> Analyzer::compTRange(int row) {
  long long colNumT = _T.getColNum();
  std::shared_ptr<WORKLOAD::Polynomial> dim =
      std::make_shared<WORKLOAD::Polynomial>();
  for (int i = 0; i < colNumT; i++) {
    if (_T(row, i) == 1)
      dim = dim + _coupledVarVec[i];
  }
  std::pair<long long, long long> range = dim->getRange();
  return range;
}

void Analyzer::compOneStateVolumn(long long &uniqueVolumn,
                                  long long &totalVolumn,
                                  std::vector<int> &innerTimeVec,
                                  ARCH::DATATYPE dataType,
                                  WORKLOAD::Tensor &curTensor) {
  std::pair<int, int> PEXRange = compTRange(0);
  std::pair<int, int> PEYRange = compTRange(1);
  long long pexSize = ((long long)PEYRange.second - PEYRange.first + 1);
  long long peySize = ((long long)PEXRange.second - PEXRange.first + 1);
  ARCH::NETWORKTYPE networkType = _L.getNetworkType(dataType);
  PEX->lock();
  PEY->lock();
  totalVolumn +=
      (long long)compOneStateTimeSize(innerTimeVec) *
      std::accumulate(_baseSet[_curBaseIndex].baseData[dataType].begin(),
                      _baseSet[_curBaseIndex].baseData[dataType].end(), 1,
                      std::multiplies<long long>()) *
      pexSize * peySize;
  PEX->unlock();
  PEY->unlock();
  if (_L.checkIfStationary(dataType)) {
    long long baseVolumn =
        std::accumulate(_baseSet[_curBaseIndex].baseData[dataType].begin(),
                        _baseSet[_curBaseIndex].baseData[dataType].end(), 1,
                        std::multiplies<long long>());
    uniqueVolumn = std::max(
        uniqueVolumn,
        baseVolumn * _L.getActiveAccessPointNum(dataType, PEXRange, PEYRange));
  } else {
    PEX->lock();
    PEY->lock();
    if (_L.checkIfUnlockPEDim(0, dataType))
      PEX->unlock();
    if (_L.checkIfUnlockPEDim(1, dataType))
      PEY->unlock();
    int innerCoupledDimIndex = curTensor.getCoupledDimIndex(INNERTIME);
    int coef = curTensor.getCoupledDimCoef(INNERTIME, innerCoupledDimIndex);
    long long baseVolumn =
        std::accumulate(_baseSet[_curBaseIndex].baseData[dataType].begin(),
                        _baseSet[_curBaseIndex].baseData[dataType].end(), 1,
                        std::multiplies<long long>());
    if (innerCoupledDimIndex == -1 ||
        coef >=
            _baseSet[_curBaseIndex].baseData[dataType][innerCoupledDimIndex]) {
      uniqueVolumn += compOneStateTimeSize(innerTimeVec) * baseVolumn *
                      _L.getActiveAccessPointNum(dataType, PEXRange, PEYRange);
    } else {
      std::vector<int> tmpTimeVec;
      tmpTimeVec.push_back(2);
      int innerMostRange = compOneStateTimeSize(tmpTimeVec);
      long long innerMostVolumn = 1;
      for (int i = 0; i < _baseSet[_curBaseIndex].baseData[dataType].size();
           i++) {
        if (i != innerCoupledDimIndex)
          innerMostVolumn *= _baseSet[_curBaseIndex].baseData[dataType][i];
        else
          innerMostVolumn *= coef;
      }
      tmpTimeVec.clear();
      for (auto timeIndex : innerTimeVec)
        if (timeIndex != 2)
          tmpTimeVec.push_back(timeIndex);
      int innerNotMostRange = compOneStateTimeSize(tmpTimeVec);
      long long tmp =
          (long long)baseVolumn +
          (long long)innerMostVolumn * ((long long)innerMostRange - 1);
      // uniqueVolumn += tmp * innerNotMostRange * pexSize * peySize;
      uniqueVolumn += tmp * innerNotMostRange *
                      _L.getActiveAccessPointNum(dataType, PEXRange, PEYRange);
    }
    PEX->unlock();
    PEY->unlock();
  }
}

void Analyzer::compOneDataVolumn(ARCH::DATATYPE dataType,
                                 MAPPING::Access &access,
                                 std::vector<int> &innerTimeVec,
                                 int outerTimeSize,
                                 WORKLOAD::Tensor &curTensor) {
  ARCH::NETWORKTYPE networkType = _L.getNetworkType(dataType);
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> innerVarVec;
  generateVarVec(innerTimeVec, innerVarVec);
  std::vector<std::vector<int>> state;
  WORKLOAD::generateEdgeState(state, innerVarVec);
  long long uniqueVolumn = 0;
  long long totalVolumn = 0;
  int stateNum = state.size();
  int innerVarNum = innerVarVec.size();
  if (stateNum == 0) {
    compOneStateVolumn(uniqueVolumn, totalVolumn, innerTimeVec, dataType,
                       curTensor);
  } else {
    for (int i = 0; i < stateNum; i++) {
      changeEdgeByState(1, innerVarNum, i, state, innerVarVec);
      compOneStateVolumn(uniqueVolumn, totalVolumn, innerTimeVec, dataType,
                         curTensor);
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
  std::pair<long long, long long> timeRange;
  long long timeSize = 1;
  for (auto timeIndex : timeVec) {
    timeRange = compTRange(timeIndex);
    timeSize *= timeRange.second - timeRange.first + 1;
  }
  return timeSize;
}

bool Analyzer::compAndCheckRequiredDataSize() {
  _tensorDimRange = std::vector<std::vector<long long>>(3);
  _tensorDimRange[ARCH::OUTPUT] = _oriO.getEveryDimRange();
  _tensorDimRange[ARCH::INPUT] = _oriI.getEveryDimRange();
  _tensorDimRange[ARCH::WEIGHT] = _oriW.getEveryDimRange();
  _requiredDataSize[ARCH::OUTPUT] = std::accumulate(
      _tensorDimRange[ARCH::OUTPUT].begin(),
      _tensorDimRange[ARCH::OUTPUT].end(), 1, std::multiplies<long long>());
  _requiredDataSize[ARCH::INPUT] = std::accumulate(
      _tensorDimRange[ARCH::INPUT].begin(), _tensorDimRange[ARCH::INPUT].end(),
      1, std::multiplies<long long>());
  _requiredDataSize[ARCH::WEIGHT] = std::accumulate(
      _tensorDimRange[ARCH::WEIGHT].begin(),
      _tensorDimRange[ARCH::WEIGHT].end(), 1, std::multiplies<long long>());
  //_requiredDataSize[ARCH::OUTPUT] = _O.getVolumn();
  //_requiredDataSize[ARCH::INPUT] = _I.getVolumn();
  //_requiredDataSize[ARCH::WEIGHT] = _W.getVolumn();
  if (_L.checkIfBufferTotal()) {
    long long tmp =
        _requiredDataSize[ARCH::INPUT] * (_doubleBufferFlag ? 2 : 1) +
        _requiredDataSize[ARCH::WEIGHT] * (_doubleBufferFlag ? 2 : 1) +
        _requiredDataSize[ARCH::OUTPUT] * (_doubleBufferFlag ? 2 : 1);
    if (!_L.checkBufferSize(tmp, ARCH::TOTAL))
      return false;
  } else {
    if (!_L.checkBufferSize(_requiredDataSize[ARCH::INPUT] *
                                (_doubleBufferFlag ? 2 : 1),
                            ARCH::INPUT))
      return false;
    if (!_L.checkBufferSize(_requiredDataSize[ARCH::WEIGHT] *
                                (_doubleBufferFlag ? 2 : 1),
                            ARCH::WEIGHT))
      return false;
    if (!_L.checkBufferSize(_requiredDataSize[ARCH::OUTPUT] *
                                (_doubleBufferFlag ? 2 : 1),
                            ARCH::OUTPUT))
      return false;
  }
  return true;
}

int Analyzer::compTotalBandWidth(ARCH::DATATYPE dataType) {
  return _L.getBufferBandWidth(dataType);
}

std::shared_ptr<AnalyzerResult> Analyzer::getResult() { return _result; }
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
    std::vector<std::shared_ptr<AnalyzerResult>> &subLevelResultVec) {
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