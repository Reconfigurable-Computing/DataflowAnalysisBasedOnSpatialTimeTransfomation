#include "include/searchEngine/transformSearchEngine.h"
namespace DSE {
void TransformSearchEngine::setMatrixAsOne(MAPPING::Transform &T,
                                           std::vector<int> &permute,
                                           int start) {
  int dimNum = _coupledVarVec.size();
  for (int i = start; i < dimNum; i++) {
    T.setValue(i, permute[i], 1);
  }
}

void TransformSearchEngine::generateTransformMatrix(
    std::vector<int> &permute, std::vector<MAPPING::Transform> &Tvec) {
  int dimNum = _coupledVarVec.size();
  int count = 0;

  if (_spatialDimNum == 0) {
    Tvec.emplace_back(dimNum);
    setMatrixAsOne(Tvec[count++], permute, 0);
  } else if (_spatialDimNum == 1) {
    int PEXindex = permute[0];
    if (dimNum == 1) {
      Tvec.emplace_back(dimNum);
      Tvec[count++].setValue(0, PEXindex, 1);
    } else {
      int majorTimeIndex = permute[1];
      Tvec.emplace_back(dimNum);
      Tvec[count].setValue(0, PEXindex, 1);
      Tvec[count].setValue(1, majorTimeIndex, 1);
      setMatrixAsOne(Tvec[count++], permute, 2);

      Tvec.emplace_back(dimNum);
      Tvec[count].setValue(0, PEXindex, 1);
      Tvec[count].setValue(1, majorTimeIndex, 1);
      Tvec[count].setValue(1, PEXindex, 1);
      setMatrixAsOne(Tvec[count++], permute, 2);
    }
  } else {
    int PEXindex = permute[0];
    int PEYindex = permute[1];

    if (dimNum == 2) {
      Tvec.emplace_back(dimNum);
      Tvec[count].setValue(0, PEXindex, 1);
      Tvec[count++].setValue(1, PEYindex, 1);
    } else {
      int majorTimeIndex = permute[2];
      Tvec.emplace_back(dimNum);
      Tvec[count].setValue(0, PEXindex, 1);
      Tvec[count].setValue(1, PEYindex, 1);
      Tvec[count].setValue(2, majorTimeIndex, 1);
      setMatrixAsOne(Tvec[count++], permute, 3);

      Tvec.emplace_back(dimNum);
      Tvec[count].setValue(0, PEXindex, 1);
      Tvec[count].setValue(1, PEYindex, 1);
      Tvec[count].setValue(2, majorTimeIndex, 1);
      Tvec[count].setValue(2, PEXindex, 1);
      setMatrixAsOne(Tvec[count++], permute, 3);

      Tvec.emplace_back(dimNum);
      Tvec[count].setValue(0, PEXindex, 1);
      Tvec[count].setValue(1, PEYindex, 1);
      Tvec[count].setValue(2, majorTimeIndex, 1);
      Tvec[count].setValue(2, PEYindex, 1);
      setMatrixAsOne(Tvec[count++], permute, 3);

      Tvec.emplace_back(dimNum);
      Tvec[count].setValue(0, PEXindex, 1);
      Tvec[count].setValue(1, PEYindex, 1);
      Tvec[count].setValue(2, majorTimeIndex, 1);
      Tvec[count].setValue(2, PEXindex, 1);
      Tvec[count].setValue(2, PEYindex, 1);
      setMatrixAsOne(Tvec[count++], permute, 3);
    }
  }
}

void TransformSearchEngine::generateAllTransformMatrix(
    int level, MultLevelAnalyzer &multanalysis) {
  int dimNum = _coupledVarVec.size();
  std::vector<MAPPING::Transform> TVecTmp;
  assert(dimNum != 0);

  std::vector<int> permute(dimNum, 0);
  std::iota(permute.begin(), permute.end(), 0);
  do {
    for (int i = 0; i < dimNum; i++) {
      std::cout << permute[i] << ' ';
    }
    std::cout << std::endl;
    generateTransformMatrix(permute, TVecTmp);
    for (auto &T : TVecTmp) {
      std::cout << std::endl;
      std::cout << T.to_string();
      std::cout << std::endl;
      if (multanalysis.changeT(level, _coupledVarVec, _spatialDimNum, T,
                               true)) {
        _TVec.push_back(T);
      }
    }
    TVecTmp.clear();
  } while (std::next_permutation(permute.begin(), permute.end()));
  std::cout << _TVec.size() << std::endl;
}
} // namespace DSE