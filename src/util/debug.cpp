#include "include/util/debug.h"
namespace DEBUG {

void printError(ErrorType errortype, std::string msg) {
  switch (errortype) {
  case TMATRIXERROR: {
    std::cout << "Error!Invalid T Matrix\n\t" << msg << std::endl;
    break;
  }
  case NETWORKOPERROR: {
    std::cout << "Error!Excute a error network operation:" << msg << std::endl;
    break;
  }
  case NETWORKFEATUREERROR: {
    std::cout << "Error!Invalid featureVec:" << msg << std::endl;
    break;
  }
  case REUSEVECSOLVEERROR: {
    std::cout << "Error!ReuseVec solve error:" << msg << std::endl;
    break;
  }
  case ITERATOREDGEERROR: {
    std::cout << "Error!The definition of edge edge is forbidden:" << msg
              << std::endl;
    break;
  }
  case REUSEVECNOTFITNETWORKARCHERROR: {
    std::cout << "Error!The reuseVec doesn't fit for arch:" << msg << std::endl;
    break;
  }
  case EDGEPEDIMERROR: {
    std::cout << "Error!Spatial dim has edge state:" << msg << std::endl;
    break;
  }
  case PEDIMOUTOFRANGE: {
    std::cout << "Error!PEDim out of Range:" << msg << std::endl;
    break;
  }
  }
}

void check(bool flag, ErrorType errortype, std::string msg) {
  if (!flag) {
    printError(errortype, msg);
    exit(-1);
  }
}
} // namespace DEBUG