
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
class Network {
private:
  std::pair<int, int>
      _firstCoord; // the first coordinate in array coupled with network
  int _coupledNum; // the num of coordinate in array coupled with network

public:
  Network(std::pair<int, int> firstCoord, int coupledNum)
      : _firstCoord(firstCoord), _coupledNum(coupledNum) {}

}; // end of Network
class NetworkGroup {
private:
  int _featureVec[3];
  std::shared_ptr<std::vector<std::shared_ptr<Network>>>
      _NetworkSet; // store network, unicast networks do not store
  NETWORKTYPE _networkType;
  DATATYPE _dataType;

public:
  NetworkGroup(int featureX, int featureY, int featureT,
               NETWORKTYPE networkType, int rowNum, int colNum,
               DATATYPE dataType)
      : _dataType(dataType) {
    assert(std::abs(featureX) < 2);
    assert(std::abs(featureY) < 2);
    assert(featureT == 0 || featureT == 1);
    // never set a stationary reuse feature, if need stationay, set networkType
    // as STATIONARY but featureXYT as a different type
    assert(!(featureX == 0 && featureY == 0 && featureT != 0));
    if ((featureX != 0 || featureY != 0) && featureT == 0)
      assert(networkType == MULTICAST || networkType == STATIONARY);
    else if ((featureX != 0 || featureY != 0) && featureT != 0)
      assert(networkType == SYSTOLIC || networkType == STATIONARY);
    else if (featureX == 0 || featureY == 0 && featureT == 0)
      assert(networkType == UNICAST || networkType == STATIONARY);
    _networkType = networkType;
    _featureVec[0] = featureX;
    _featureVec[1] = featureY;
    _featureVec[2] = featureT;
    if (!(featureX == 0 && featureY == 0 && featureT == 0)) {
      _NetworkSet = std::make_shared<std::vector<std::shared_ptr<Network>>>();
      constructNetwork(rowNum, colNum);
    }
  }
  void constructNetwork(int rowNum, int colNum) {
    if (_featureVec[0] == 1 && _featureVec[1] == 0) {
      for (int rowIndex = 0; rowIndex < rowNum; rowIndex++) {
        _NetworkSet->push_back(std::make_shared<Network>(
            std::pair<int, int>(0, rowIndex), colNum));
      }
    } else if (_featureVec[0] == -1 && _featureVec[1] == 0) {
      for (int rowIndex = 0; rowIndex < rowNum; rowIndex++) {
        _NetworkSet->push_back(std::make_shared<Network>(
            std::pair<int, int>(colNum - 1, rowIndex), colNum));
      }
    } else if (_featureVec[0] == 0 && _featureVec[1] == 1) {
      for (int colIndex = 0; colIndex < colNum; colIndex++) {
        _NetworkSet->push_back(std::make_shared<Network>(
            std::pair<int, int>(colIndex, 0), rowNum));
      }
    } else if (_featureVec[0] == 0 && _featureVec[1] == -1) {
      for (int colIndex = 0; colIndex < colNum; colIndex++) {
        _NetworkSet->push_back(std::make_shared<Network>(
            std::pair<int, int>(colIndex, rowNum - 1), rowNum));
      }
    } else if (_featureVec[0] == 1 && _featureVec[1] == 1) {
      for (int colIndex = 0; colIndex < colNum; colIndex++) {
        _NetworkSet->push_back(
            std::make_shared<Network>(std::pair<int, int>(colIndex, 0),
                                      std::min(rowNum, colNum - colIndex)));
      }
      for (int rowIndex = 1; rowIndex < rowNum; rowIndex++) {
        _NetworkSet->push_back(
            std::make_shared<Network>(std::pair<int, int>(0, rowIndex),
                                      std::min(colNum, rowNum - rowIndex)));
      }
    } else if (_featureVec[0] == 1 && _featureVec[1] == -1) {
      for (int colIndex = 0; colIndex < colNum; colIndex++) {
        _NetworkSet->push_back(
            std::make_shared<Network>(std::pair<int, int>(colIndex, rowNum - 1),
                                      std::min(rowNum, colNum - colIndex)));
      }
      for (int rowIndex = rowNum - 2; rowIndex >= 0; rowIndex--) {
        _NetworkSet->push_back(std::make_shared<Network>(
            std::pair<int, int>(0, rowIndex), std::min(colNum, rowIndex + 1)));
      }
    } else if (_featureVec[0] == -1 && _featureVec[1] == 1) {
      for (int colIndex = 0; colIndex < colNum; colIndex++) {
        _NetworkSet->push_back(std::make_shared<Network>(
            std::pair<int, int>(colIndex, 0), std::min(rowNum, colIndex + 1)));
      }
      for (int rowIndex = 1; rowIndex < rowNum; rowIndex++) {
        _NetworkSet->push_back(
            std::make_shared<Network>(std::pair<int, int>(colNum - 1, rowIndex),
                                      std::min(colNum, rowNum - rowIndex)));
      }
    } else if (_featureVec[0] == -1 && _featureVec[1] == -1) {
      for (int colIndex = 0; colIndex < colNum; colIndex++) {
        _NetworkSet->push_back(
            std::make_shared<Network>(std::pair<int, int>(colIndex, rowNum - 1),
                                      std::min(rowNum, colIndex + 1)));
      }
      for (int rowIndex = rowNum - 2; rowIndex >= 0; rowIndex--) {
        _NetworkSet->push_back(
            std::make_shared<Network>(std::pair<int, int>(colNum - 1, rowIndex),
                                      std::min(colNum, rowIndex + 1)));
      }
    }
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
  void appendNetworkGroup(int featureX, int featureY, int featureT,
                          NETWORKTYPE networkType, int rowNum, int colNum,
                          DATATYPE dataType) {
    (*_networkGroupSet)[dataType] = std::make_shared<NetworkGroup>(
        featureX, featureY, featureT, networkType, rowNum, colNum, dataType);
  }
  void appendArray(int rowNum, int colNum, int wordBit) {
    _array = std::make_shared<Array>(rowNum, colNum, wordBit);
  }
}; // end of Level

} // namespace ARCH

int main() {

  // NetworkGroup test
  ARCH::NetworkGroup n1(0, 1, 0, ARCH::STATIONARY, 3, 4, ARCH::INPUT);
  ARCH::NetworkGroup n2(1, 0, 0, ARCH::STATIONARY, 3, 4, ARCH::INPUT);
  ARCH::NetworkGroup n3(0, -1, 0, ARCH::STATIONARY, 3, 4, ARCH::INPUT);
  ARCH::NetworkGroup n4(-1, 0, 0, ARCH::STATIONARY, 3, 4, ARCH::INPUT);
  ARCH::NetworkGroup n5(1, 1, 0, ARCH::STATIONARY, 3, 4, ARCH::INPUT);
  ARCH::NetworkGroup n6(1, 1, 0, ARCH::STATIONARY, 4, 3, ARCH::INPUT);
  ARCH::NetworkGroup n7(-1, 1, 0, ARCH::STATIONARY, 3, 4, ARCH::INPUT);
  ARCH::NetworkGroup n8(-1, 1, 0, ARCH::STATIONARY, 4, 3, ARCH::INPUT);
  ARCH::NetworkGroup n9(1, -1, 0, ARCH::STATIONARY, 3, 4, ARCH::INPUT);
  ARCH::NetworkGroup n10(1, -1, 0, ARCH::STATIONARY, 4, 3, ARCH::INPUT);
  ARCH::NetworkGroup n11(-1, -1, 0, ARCH::STATIONARY, 3, 4, ARCH::INPUT);
  ARCH::NetworkGroup n12(-1, -1, 0, ARCH::STATIONARY, 4, 3, ARCH::INPUT);
  ARCH::NetworkGroup n111(0, 1, 0, ARCH::STATIONARY, 1, 1, ARCH::INPUT);
  ARCH::NetworkGroup n112(1, 0, 0, ARCH::STATIONARY, 1, 1, ARCH::INPUT);
  ARCH::NetworkGroup n113(0, -1, 0, ARCH::STATIONARY, 1, 1, ARCH::INPUT);
  ARCH::NetworkGroup n114(-1, 0, 0, ARCH::STATIONARY, 1, 1, ARCH::INPUT);
  ARCH::NetworkGroup n115(1, 1, 0, ARCH::STATIONARY, 1, 1, ARCH::INPUT);
  ARCH::NetworkGroup n116(1, 1, 0, ARCH::STATIONARY, 1, 1, ARCH::INPUT);
  ARCH::NetworkGroup n117(-1, 1, 0, ARCH::STATIONARY, 1, 1, ARCH::INPUT);
  ARCH::NetworkGroup n118(-1, 1, 0, ARCH::STATIONARY, 1, 1, ARCH::INPUT);
  ARCH::NetworkGroup n119(1, -1, 0, ARCH::STATIONARY, 1, 1, ARCH::INPUT);
  ARCH::NetworkGroup n1110(1, -1, 0, ARCH::STATIONARY, 1, 1, ARCH::INPUT);
  ARCH::NetworkGroup n1111(-1, -1, 0, ARCH::STATIONARY, 1, 1, ARCH::INPUT);
  ARCH::NetworkGroup n1112(-1, -1, 0, ARCH::STATIONARY, 1, 1, ARCH::INPUT);
  // level test
  ARCH::Level L1;
  L1.appendArray(3, 4, 16);
  L1.appendBuffer(ARCH::REG, ARCH::INPUT, 128, 16);
  L1.appendBuffer(ARCH::REG, ARCH::WEIGHT, 128, 16);
  L1.appendBuffer(ARCH::REG, ARCH::OUTPUT, 128, 16);
  L1.appendNetworkGroup(1, 0, 1, ARCH::SYSTOLIC, 3, 4, ARCH::INPUT);
  L1.appendNetworkGroup(0, 1, 1, ARCH::SYSTOLIC, 3, 4, ARCH::WEIGHT);
  L1.appendNetworkGroup(1, 0, 1, ARCH::STATIONARY, 3, 4, ARCH::OUTPUT);
  return 0;
}