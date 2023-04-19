#pragma once

#include "include/util/debug.h"
#include <assert.h>
#include <iostream>
#include <map>
#include <math.h>
#include <memory>
#include <string>
#include <vector>
namespace ARCH {
// decclare the network reuse type
typedef enum { UNICAST, MULTICAST, SYSTOLIC, STATIONARY } NETWORKTYPE;
// declare the physical type of buffer
typedef enum { SRAM, DRAM, REG } BufferType;
// declare the data type of buffer
typedef enum { INPUT = 0, WEIGHT = 1, OUTPUT = 2 } DATATYPE;
class Array {
private:
  int _rowNum;  // the row num of array
  int _colNum;  // the col num of array
  int _wordBit; // the word bit of array if array is the last level(arithmetic)

public:
  Array() : _rowNum(1), _colNum(1), _wordBit(16) {}
  Array(int rowNum, int colNum, int wordBit)
      : _rowNum(rowNum), _colNum(colNum), _wordBit(wordBit) {}
  int getRowNum() { return _rowNum; }
  int getColNum() { return _colNum; }
}; // end of Array
class NetworkItem {
private:
  std::pair<int, int>
      _firstCoord; // the first coordinate in array coupled with network
  int _coupledNum; // the num of coordinate in array coupled with network
  int _bandWidth;

public:
  NetworkItem(std::pair<int, int> firstCoord, int coupledNum)
      : _firstCoord(firstCoord), _coupledNum(coupledNum), _bandWidth(0) {}
  std::pair<int, int> getFirstCoord() { return _firstCoord; }
  int getCoupledNum() { return _coupledNum; }
  void setBandWidth(int bandWidth) { _bandWidth = bandWidth; }
  int getBandWidth() { return _bandWidth; }
}; // end of NetworkItem
class Network {
private:
  std::vector<int> _featureVec;
  std::shared_ptr<std::map<std::pair<int, int>, std::shared_ptr<NetworkItem>>>
      _networkItemMap; // store network, unicast networks do not store
  NETWORKTYPE _networkType;
  int _bandWidth; // for unicast and systolic is common case  for multicast
                  // means baseBandWidth  unit:bit/cycle
  int _rowNum;
  int _colNum;

public:
  Network(std::vector<int> featureVec, int rowNum, int colNum,
          NETWORKTYPE networkType, int bandWidth,
          int doubleNetworkFirstFlag = 0)
      : _featureVec(featureVec), _networkType(networkType),
        _bandWidth(bandWidth), _colNum(colNum), _rowNum(rowNum) {

    if (_networkType == SYSTOLIC || _networkType == MULTICAST) {
      _networkItemMap = std::make_shared<
          std::map<std::pair<int, int>, std::shared_ptr<NetworkItem>>>();
      constructNetwork(rowNum, colNum, doubleNetworkFirstFlag);
      if (_networkType == MULTICAST) {
        setBandWidthByCoupleNumRatio(bandWidth);
      } else // SYSTOLIC
      {
        setBandWidth(bandWidth);
      }
    } else // UNICAST STATIONARY(no use)
    {
      // _bandWidth = bandWidth;
    }
  }

  NETWORKTYPE getNetworkType() { return _networkType; }
  int getActiveAccessPointNum(std::pair<int, int> &PEXRange,
                              std::pair<int, int> &PEYRange);
  // num from access point(first Coord) to max coupled coord
  int getSlantCoupleNum(std::pair<int, int> &PEXRange,
                        std::pair<int, int> &PEYRange,
                        std::pair<int, int> accessPoint);
  int getMaxCoupleNum(std::pair<int, int> &PEXRange,
                      std::pair<int, int> &PEYRange);
  void constructNetwork(int rowNum, int colNum, int doubleNetworkFirstFlag);
  void setBandWidth(int bandWidth) {
    _bandWidth = bandWidth;
    DEBUG::checkError(_networkType != UNICAST && _networkType != STATIONARY,
                      DEBUG::NETWORKOPERROR, "setBandWidth");
    for (auto item : *_networkItemMap) {
      item.second->setBandWidth(bandWidth);
    }
  }
  // void setBandWidth(std::pair<int, int> firstCoord, int bandWidth)
  //{
  //    assert(_networkType != UNICAST && _networkType != STATIONARY);
  //    (*_networkItemMap)[firstCoord]->setBandWidth(bandWidth);
  //}
  void setBandWidthByCoupleNumRatio(int baseBandWidth) {
    _bandWidth = baseBandWidth;
    DEBUG::checkError(_networkType != UNICAST && _networkType != STATIONARY,
                      DEBUG::NETWORKOPERROR, "setBandWidthByCoupleNumRatio");
    for (auto item : *_networkItemMap) {
      item.second->setBandWidth(baseBandWidth * item.second->getCoupledNum());
    }
  }
  int getBandWidth() { return _bandWidth; }
  int getNetworkItemNum() { return _networkItemMap->size(); }
  int getDelay(int base, int bitWidth) {
    return std::ceil(double(base) * double(bitWidth) / double(_bandWidth));
  }
}; // end of Network

class NetworkGroup {
private:
  std::shared_ptr<std::vector<std::shared_ptr<Network>>> _networkSet;
  DATATYPE _dataType;
  std::vector<std::vector<int>> _featureVec;

public:
  NetworkGroup(DATATYPE dataType) : _dataType(dataType) {
    _networkSet = std::make_shared<std::vector<std::shared_ptr<Network>>>();
  }
  NETWORKTYPE classifyNetworkType(std::vector<int> featureVec) {

    DEBUG::checkError(featureVec.size() == 3, DEBUG::NETWORKFEATUREERROR,
                      DEBUG::vec2string(featureVec));
    int featureX = featureVec[0], featureY = featureVec[1],
        featureT = featureVec[2];
    DEBUG::checkError(
        std::abs(featureX) < 2 && std::abs(featureY) < 2 && featureT == 0 ||
            featureT == 1,
        DEBUG::NETWORKFEATUREERROR, DEBUG::vec2string(featureVec));

    if (!(featureX == 0 && featureY == 0) && featureT == 0)
      return MULTICAST;
    else if (!(featureX == 0 && featureY == 0) && featureT != 0)
      return SYSTOLIC;
    else if (featureX == 0 && featureY == 0 && featureT == 0)
      return UNICAST;
    else
      return STATIONARY;
  }
  NetworkGroup(DATATYPE dataType, int rowNum, int colNum, int bandWidth,
               std::vector<int> featureVec1)
      : NetworkGroup(dataType) {
    _featureVec.push_back(featureVec1);
    NETWORKTYPE networkType;
    networkType = classifyNetworkType(featureVec1);
    if (networkType == STATIONARY || networkType == UNICAST) {
      _networkSet->push_back(std::make_shared<Network>(featureVec1, rowNum,
                                                       colNum, networkType, 0));
      _networkSet->push_back(std::make_shared<Network>(
          std::vector<int>{1, 0, 1}, rowNum, colNum, SYSTOLIC, bandWidth));
    } else {
      _networkSet->push_back(std::make_shared<Network>(
          featureVec1, rowNum, colNum, networkType, bandWidth));
    }
  }
  NetworkGroup(DATATYPE dataType, int rowNum, int colNum, int bandWidth,
               std::vector<int> featureVec1, std::vector<int> featureVec2)
      : NetworkGroup(dataType) {
    NETWORKTYPE networkType1, networkType2;
    networkType1 = classifyNetworkType(featureVec1);
    networkType2 = classifyNetworkType(featureVec2);
    if (networkType1 == STATIONARY || networkType1 == UNICAST) {
      _featureVec.push_back(featureVec1);
      // realize STAIONARY with UNICAST / SYSTOLIC
      // realize UNICAST(reuse type) with UNICAST(physics) / SYSTOLIC
      DEBUG::checkError(networkType2 != MULTICAST && networkType2 != STATIONARY,
                        DEBUG::NETWORKFEATUREERROR,
                        DEBUG::vec2string(featureVec1) +
                            DEBUG::vec2string(featureVec2));
      // SYSTOLIC UNICAST valid
      _networkSet->push_back(std::make_shared<Network>(
          featureVec1, rowNum, colNum, networkType1, 0));
      _networkSet->push_back(std::make_shared<Network>(
          featureVec2, rowNum, colNum, networkType2, bandWidth));
    } else if (networkType2 == STATIONARY) {
      _featureVec.push_back(featureVec1);
      _featureVec.push_back(featureVec2);
      // realize STAIONARY with MULTICAST / SYSTOLIC
      DEBUG::checkError(networkType1 != UNICAST, DEBUG::NETWORKFEATUREERROR,
                        DEBUG::vec2string(featureVec1) +
                            DEBUG::vec2string(featureVec2));
      // SYSTOLIC MULTICAST valid
      _networkSet->push_back(std::make_shared<Network>(
          featureVec1, rowNum, colNum, networkType1, bandWidth));
      _networkSet->push_back(std::make_shared<Network>(
          featureVec2, rowNum, colNum, networkType2, 0));
    } else {
      _featureVec.push_back(featureVec1);
      _featureVec.push_back(featureVec2);
      // MULTICAST-MULTICAST MULTICAST-UNICAST MULTICAST-SYSTOLIC valid
      // SYSTOLIC-MULTICAST SYSTOLIC-SYSTOLIC SYSTOLIC-UNICAST valid
      DEBUG::checkError(checkDoubleNetwork(featureVec1, featureVec2),
                        DEBUG::NETWORKFEATUREERROR,
                        DEBUG::vec2string(featureVec1) +
                            DEBUG::vec2string(featureVec2));
      _networkSet->push_back(std::make_shared<Network>(
          featureVec1, rowNum, colNum, networkType1, 0, 1));
      _networkSet->push_back(std::make_shared<Network>(
          featureVec2, rowNum, colNum, networkType2, bandWidth));
      setFirstNetworkBySecondNetwork(networkType1, networkType2);
    }
  }
  bool checkDoubleNetwork(std::vector<int> featureVec1,
                          std::vector<int> featureVec2) // to do more check
  {
    int featureX1 = featureVec1[0], featureY1 = featureVec1[1],
        featureT1 = featureVec1[2];
    int featureX2 = featureVec2[0], featureY2 = featureVec2[1],
        featureT2 = featureVec2[2];
    if (featureX1 == 0 && featureY1 != 0) {
      if (featureY2 == 0)
        return true;
    }
    if (featureX1 != 0 && featureY1 == 0) {
      if (featureX2 == 0)
        return true;
    }
    return false;
  }
  void setFirstNetworkBySecondNetwork(NETWORKTYPE networkType1,
                                      NETWORKTYPE networkType2) {
    if (networkType1 == MULTICAST) {
      (*_networkSet)[0]->setBandWidthByCoupleNumRatio(
          (*_networkSet)[1]->getBandWidth());
    } else // SYSTOLIC
    {
      (*_networkSet)[0]->setBandWidth((*_networkSet)[1]->getBandWidth());
    }
  }

  NETWORKTYPE getNetworkType() // to do mult network
  {
    return (*_networkSet)[0]->getNetworkType();
  }
  int getActiveAccessPointNum(
      std::pair<int, int> &PEXRange,
      std::pair<int, int> &PEYRange) // to do mult network
  {
    return (*_networkSet)[0]->getActiveAccessPointNum(PEXRange, PEYRange);
  }
  bool checkIfStationary() {
    if (_networkSet->size() == 1) {
      return false;
    } else {
      NETWORKTYPE networkType1 = (*_networkSet)[0]->getNetworkType();
      NETWORKTYPE networkType2 = (*_networkSet)[1]->getNetworkType();
      if (networkType1 == STATIONARY || networkType2 == STATIONARY) {
        return true;
      } else {
        return false;
      }
    }
  }
  int getInitOrOutDelay(int base, int bitWidth, std::pair<int, int> &PEXRange,
                        std::pair<int, int> &PEYRange);
  int getStableDelay(int base, int bitWidth);
  bool compareReuseVecAndFeatureVec(std::vector<int> &vec1,
                                    std::vector<int> &vec2) {
    bool ret = true;
    for (int j = 0; j < 3; j++) {
      if (vec1[j] != vec2[j])
        ret = false;
    }
    return ret;
  }
  bool checkNetworkReuseValid(
      std::shared_ptr<std::vector<std::vector<int>>> reuseVec) {
    NETWORKTYPE networkType1 = (*_networkSet)[0]->getNetworkType();
    if (networkType1 == UNICAST)
      return true;
    bool ret = true;
    for (auto &fvec : _featureVec) {
      bool flag = false;
      for (auto &rvec : *reuseVec) {
        if (compareReuseVecAndFeatureVec(fvec, rvec)) {
          flag = true;
          break;
        }
      }
      if (!flag) {
        return false;
      }
    }
    return true;
  }
}; // end of NetworkGroup

class Buffer {
private:
  BufferType _bufferType;
  DATATYPE _dataType;
  int _capacity; // define how many words can hold by buffer
  int _wordBit;  // the word bit of buffer
  int _readBW;   // read bandwidth
  int _writeBW;  // write bandwidth
  int _readPort; // define how many network read this buffer

public:
  Buffer(BufferType bufferType, DATATYPE dataType, int capacity, int wordBit)
      : _bufferType(bufferType), _dataType(dataType), _capacity(capacity),
        _wordBit(wordBit), _readBW(0), _writeBW(0), _readPort(0) {}
}; // end of Buffer
class Level {
private:
  std::shared_ptr<std::map<DATATYPE, std::shared_ptr<Buffer>>> _bufferSet;
  std::shared_ptr<std::map<DATATYPE, std::shared_ptr<NetworkGroup>>>
      _networkGroupSet;
  std::shared_ptr<Array> _array;
  int _bitWidth;

public:
  Level(int bitWidth) : _bitWidth(bitWidth) {
    _bufferSet =
        std::make_shared<std::map<DATATYPE, std::shared_ptr<Buffer>>>();
    _networkGroupSet =
        std::make_shared<std::map<DATATYPE, std::shared_ptr<NetworkGroup>>>();
    _array = std::make_shared<Array>();
  }
  void appendBuffer(BufferType bufferType, DATATYPE dataType, int capacity,
                    int wordBit) {
    (*_bufferSet)[dataType] =
        std::make_shared<Buffer>(bufferType, dataType, capacity, wordBit);
  }
  void appendNetworkGroup(int rowNum, int colNum, DATATYPE dataType,
                          int bandWidth, std::vector<int> featureVec1) {

    (*_networkGroupSet)[dataType] = std::make_shared<NetworkGroup>(
        dataType, rowNum, colNum, bandWidth, featureVec1);
  }
  void appendNetworkGroup(int rowNum, int colNum, DATATYPE dataType,
                          int bandWidth, std::vector<int> featureVec1,
                          std::vector<int> featureVec2) {

    (*_networkGroupSet)[dataType] = std::make_shared<NetworkGroup>(
        dataType, rowNum, colNum, bandWidth, featureVec1, featureVec2);
  }
  void appendArray(int rowNum, int colNum, int wordBit) {
    _array = std::make_shared<Array>(rowNum, colNum, wordBit);
  }

  NETWORKTYPE getNetworkType(DATATYPE dataType) // to do mult network
  {
    return (*_networkGroupSet)[dataType]->getNetworkType();
  }
  int getActiveAccessPointNum(
      DATATYPE dataType, std::pair<int, int> &PEXRange,
      std::pair<int, int> &PEYRange) // to do mult network
  {
    return (*_networkGroupSet)[dataType]->getActiveAccessPointNum(PEXRange,
                                                                  PEYRange);
  }
  int getInitOrOutDelay(DATATYPE dataType, int base, int bitWidth,
                        std::pair<int, int> &PEXRange,
                        std::pair<int, int> &PEYRange) {
    return (*_networkGroupSet)[dataType]->getInitOrOutDelay(base, bitWidth,
                                                            PEXRange, PEYRange);
  }
  int getStableDelay(DATATYPE dataType, int base, int bitWidth) {
    return (*_networkGroupSet)[dataType]->getStableDelay(base, bitWidth);
  }
  bool checkIfStationary(DATATYPE dataType) {
    return (*_networkGroupSet)[dataType]->checkIfStationary();
  }

  bool checkNetworkReuseValid(
      DATATYPE dataType,
      std::shared_ptr<std::vector<std::vector<int>>> reuseVec) {
    return (*_networkGroupSet)[dataType]->checkNetworkReuseValid(reuseVec);
  }
}; // end of Level

} // namespace ARCH