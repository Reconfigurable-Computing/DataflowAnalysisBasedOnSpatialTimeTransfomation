#pragma once

#include <assert.h>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
namespace ARCH {
// decclare the network reuse type
typedef enum { UNICAST, MULTICAST, SYSTOLIC, STATIONARY } NETWORKTYPE;
// declare the physical type of buffer
typedef enum { SRAM, DRAM, REG } BufferType;
// declare the data type of buffer
typedef enum { INPUT, WEIGHT, OUTPUT } DATATYPE;
class Array {
private:
  int _rowNum;  // the row num of array
  int _colNum;  // the col num of array
  int _wordBit; // the word bit of array if array is the last level(arithmetic)

public:
  Array() : _rowNum(1), _colNum(1), _wordBit(16) {}
  Array(int rowNum, int colNum, int wordBit)
      : _rowNum(rowNum), _colNum(colNum), _wordBit(wordBit) {}
}; // end of Array
class NetworkItem {
private:
  std::pair<int, int>
      _firstCoord; // the first coordinate in array coupled with network
  int _coupledNum; // the num of coordinate in array coupled with network

public:
  NetworkItem(std::pair<int, int> firstCoord, int coupledNum)
      : _firstCoord(firstCoord), _coupledNum(coupledNum) {}

}; // end of NetworkItem
class Network {
private:
  std::vector<int> _featureVec;
  std::shared_ptr<std::vector<std::shared_ptr<NetworkItem>>>
      _networkItemSet; // store network, unicast networks do not store
  NETWORKTYPE _networkType;

public:
  Network(std::vector<int> featureVec, int rowNum, int colNum)
      : _featureVec(featureVec) {
    int featureX = featureVec[0], featureY = featureVec[1],
        featureT = featureVec[2];
    assert(std::abs(featureX) < 2);
    assert(std::abs(featureY) < 2);
    assert(featureT == 0 || featureT == 1);
    if (!(featureX == 0 && featureY == 0) && featureT == 0)
      _networkType = MULTICAST;
    else if (!(featureX == 0 && featureY == 0) && featureT != 0)
      _networkType = SYSTOLIC;
    else if (featureX == 0 && featureY == 0 && featureT == 0)
      _networkType = UNICAST;
    else
      _networkType = STATIONARY;
    if (_networkType == SYSTOLIC || _networkType == MULTICAST) {
      _networkItemSet =
          std::make_shared<std::vector<std::shared_ptr<NetworkItem>>>();
      constructNetwork(rowNum, colNum);
    }
  }

  void constructNetwork(int rowNum, int colNum) {
    if (_featureVec[0] == 1 && _featureVec[1] == 0) {
      for (int rowIndex = 0; rowIndex < rowNum; rowIndex++) {
        _networkItemSet->push_back(std::make_shared<NetworkItem>(
            std::pair<int, int>(0, rowIndex), colNum));
      }
    } else if (_featureVec[0] == -1 && _featureVec[1] == 0) {
      for (int rowIndex = 0; rowIndex < rowNum; rowIndex++) {
        _networkItemSet->push_back(std::make_shared<NetworkItem>(
            std::pair<int, int>(colNum - 1, rowIndex), colNum));
      }
    } else if (_featureVec[0] == 0 && _featureVec[1] == 1) {
      for (int colIndex = 0; colIndex < colNum; colIndex++) {
        _networkItemSet->push_back(std::make_shared<NetworkItem>(
            std::pair<int, int>(colIndex, 0), rowNum));
      }
    } else if (_featureVec[0] == 0 && _featureVec[1] == -1) {
      for (int colIndex = 0; colIndex < colNum; colIndex++) {
        _networkItemSet->push_back(std::make_shared<NetworkItem>(
            std::pair<int, int>(colIndex, rowNum - 1), rowNum));
      }
    } else if (_featureVec[0] == 1 && _featureVec[1] == 1) {
      for (int colIndex = 0; colIndex < colNum; colIndex++) {
        _networkItemSet->push_back(
            std::make_shared<NetworkItem>(std::pair<int, int>(colIndex, 0),
                                          std::min(rowNum, colNum - colIndex)));
      }
      for (int rowIndex = 1; rowIndex < rowNum; rowIndex++) {
        _networkItemSet->push_back(
            std::make_shared<NetworkItem>(std::pair<int, int>(0, rowIndex),
                                          std::min(colNum, rowNum - rowIndex)));
      }
    } else if (_featureVec[0] == 1 && _featureVec[1] == -1) {
      for (int colIndex = 0; colIndex < colNum; colIndex++) {
        _networkItemSet->push_back(std::make_shared<NetworkItem>(
            std::pair<int, int>(colIndex, rowNum - 1),
            std::min(rowNum, colNum - colIndex)));
      }
      for (int rowIndex = rowNum - 2; rowIndex >= 0; rowIndex--) {
        _networkItemSet->push_back(std::make_shared<NetworkItem>(
            std::pair<int, int>(0, rowIndex), std::min(colNum, rowIndex + 1)));
      }
    } else if (_featureVec[0] == -1 && _featureVec[1] == 1) {
      for (int colIndex = 0; colIndex < colNum; colIndex++) {
        _networkItemSet->push_back(std::make_shared<NetworkItem>(
            std::pair<int, int>(colIndex, 0), std::min(rowNum, colIndex + 1)));
      }
      for (int rowIndex = 1; rowIndex < rowNum; rowIndex++) {
        _networkItemSet->push_back(std::make_shared<NetworkItem>(
            std::pair<int, int>(colNum - 1, rowIndex),
            std::min(colNum, rowNum - rowIndex)));
      }
    } else if (_featureVec[0] == -1 && _featureVec[1] == -1) {
      for (int colIndex = 0; colIndex < colNum; colIndex++) {
        _networkItemSet->push_back(std::make_shared<NetworkItem>(
            std::pair<int, int>(colIndex, rowNum - 1),
            std::min(rowNum, colIndex + 1)));
      }
      for (int rowIndex = rowNum - 2; rowIndex >= 0; rowIndex--) {
        _networkItemSet->push_back(std::make_shared<NetworkItem>(
            std::pair<int, int>(colNum - 1, rowIndex),
            std::min(colNum, rowIndex + 1)));
      }
    }
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
  NetworkGroup(DATATYPE dataType, int rowNum, int colNum,
               std::vector<int> featureVec1)
      : NetworkGroup(dataType) {
    assert(featureVec1.size() == 3);
    _networkSet->push_back(
        std::make_shared<Network>(featureVec1, rowNum, colNum));
  }
  NetworkGroup(DATATYPE dataType, int rowNum, int colNum,
               std::vector<int> featureVec1, std::vector<int> featureVec2)
      : _dataType(dataType) {
    assert(featureVec1.size() == 3);
    assert(featureVec2.size() == 3);
    _networkSet->push_back(
        std::make_shared<Network>(featureVec1, rowNum, colNum));
    _networkSet->push_back(
        std::make_shared<Network>(featureVec2, rowNum, colNum));
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

public:
  Level() {
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
                          std::vector<int> featureVec1) {

    (*_networkGroupSet)[dataType] =
        std::make_shared<NetworkGroup>(dataType, rowNum, colNum, featureVec1);
  }
  void appendNetworkGroup(int rowNum, int colNum, DATATYPE dataType,
                          std::vector<int> featureVec1,
                          std::vector<int> featureVec2) {

    (*_networkGroupSet)[dataType] = std::make_shared<NetworkGroup>(
        dataType, rowNum, colNum, featureVec1, featureVec2);
  }
  void appendArray(int rowNum, int colNum, int wordBit) {
    _array = std::make_shared<Array>(rowNum, colNum, wordBit);
  }
}; // end of Level

} // namespace ARCH
