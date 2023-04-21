#pragma once

#include <algorithm>
#include <assert.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
namespace DEBUG {
typedef enum {
  TMATRIXERROR,
  NETWORKOPERROR,
  NETWORKFEATUREERROR,
  REUSEVECSOLVEERROR,
  ITERATOREDGEERROR,
  REUSEVECNOTFITNETWORKARCHERROR,
  EDGEPEDIMERROR,
  PEDIMOUTOFRANGE
} ErrorType;
template <typename T> std::string vec2string(std::vector<T> &vec) {
  std::string ret;
  int len = vec.size();
  ret += " vector: ";
  for (int i = 0; i < len - 1; i++) {
    ret += std::to_string(vec[i]) + ", ";
  }
  ret += std::to_string(vec[len - 1]);
  return ret;
}
void printError(ErrorType errortype, std::string msg);

void check(bool flag, ErrorType errortype, std::string msg = "");
} // namespace DEBUG
