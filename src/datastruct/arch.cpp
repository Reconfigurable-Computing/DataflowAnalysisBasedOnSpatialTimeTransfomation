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
int NetworkGroup::getInitOrOutDelay(int base,
                                    int bitWidth) // for input and weight
{
  if (_networkSet->size() == 1) {
    NETWORKTYPE networkType1 = (*_networkSet)[0]->getNetworkType();
    // MULTICAST UNICAST SYSTOLIC
    return (*_networkSet)[0]->getDelay(base, bitWidth);
  } else {
    NETWORKTYPE networkType1 = (*_networkSet)[0]->getNetworkType();
    NETWORKTYPE networkType2 = (*_networkSet)[1]->getNetworkType();
    if (networkType1 == STATIONARY) {
      // STATIONARY - SYSTOLIC UNICAST
      if (networkType2 == SYSTOLIC) {
        return (*_networkSet)[1]->getDelay(base, bitWidth) *
               (*_networkSet)[1]->getMaxCoupleNum();
      } else // UNICAST
      {
        return (*_networkSet)[1]->getDelay(base, bitWidth);
      }
    } else if (networkType2 == STATIONARY) {
      if (networkType1 == SYSTOLIC) {
        return (*_networkSet)[0]->getDelay(base, bitWidth) *
               (*_networkSet)[0]->getMaxCoupleNum();
      } else // MULTICAST
      {
        return (*_networkSet)[0]->getDelay(base, bitWidth);
      }
    } else {
      int initDelay = 0;
      if (networkType1 == SYSTOLIC) {
        initDelay += (*_networkSet)[0]->getDelay(base, bitWidth) *
                     (*_networkSet)[0]->getMaxCoupleNum();
      } else {
        initDelay += (*_networkSet)[0]->getDelay(base, bitWidth);
      }
      if (networkType2 == SYSTOLIC) {
        initDelay += (*_networkSet)[1]->getDelay(base, bitWidth) *
                     (*_networkSet)[1]->getMaxCoupleNum();
      } else {
        initDelay += (*_networkSet)[1]->getDelay(base, bitWidth);
      }
      return initDelay;
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
    if (networkType1 == STATIONARY || networkType2 == STATIONARY) {
      return 0;
    } else {
      return std::max((*_networkSet)[0]->getDelay(base, bitWidth),
                      (*_networkSet)[1]->getDelay(base, bitWidth));
    }
  }
}
} // namespace ARCH