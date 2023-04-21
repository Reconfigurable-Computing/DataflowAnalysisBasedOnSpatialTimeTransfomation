#include "include/datastruct/arch.h"
namespace ARCH {
void Network::constructNetwork(
    int rowNum, int colNum,
    int doubleNetworkFirstFlag) { // Too difficult to unify so enum

  if (_featureVec[0] == 1 && _featureVec[1] == 0) {
    for (int rowIndex = 0; rowIndex < rowNum; rowIndex++) {
      _networkItemMap->insert(
          std::make_pair(std::pair<int, int>(0, rowIndex),
                         std::make_shared<NetworkItem>(
                             std::pair<int, int>(0, rowIndex), colNum)));
      if (doubleNetworkFirstFlag)
        break;
    }
  } else if (_featureVec[0] == -1 && _featureVec[1] == 0) {
    for (int rowIndex = 0; rowIndex < rowNum; rowIndex++) {
      _networkItemMap->insert(std::make_pair(
          std::pair<int, int>(colNum - 1, rowIndex),
          std::make_shared<NetworkItem>(
              std::pair<int, int>(colNum - 1, rowIndex), colNum)));
      if (doubleNetworkFirstFlag)
        break;
    }
  } else if (_featureVec[0] == 0 && _featureVec[1] == 1) {
    for (int colIndex = 0; colIndex < colNum; colIndex++) {
      _networkItemMap->insert(
          std::make_pair(std::pair<int, int>(colIndex, 0),
                         std::make_shared<NetworkItem>(
                             std::pair<int, int>(colIndex, 0), rowNum)));
      if (doubleNetworkFirstFlag)
        break;
    }
  } else if (_featureVec[0] == 0 && _featureVec[1] == -1) {
    for (int colIndex = 0; colIndex < colNum; colIndex++) {
      _networkItemMap->insert(std::make_pair(
          std::pair<int, int>(colIndex, rowNum - 1),
          std::make_shared<NetworkItem>(
              std::pair<int, int>(colIndex, rowNum - 1), rowNum)));
      if (doubleNetworkFirstFlag)
        break;
    }
  } else if (_featureVec[0] == 1 && _featureVec[1] == 1) {
    for (int colIndex = 0; colIndex < colNum; colIndex++) {
      _networkItemMap->insert(std::make_pair(
          std::pair<int, int>(colIndex, 0),
          std::make_shared<NetworkItem>(std::pair<int, int>(colIndex, 0),
                                        std::min(rowNum, colNum - colIndex))));
    }
    for (int rowIndex = 1; rowIndex < rowNum; rowIndex++) {
      _networkItemMap->insert(std::make_pair(
          std::pair<int, int>(0, rowIndex),
          std::make_shared<NetworkItem>(std::pair<int, int>(0, rowIndex),
                                        std::min(colNum, rowNum - rowIndex))));
    }
  } else if (_featureVec[0] == 1 && _featureVec[1] == -1) {
    for (int colIndex = 0; colIndex < colNum; colIndex++) {
      _networkItemMap->insert(
          std::make_pair(std::pair<int, int>(colIndex, rowNum - 1),
                         std::make_shared<NetworkItem>(
                             std::pair<int, int>(colIndex, rowNum - 1),
                             std::min(rowNum, colNum - colIndex))));
    }
    for (int rowIndex = rowNum - 2; rowIndex >= 0; rowIndex--) {
      _networkItemMap->insert(std::make_pair(
          std::pair<int, int>(0, rowIndex),
          std::make_shared<NetworkItem>(std::pair<int, int>(0, rowIndex),
                                        std::min(colNum, rowIndex + 1))));
    }
  } else if (_featureVec[0] == -1 && _featureVec[1] == 1) {
    for (int colIndex = 0; colIndex < colNum; colIndex++) {
      _networkItemMap->insert(std::make_pair(
          std::pair<int, int>(colIndex, 0),
          std::make_shared<NetworkItem>(std::pair<int, int>(colIndex, 0),
                                        std::min(rowNum, colIndex + 1))));
    }
    for (int rowIndex = 1; rowIndex < rowNum; rowIndex++) {
      _networkItemMap->insert(
          std::make_pair(std::pair<int, int>(colNum - 1, rowIndex),
                         std::make_shared<NetworkItem>(
                             std::pair<int, int>(colNum - 1, rowIndex),
                             std::min(colNum, rowNum - rowIndex))));
    }
  } else if (_featureVec[0] == -1 && _featureVec[1] == -1) {
    for (int colIndex = 0; colIndex < colNum; colIndex++) {
      _networkItemMap->insert(
          std::make_pair(std::pair<int, int>(colIndex, rowNum - 1),
                         std::make_shared<NetworkItem>(
                             std::pair<int, int>(colIndex, rowNum - 1),
                             std::min(rowNum, colIndex + 1))));
    }
    for (int rowIndex = rowNum - 2; rowIndex >= 0; rowIndex--) {
      _networkItemMap->insert(
          std::make_pair(std::pair<int, int>(colNum - 1, rowIndex),
                         std::make_shared<NetworkItem>(
                             std::pair<int, int>(colNum - 1, rowIndex),
                             std::min(colNum, rowIndex + 1))));
    }
  }
}
int NetworkGroup::getInitOrOutDelay(
    int base, int bitWidth, std::pair<int, int> &PEXRange,
    std::pair<int, int> &PEYRange) // for input and weight
{
  if (_networkSet->size() == 1) {
    NETWORKTYPE networkType1 = (*_networkSet)[0]->getNetworkType();
    // MULTICAST UNICAST SYSTOLIC
    return (*_networkSet)[0]->getDelay(base, bitWidth);
  } else {
    NETWORKTYPE networkType1 = (*_networkSet)[0]->getNetworkType();
    NETWORKTYPE networkType2 = (*_networkSet)[1]->getNetworkType();
    if (networkType1 == STATIONARY || networkType1 == UNICAST) {
      // STATIONARY - SYSTOLIC UNICAST
      if (networkType2 == SYSTOLIC) {
        return (*_networkSet)[1]->getDelay(base, bitWidth) *
               (*_networkSet)[1]->getMaxCoupleNum(PEXRange, PEYRange);
      } else // UNICAST
      {
        return (*_networkSet)[1]->getDelay(base, bitWidth);
      }
    } else if (networkType2 == STATIONARY) {
      if (networkType1 == SYSTOLIC) {
        return (*_networkSet)[0]->getDelay(base, bitWidth) *
               (*_networkSet)[0]->getMaxCoupleNum(PEXRange, PEYRange);
      } else // MULTICAST
      {
        return (*_networkSet)[0]->getDelay(base, bitWidth);
      }
    } else {
      return (*_networkSet)[0]->getDelay(base, bitWidth);
      ;
    }
  }
}
int NetworkGroup::getStableDelay(int base, int bitWidth) {
  if (_networkSet->size() == 1) {
    NETWORKTYPE networkType1 = (*_networkSet)[0]->getNetworkType();
    // MULTICAST UNICAST SYSTOLIC
    return (*_networkSet)[0]->getDelay(base, bitWidth);
  } else {
    NETWORKTYPE networkType1 = (*_networkSet)[0]->getNetworkType();
    NETWORKTYPE networkType2 = (*_networkSet)[1]->getNetworkType();
    if (networkType1 == STATIONARY || networkType1 == UNICAST ||
        networkType2 == STATIONARY) {
      return 0;
    } else {
      return std::max((*_networkSet)[0]->getDelay(base, bitWidth),
                      (*_networkSet)[1]->getDelay(base, bitWidth));
    }
  }
}
int Network::getMaxCoupleNum(std::pair<int, int> &PEXRange,
                             std::pair<int, int> &PEYRange) {
  DEBUG::check(_networkType != UNICAST && _networkType != STATIONARY,
               DEBUG::NETWORKOPERROR, "getMaxCoupleNum");
  int featureX = _featureVec[0], featureY = _featureVec[1],
      featureT = _featureVec[2];

  if (featureX == 0) {
    if (featureY == 1) {
      return PEYRange.second + 1;
    } else // featureY = -1
    {
      return _rowNum - PEYRange.first;
    }
  } else if (featureY == 0) {
    if (featureX == 1) {
      return PEXRange.second + 1;
    } else // featureX = -1
    {
      return _colNum - PEXRange.first;
    }
  } else {
    int ret = 0;
    int perCoupleNum;
    for (auto item : (*_networkItemMap)) {
      ret = std::max(ret, getSlantCoupleNum(PEXRange, PEYRange, item.first));
    }
    return ret;
  }
}
int Network::getSlantCoupleNum(std::pair<int, int> &PEXRange,
                               std::pair<int, int> &PEYRange,
                               std::pair<int, int> accessPoint) {
  std::pair<int, int> left, top, right, bottom;
  int featureX = _featureVec[0], featureY = _featureVec[1],
      featureT = _featureVec[2];
  // ax + by + c = 0
  int a, b, c, x, y;
  x = accessPoint.first;
  y = accessPoint.second;
  a = -featureX;
  b = featureY;
  c = -a * x - b * y;

  x = PEXRange.first;
  y = (-c - a * x) / b;
  left.first = x;
  left.second = y;

  x = PEXRange.second;
  y = (-c - a * x) / b;
  right.first = x;
  right.second = y;

  y = PEYRange.first;
  x = (-c - b * y) / a;
  bottom.first = x;
  bottom.second = y;

  y = PEYRange.second;
  x = (-c - b * y) / a;
  top.first = x;
  top.second = y;

  if (featureX * featureY == 1) {
    return std::min(right.first, top.first) -
           std::max(left.first, bottom.first) + 1;
  } else {
    return std::min(right.first, bottom.first) -
           std::max(left.first, top.first) + 1;
  }
}

int Network::getActiveAccessPointNum(std::pair<int, int> &PEXRange,
                                     std::pair<int, int> &PEYRange) {
  DEBUG::check(_networkType != UNICAST && _networkType != STATIONARY,
               DEBUG::NETWORKOPERROR, "getActiveAccessPointNum");
  int featureX = _featureVec[0], featureY = _featureVec[1],
      featureT = _featureVec[2];

  if (featureX == 0) {
    return PEXRange.second - PEXRange.first + 1;
  } else if (featureY == 0) {
    return PEYRange.second - PEYRange.first + 1;
  } else {
    int ret = 0;
    int perCoupleNum;
    for (auto item : (*_networkItemMap)) {
      if (getSlantCoupleNum(PEXRange, PEYRange, item.first) > 0)
        ret++;
    }
    return ret;
  }
}
} // namespace ARCH
