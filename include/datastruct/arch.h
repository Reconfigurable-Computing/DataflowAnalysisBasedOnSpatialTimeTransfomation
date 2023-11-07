#pragma once

#include "include/util/debug.h"
#include <assert.h>
#include <climits>
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
typedef enum { INPUT = 0, WEIGHT = 1, OUTPUT = 2, TOTAL = 3 } DATATYPE;
class Array {
private:
  int _rowNum;  // the row num of array
  int _colNum;  // the col num of array
  int _wordBit; // the word bit of array if array is the last level(arithmetic)
  int _spatialDimNum;

public:
  Array() : _rowNum(1), _colNum(1), _wordBit(16), _spatialDimNum(1) {}
  Array(int rowNum, int colNum, int wordBit, int spatialDimNum)
      : _rowNum(rowNum), _colNum(colNum), _wordBit(wordBit),
        _spatialDimNum(spatialDimNum) {}
  int getRowNum() { return _rowNum; }
  int getColNum() { return _colNum; }
  int getSpatialDimNum() { return _spatialDimNum; }
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
    DEBUG::check(_networkType != UNICAST && _networkType != STATIONARY,
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
    DEBUG::check(_networkType != UNICAST && _networkType != STATIONARY,
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
  int getBufferBandWidth() {
    int ret = 0;
    for (auto item : *_networkItemMap) {
      ret += item.second->getBandWidth();
    }
    return ret;
  }
}; // end of Network

class NetworkGroup {
private:
  std::shared_ptr<std::vector<std::shared_ptr<Network>>> _networkSet;
  DATATYPE _dataType;
  std::vector<std::vector<int>> _featureVec;
  bool _peFlag;
  int _bandWidth;

public:
  void outputNetworkGroup(std::ofstream &logFile) {
    std::string ret;
    int reuseVecNum = _featureVec.size();
    if (reuseVecNum == 0)
      return;
    logFile << "{";
    int dimNum = _featureVec[0].size();
    for (int i = 0; i < reuseVecNum; i++) {
      if (i != 0)
        logFile << ',';
      logFile << "\"" << std::to_string(i) << "\":\"";
      for (int j = 0; j < dimNum; j++) {
        logFile << std::to_string(_featureVec[i][j]) + ' ';
      }
      logFile << "\"\n";
    }
    logFile << "}";
    return;
  }
  int getDelay(int base, int bitWidth) {
    return std::ceil(double(base) * double(bitWidth) / double(_bandWidth));
  }
  NetworkGroup(DATATYPE dataType, bool peFlag, int bandWidth)
      : _dataType(dataType), _peFlag(peFlag), _bandWidth(bandWidth) {
    _networkSet = std::make_shared<std::vector<std::shared_ptr<Network>>>();
  }
  NETWORKTYPE classifyNetworkType(std::vector<int> featureVec) {

    DEBUG::check(featureVec.size() == 3, DEBUG::NETWORKFEATUREERROR,
                 DEBUG::vec2string(featureVec));
    int featureX = featureVec[0], featureY = featureVec[1],
        featureT = featureVec[2];
    // DEBUG::check(std::abs(featureX) < 2 && std::abs(featureY) < 2 && featureT
    // == 0 || featureT == 1,
    //    DEBUG::NETWORKFEATUREERROR,
    //    DEBUG::vec2string(featureVec));

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
               std::vector<int> featureVec1, bool peFlag)
      : NetworkGroup(dataType, peFlag, bandWidth) {
    _featureVec.push_back(featureVec1);
    NETWORKTYPE networkType;
    networkType = classifyNetworkType(featureVec1);
    if (networkType == STATIONARY || networkType == UNICAST) {
      _networkSet->push_back(std::make_shared<Network>(featureVec1, rowNum,
                                                       colNum, networkType, 0));
      _networkSet->push_back(std::make_shared<Network>(
          std::vector<int>{1, 0, 1}, rowNum, colNum, SYSTOLIC, bandWidth));
    }
    // else if (networkType == UNICAST)
    //{
    //    _networkSet->push_back(std::make_shared<Network>(featureVec1, rowNum,
    //    colNum, networkType, 0));
    //    _networkSet->push_back(std::make_shared<Network>(std::vector<int>{0,
    //    0, 0}, rowNum, colNum, UNICAST, bandWidth));
    //}
    else {
      _networkSet->push_back(std::make_shared<Network>(
          featureVec1, rowNum, colNum, networkType, bandWidth));
    }
  }
  NetworkGroup(DATATYPE dataType, int rowNum, int colNum, int bandWidth,
               std::vector<int> featureVec1, std::vector<int> featureVec2,
               bool peFlag)
      : NetworkGroup(dataType, peFlag, bandWidth) {
    NETWORKTYPE networkType1, networkType2;
    networkType1 = classifyNetworkType(featureVec1);
    networkType2 = classifyNetworkType(featureVec2);
    if (networkType1 == STATIONARY || networkType1 == UNICAST) {
      _featureVec.push_back(featureVec1);
      // realize STAIONARY with UNICAST / SYSTOLIC
      // realize UNICAST(reuse type) with UNICAST(physics) / SYSTOLIC
      DEBUG::check(networkType2 != MULTICAST && networkType2 != STATIONARY,
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
      DEBUG::check(networkType1 != UNICAST, DEBUG::NETWORKFEATUREERROR,
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
      DEBUG::check(checkDoubleNetwork(featureVec1, featureVec2),
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

    if (_networkSet->size() == 1) {
      return (*_networkSet)[0]->getActiveAccessPointNum(PEXRange, PEYRange);
    } else {

      if (checkIfStationary()) {
        NETWORKTYPE networkType1 = (*_networkSet)[0]->getNetworkType();
        if (networkType1 == STATIONARY)
          return ((long long)PEYRange.second - PEYRange.first + 1) *
                 ((long long)PEXRange.second - PEXRange.first + 1);
        else // for multicast-stationary or systolic-stationary
          return (*_networkSet)[0]->getActiveAccessPointNum(PEXRange, PEYRange);
      } else if (checkIfUnicast())
        return ((long long)PEYRange.second - PEYRange.first + 1) *
               ((long long)PEXRange.second - PEXRange.first + 1);
      else
        return 1;
    }
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
  bool checkIfUnicast() {
    if (_networkSet->size() == 1) {
      return false;
    } else {
      NETWORKTYPE networkType1 = (*_networkSet)[0]->getNetworkType();
      NETWORKTYPE networkType2 = (*_networkSet)[1]->getNetworkType();
      if (networkType1 == UNICAST || networkType2 == UNICAST) {
        return true;
      } else {
        return false;
      }
    }
  }
  int getInitOrOutDelay(int base, int bitWidth, std::pair<int, int> &PEXRange,
                        std::pair<int, int> &PEYRange);
  int getStableDelay(int base, int bitWidth, std::pair<int, int> &PEXRange,
                     std::pair<int, int> &PEYRange);
  bool compareReuseVecAndFeatureVec(std::vector<int> &vec1,
                                    std::vector<int> &vec2) {
    if (vec1[0] < 0) {
      for (int j = 0; j < 3; j++) {
        vec1[0] = -vec1[0];
      }
    }
    bool ret = true;
    for (int j = 0; j < 3; j++) {
      if (vec1[j] != vec2[j])
        ret = false;
    }
    int vec2Num = vec2.size();
    if (vec1[2] == 0) {
      for (int j = 3; j < vec2Num; j++) {
        if (vec2[j] != 0)
          ret = false;
      }
    }
    return ret;
  }
  bool checkNetworkReuseValid(
      std::shared_ptr<std::vector<std::vector<int>>> reuseVec) {
    NETWORKTYPE networkType1 = (*_networkSet)[0]->getNetworkType();
    if (networkType1 == UNICAST) {
      return true;
    } else if (_networkSet->size() == 1) // for MULTICAST SYSTOLIC
    {
      bool ret = true;
      auto &fvec = _featureVec[0];
      bool flag = false;
      for (auto &rvec : *reuseVec) {
        if (compareReuseVecAndFeatureVec(fvec, rvec)) {
          return true;
        }
      }
      return false;
    } else if (networkType1 == STATIONARY) {
      for (auto &rvec : *reuseVec) {
        if (rvec[0] == 0 && rvec[1] == 0 && rvec[2] == 1) {
          return true;
        }
      }
      return false;
    } else {
      NETWORKTYPE networkType2 = (*_networkSet)[1]->getNetworkType();
      bool flag0 = false, flag1 = false;
      for (auto &rvec : *reuseVec) {
        if (compareReuseVecAndFeatureVec(_featureVec[0], rvec)) {
          flag0 = true;
        }
        if (compareReuseVecAndFeatureVec(_featureVec[1], rvec)) {
          flag1 = true;
        }
      }
      return flag0 && flag1;
    }
  }
  int getBufferBandWidth() { return _bandWidth; }
  bool checkIfUnlockPEDim(int index) {
    assert(index == 0 || index == 1);
    if (_networkSet->size() == 1)
      return false;
    else if ((*_networkSet)[0]->getNetworkType() == UNICAST)
      return false;
    else {
      if ((_featureVec[0][index] == 1 && _featureVec[0][1 - index] == 0) ||
          (_featureVec[1][index] == 1 && _featureVec[1][1 - index] == 0))
        return true;
      else
        return false;
    }
  }
}; // end of NetworkGroup

class Buffer {
private:
  BufferType _bufferType;
  DATATYPE _dataType;
  long long _capacity; // define how many bytes can hold by buffer
  int _wordBit;        // the word bit of buffer
  int _readBW;         // read bandwidth
  int _writeBW;        // write bandwidth
  int _readPort;       // define how many network read this buffer

public:
  Buffer(BufferType bufferType, DATATYPE dataType, long long capacity,
         int wordBit)
      : _bufferType(bufferType), _dataType(dataType), _capacity(capacity),
        _wordBit(wordBit), _readBW(0), _writeBW(0), _readPort(0) {}
  bool checkBufferSize(long long requiredBufferSize) {
    return requiredBufferSize * _wordBit / 8 <= _capacity;
  }
  long long getCapacity() { return _capacity; }
}; // end of Buffer
class Level {
private:
  std::shared_ptr<std::map<DATATYPE, std::shared_ptr<Buffer>>> _bufferSet;
  std::shared_ptr<std::map<DATATYPE, std::shared_ptr<NetworkGroup>>>
      _networkGroupSet;
  std::shared_ptr<Array> _array;
  bool _inputWeightSharedBWFlag;
  int _bitWidth;
  bool _peFlag;
  void appendArray(int rowNum, int colNum, int spaitalNum) {
    _array = std::make_shared<Array>(rowNum, colNum, _bitWidth, spaitalNum);
  }

public:
  Level(int rowNum, int colNum, int bitWidth, bool peFlag = false,
        int spatialDimNum = 2)
      : _bitWidth(bitWidth) {
    _bufferSet =
        std::make_shared<std::map<DATATYPE, std::shared_ptr<Buffer>>>();
    _networkGroupSet =
        std::make_shared<std::map<DATATYPE, std::shared_ptr<NetworkGroup>>>();
    _array = std::make_shared<Array>();
    appendArray(rowNum, colNum, spatialDimNum);
    _peFlag = peFlag;
    _inputWeightSharedBWFlag = false;
  }
  Level(int rowNum, int bitWidth, bool peFlag = false)
      : Level(rowNum, 1, bitWidth, peFlag, 1) {}
  Level(int bitWidth, bool peFlag = false) : Level(1, 1, bitWidth, peFlag, 0) {}
  void setInputWeightSharedBW() { _inputWeightSharedBWFlag = true; }
  bool ifInputWeightSharedBW() { return _inputWeightSharedBWFlag; }
  int getSpatialDimNum() { return _array->getSpatialDimNum(); }
  void appendBuffer(BufferType bufferType, DATATYPE dataType,
                    long long capacity = LLONG_MAX) {
    (*_bufferSet)[dataType] =
        std::make_shared<Buffer>(bufferType, dataType, capacity, _bitWidth);
  }
  void appendNetworkGroup(DATATYPE dataType, int bandWidth,
                          std::vector<int> featureVec1) {

    (*_networkGroupSet)[dataType] = std::make_shared<NetworkGroup>(
        dataType, _array->getRowNum(), _array->getColNum(), bandWidth,
        featureVec1, _peFlag);
  }
  void appendNetworkGroup(DATATYPE dataType, int bandWidth,
                          std::vector<int> featureVec1,
                          std::vector<int> featureVec2) {

    (*_networkGroupSet)[dataType] = std::make_shared<NetworkGroup>(
        dataType, _array->getRowNum(), _array->getColNum(), bandWidth,
        featureVec1, featureVec2, _peFlag);
  }

  NETWORKTYPE getNetworkType(DATATYPE dataType) // to do mult network
  {
    return (*_networkGroupSet)[dataType]->getNetworkType();
  }
  bool checkIfBufferTotal() { return _bufferSet->size() == 1; }
  int getActiveAccessPointNum(
      DATATYPE dataType, std::pair<int, int> &PEXRange,
      std::pair<int, int> &PEYRange) // to do mult network
  {
    return (*_networkGroupSet)[dataType]->getActiveAccessPointNum(PEXRange,
                                                                  PEYRange);
  }
  int getInitOrOutDelay(DATATYPE dataType, int base,
                        std::pair<int, int> &PEXRange,
                        std::pair<int, int> &PEYRange) {
    return (*_networkGroupSet)[dataType]->getInitOrOutDelay(base, _bitWidth,
                                                            PEXRange, PEYRange);
  }
  int getStableDelay(DATATYPE dataType, int base, std::pair<int, int> &PEXRange,
                     std::pair<int, int> &PEYRange) {
    return (*_networkGroupSet)[dataType]->getStableDelay(base, _bitWidth,
                                                         PEXRange, PEYRange);
  }
  bool checkIfStationary(DATATYPE dataType) {
    return (*_networkGroupSet)[dataType]->checkIfStationary();
  }
  bool isPE() { return _peFlag; }
  bool checkNetworkReuseValid(
      DATATYPE dataType,
      std::shared_ptr<std::vector<std::vector<int>>> reuseVec) {
    return (*_networkGroupSet)[dataType]->checkNetworkReuseValid(reuseVec);
  }
  int getPENum() { return _array->getColNum() * _array->getRowNum(); }
  bool checkPEDimRange(std::pair<int, int> &PERange, int flag) {
    if (flag == 0)
      return _array->getRowNum() > PERange.second && PERange.first >= 0;
    else
      return _array->getColNum() > PERange.second && PERange.first >= 0;
  }

  int getPEDimRange(int flag) {
    if (flag == 0)
      return _array->getRowNum();
    else
      return _array->getColNum();
  }

  int getBufferBandWidth(DATATYPE dataType) {
    return (*_networkGroupSet)[dataType]->getBufferBandWidth();
  }
  bool checkIfUnlockPEDim(int index, DATATYPE dataType) {
    return (*_networkGroupSet)[dataType]->checkIfUnlockPEDim(index);
  }
  bool checkBufferSize(long long requiredBufferSize, DATATYPE dataType) {
    return (*_bufferSet)[dataType]->checkBufferSize(requiredBufferSize);
  }
  void outputLevel(std::ofstream &logFile) {
    logFile << "{";
    std::string ret;
    logFile << "\"Array Size\":\"" + std::to_string(_array->getRowNum()) + '*' +
                   std::to_string(_array->getColNum()) + "\",\n";
    if (checkIfBufferTotal()) {
      logFile << "\"Total Buffer Size\":\"" +
                     std::to_string((*_bufferSet)[TOTAL]->getCapacity()) +
                     "\",\n";
    } else {
      logFile << "\"Input Buffer Size\":\"" +
                     std::to_string((*_bufferSet)[INPUT]->getCapacity()) +
                     "\",\n";
      logFile << "\"Weight Buffer Size\":\"" +
                     std::to_string((*_bufferSet)[WEIGHT]->getCapacity()) +
                     "\",\n";
      logFile << "\"OUTPUT Buffer Size\":\"" +
                     std::to_string((*_bufferSet)[OUTPUT]->getCapacity()) +
                     "\",\n";
    }

    logFile << "\"Input Network\":";
    (*_networkGroupSet)[INPUT]->outputNetworkGroup(logFile);
    logFile << ",\n";
    logFile << "\"Weight Network\":";
    (*_networkGroupSet)[WEIGHT]->outputNetworkGroup(logFile);
    logFile << ",\n";
    logFile << "\"OUTPUT Network\":";
    (*_networkGroupSet)[OUTPUT]->outputNetworkGroup(logFile);
    logFile << "}";
  }
}; // end of Level

} // namespace ARCH