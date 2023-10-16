#pragma once
#include "include/datastruct/mapping.h"
#include <memory>
#include <vector>
struct Base {
  int baseData[3];
  int baseCompCycle;
  Base() {
    for (int i = 0; i < 3; i++) {
      baseData[i] = 1;
    }
    baseCompCycle = 1;
  }
  Base(int newBaseCompCycle, int newBaseData[3]) {
    baseCompCycle = newBaseCompCycle;
    for (int i = 0; i < 3; i++) {
      baseData[i] = newBaseData[i];
    }
  }
};
struct AnalyzerResult {
  int uniqueVolumn[3]; // input weight output
  int totalVolumn[3];
  int reuseVolumn[3];
  int requiredDataSize[3];
  int initDelay[3];
  int initTimes;
  int stableDelay[4];
  int delay;
  int compCycle;
  int occTimes;
  float compRate;
  int activePEMultTimeNum;
  int totalPEMultTimeNum;
  float PEUtilRate;
  int totalBandWidth[3];
  std::vector<std::shared_ptr<AnalyzerResult>> subLevelResultVec;
  AnalyzerResult() { reset(); }
  void reset() {
    for (int i = 0; i < 3; i++) {
      uniqueVolumn[i] = 0;
      totalVolumn[i] = 0;
      reuseVolumn[i] = 0;
      requiredDataSize[i] = 0;
      initDelay[i] = 0;
      stableDelay[i] = 0;
      totalBandWidth[i] = 0;
    }
    stableDelay[3] = 0;
    initTimes = 0;
    delay = 0;
    occTimes = 0;
    compCycle = 0;
    compRate = 0;
    activePEMultTimeNum = 0;
    totalPEMultTimeNum = 0;
    PEUtilRate = 0;
    subLevelResultVec.clear();
  }
  AnalyzerResult &operator+=(AnalyzerResult &other) {
    for (int i = 0; i < 3; i++) {
      uniqueVolumn[i] += other.uniqueVolumn[i] * other.occTimes;
      totalVolumn[i] += other.totalVolumn[i] * other.occTimes;
      reuseVolumn[i] += other.reuseVolumn[i] * other.occTimes;
      requiredDataSize[i] =
          std::max(requiredDataSize[i], other.requiredDataSize[i]);
      initDelay[i] = std::max(initDelay[i], other.initDelay[i]);
      stableDelay[i] = std::max(stableDelay[i], other.stableDelay[i]);
      totalBandWidth[i] = 0;
    }
    stableDelay[3] = std::max(stableDelay[3], other.stableDelay[3]);
    initTimes = std::max(initTimes, other.initTimes);
    delay = std::max(delay, other.delay);
    compRate += other.delay * other.occTimes;
    compCycle += other.compCycle * other.occTimes;
    occTimes += other.occTimes;
    activePEMultTimeNum += other.activePEMultTimeNum * other.occTimes;
    totalPEMultTimeNum += other.totalPEMultTimeNum * other.occTimes;
    PEUtilRate = 0;
    return *this;
  }
};

struct TransformSearchResult {
  MAPPING::Transform _T;
  std::shared_ptr<AnalyzerResult> _result;
  TransformSearchResult(MAPPING::Transform &T,
                        std::shared_ptr<AnalyzerResult> &result)
      : _result(result), _T(T) {}
};

struct MultiLevelTransformSearchResult {
  std::vector<TransformSearchResult> _transformSearchResult;
  MultiLevelTransformSearchResult(){};
  void addResult(MAPPING::Transform T,
                 std::shared_ptr<AnalyzerResult> &result) {
    _transformSearchResult.emplace_back(T, result);
  }
};