#pragma once

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

public:
  Network(std::vector<int> featureVec, int rowNum, int colNum,
          NETWORKTYPE networkType, int bandWidth,
          int doubleNetworkFirstFlag = 0)
      : _featureVec(featureVec), _networkType(networkType),
        _bandWidth(bandWidth) {

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
  void getAccessPoint(std::vector<std::pair<int, int>> &ret, int rowNum,
                      int colNum) {
    assert(_networkType != UNICAST && _networkType != STATIONARY);
    for (auto item : *_networkItemMap) {
      ret.push_back(item.second->getFirstCoord());
    }
  }
  int getMaxCoupleNum() {
    assert(_networkType != UNICAST && _networkType != STATIONARY);
    int ret = 0;
    for (auto item : (*_networkItemMap)) {
      ret = std::max(item.second->getCoupledNum(), ret);
    }
    return ret;
  }
  void constructNetwork(int rowNum, int colNum, int doubleNetworkFirstFlag);
  void setBandWidth(int bandWidth) {
    _bandWidth = bandWidth;
    assert(_networkType != UNICAST && _networkType != STATIONARY);
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
    assert(_networkType != UNICAST && _networkType != STATIONARY);
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

public:
  NetworkGroup(DATATYPE dataType) : _dataType(dataType) {
    _networkSet = std::make_shared<std::vector<std::shared_ptr<Network>>>();
  }
  NETWORKTYPE classifyNetworkType(std::vector<int> featureVec) {

    assert(featureVec.size() == 3);
    int featureX = featureVec[0], featureY = featureVec[1],
        featureT = featureVec[2];
    assert(std::abs(featureX) < 2);
    assert(std::abs(featureY) < 2);
    assert(featureT == 0 || featureT == 1);
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

    NETWORKTYPE networkType;
    networkType = classifyNetworkType(featureVec1);
    if (networkType == STATIONARY) {
      _networkSet->push_back(std::make_shared<Network>(featureVec1, rowNum,
                                                       colNum, STATIONARY, 0));
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
    if (networkType1 == STATIONARY) {
      assert(networkType2 != MULTICAST && networkType2 != STATIONARY);
      // SYSTOLIC UNICAST valid
      _networkSet->push_back(std::make_shared<Network>(
          featureVec1, rowNum, colNum, networkType1, 0));
      _networkSet->push_back(std::make_shared<Network>(
          featureVec2, rowNum, colNum, networkType2, bandWidth));
    } else if (networkType2 == STATIONARY) {
      assert(networkType1 != UNICAST);
      // SYSTOLIC MULTICAST valid
      _networkSet->push_back(std::make_shared<Network>(
          featureVec1, rowNum, colNum, networkType1, bandWidth));
      _networkSet->push_back(std::make_shared<Network>(
          featureVec2, rowNum, colNum, networkType2, 0));
    } else {
      // MULTICAST-MULTICAST MULTICAST-UNICAST MULTICAST-SYSTOLIC valid
      // SYSTOLIC-MULTICAST SYSTOLIC-SYSTOLIC SYSTOLIC-UNICAST valid
      assert(checkDoubleNetwork(featureVec1, featureVec2));
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
  void getAccessPoint(std::vector<std::pair<int, int>> &ret, int rowNum,
                      int colNum) // to do mult network
  {
    (*_networkSet)[0]->getAccessPoint(ret, rowNum, colNum);
  }
  int getInitOrOutDelay(int base, int bitWidth);
  int getStableDelay(int base, int bitWidth);
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
  void
  getAccessPoint(DATATYPE dataType,
                 std::vector<std::pair<int, int>> &ret) // to do mult network
  {
    (*_networkGroupSet)[dataType]->getAccessPoint(ret, _array->getRowNum(),
                                                  _array->getColNum());
  }
  int getInitOrOutDelay(DATATYPE dataType, int base, int bitWidth) {
    return (*_networkGroupSet)[dataType]->getInitOrOutDelay(base, bitWidth);
  }
  int getStableDelay(DATATYPE dataType, int base, int bitWidth) {
    return (*_networkGroupSet)[dataType]->getStableDelay(base, bitWidth);
  }
}; // end of Level

} // namespace ARCH
