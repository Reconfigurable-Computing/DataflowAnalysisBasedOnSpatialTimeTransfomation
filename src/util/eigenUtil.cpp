

#include "include/datastruct/mapping.h"
#include "include/util/eigenUtil.h"

typedef double valueType;

void printMatrix(
    Eigen::Matrix<valueType, Eigen::Dynamic, Eigen::Dynamic> matrix) {
  std::cout << matrix << '\n' << std::endl;
}

int GCD(int a, int b) {
  int temp = 0;
  while (b != 0) {
    temp = a;
    a = b;
    b = temp % b;
  }
  return a;
}

void compATinv(MAPPING::Transform &T, MAPPING::Access &A,
               std::vector<std::vector<Fraction>> &ATinv) {
  int TDimNum = T.getDimNum();
  int AColNum = A.getColNum();
  int ARowNum = A.getMatrix()->size() / A.getColNum();
  Eigen::Matrix<valueType, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>
      TMatrix =
          Eigen::Map<Eigen::Matrix<valueType, Eigen::Dynamic, Eigen::Dynamic>>(
              T.getMatrix()->data(), TDimNum, TDimNum)
              .transpose();
  Eigen::Matrix<valueType, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>
      AMatrix =
          Eigen::Map<Eigen::Matrix<valueType, Eigen::Dynamic, Eigen::Dynamic>>(
              A.getMatrix()->data(), AColNum, ARowNum)
              .transpose();
  printMatrix(TMatrix);
  printMatrix(AMatrix);
  printMatrix(TMatrix.inverse());
  printMatrix(AMatrix * TMatrix.inverse());
  Eigen::Matrix<valueType, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>
      eigenATinv = AMatrix * TMatrix.inverse();
  std::vector<Fraction> tmp;
  int value;
  for (int i = 0; i < eigenATinv.rows(); i++) {
    tmp.clear();
    for (int j = 0; j < eigenATinv.cols(); j++) {
      value = std::floor(eigenATinv(i, j));
      assert((value - eigenATinv(i, j)) < 1e-5);
      tmp.push_back(Fraction(value));
    }
    ATinv.push_back(tmp);
  }
}

std::pair<int, int>
findFirstNoZeroRow(std::vector<std::vector<Fraction>> &ATinv, int startRow) {
  int rowNum = ATinv.size();
  int colNum = ATinv[0].size();
  int curIndex;
  int minIndex = colNum;
  std::pair<int, int> ret = std::make_pair(-1, -1);
  for (int i = startRow; i < rowNum; i++) {
    curIndex = colNum;
    for (int j = 0; j < colNum; j++) {
      if (!ATinv[i][j].isZero()) {
        curIndex = j;
        break;
      }
    }
    if (curIndex < minIndex) {
      minIndex = curIndex;
      ret = std::make_pair(i, curIndex);
    }
  }
  return ret;
}
void rowSubRow(std::vector<std::vector<Fraction>> &ATinv, int row1, int row2,
               Fraction rate) {
  int colNum = ATinv[0].size();
  for (int i = 0; i < colNum; i++) {
    ATinv[row2][i] -= ATinv[row1][i] * rate;
  }
}
void rowDivNum(std::vector<std::vector<Fraction>> &ATinv, int row1,
               Fraction rate) {
  int colNum = ATinv[0].size();
  for (int i = 0; i < colNum; i++) {
    ATinv[row1][i] = ATinv[row1][i] / rate;
  }
}
void convertTrapezoidalMatrix(std::vector<std::vector<Fraction>> &ATinv,
                              std::vector<std::pair<int, int>> &majorVec) {
  int rowNum = ATinv.size();
  int colNum = ATinv[0].size();
  std::pair<int, int> NZIndex;
  for (int i = 0; i < rowNum; i++) {
    NZIndex = findFirstNoZeroRow(ATinv, i);
    if (i == 0) {
      assert(NZIndex.first != -1);
    }
    if (NZIndex.first == -1)
      break;
    majorVec.push_back(NZIndex);
    std::swap(ATinv[NZIndex.first], ATinv[i]);
    for (int j = i + 1; j < rowNum; j++) {
      rowSubRow(ATinv, i, j,
                ATinv[j][NZIndex.second] / ATinv[i][NZIndex.second]);
    }
  }
  int majorLen = majorVec.size();
  for (int i = 1; i < majorLen; i++) {
    NZIndex = majorVec[i];
    for (int j = 0; j < i; j++) {
      rowSubRow(ATinv, i, j,
                ATinv[j][NZIndex.second] / ATinv[i][NZIndex.second]);
    }
  }
  int rate;
  for (int i = 0; i < majorLen; i++) {
    NZIndex = majorVec[i];
    rowDivNum(ATinv, i, ATinv[i][NZIndex.second]);
  }
  for (int i = 0; i < rowNum; i++) {
    for (int j = 0; j < colNum; j++) {
      std::cout << ATinv[i][j].numerator << ' ';
    }
    std::cout << std::endl;
  }
}
void swapCol(std::vector<std::vector<Fraction>> &matrix, int col1, int col2) {
  int rowNum = matrix.size();

  int colNum = matrix[0].size();
  for (int i = 0; i < rowNum; i++) {
    std::swap(matrix[i][col1], matrix[i][col2]);
  }
}
void aggregateMajorVar(std::vector<std::pair<int, int>> &majorVec,
                       std::vector<int> &varSymFlag,
                       std::vector<std::vector<Fraction>> &ATinv) {
  int majorLen = majorVec.size();
  std::pair<int, int> NZIndex;
  for (int i = 0; i < majorLen; i++) {
    NZIndex = majorVec[i];
    if (NZIndex.second != i) {
      std::swap(varSymFlag[i], varSymFlag[NZIndex.second]);
      swapCol(ATinv, i, NZIndex.second);
    }
  }
}

void buildFractionReuseVec(std::vector<std::pair<int, int>> &majorVec,
                           std::vector<int> &varSymFlag,
                           std::vector<std::vector<Fraction>> &ATinv,
                           std::vector<std::vector<Fraction>> &reuseVec) {
  int rowNum = ATinv.size();
  int colNum = ATinv[0].size();
  int majorLen = majorVec.size();
  for (int i = 0; i < colNum - majorLen; i++) {
    for (int j = 0; j < colNum - majorLen; j++) {
      reuseVec[j][i] = -ATinv[i][majorLen + j];
    }
  }
  for (int j = majorLen; j < colNum; j++) {
    for (int i = 0; i < colNum - majorLen; i++) {
      reuseVec[i][j] = i == j - majorLen;
    }
  }
  for (int i = 0; i < colNum; i++) {
    if (varSymFlag[i] == i)
      continue;
    for (int j = i + 1; j < colNum; j++) {
      if (varSymFlag[j] == i) {
        std::swap(varSymFlag[i], varSymFlag[j]);
        swapCol(reuseVec, i, j);
        break;
      }
    }
  }
}
void buildIntReuseVec(std::vector<std::pair<int, int>> &majorVec,
                      std::vector<std::vector<Fraction>> &ATinv,
                      std::vector<std::vector<Fraction>> &reuseVec,
                      std::shared_ptr<std::vector<std::vector<int>>> ret) {
  int rowNum = ATinv.size();
  int colNum = ATinv[0].size();
  int majorLen = majorVec.size();
  int lcm;
  for (int i = 0; i < colNum - majorLen; i++) {
    lcm = 1;
    for (int j = 0; j < colNum; j++) {
      lcm = reuseVec[i][j].denominator * lcm *
            GCD(reuseVec[i][j].denominator, lcm);
    }
    for (int j = 0; j < colNum; j++) {
      (*ret)[i][j] =
          reuseVec[i][j].numerator * lcm / reuseVec[i][j].denominator;
    }
  }
}
std::shared_ptr<std::vector<std::vector<int>>>
compReuseVec(MAPPING::Transform &T, MAPPING::Access &A) {
  std::vector<std::vector<Fraction>> ATinv;
  compATinv(T, A, ATinv);
  // std::vector<std::vector<valueType>> ATinv =
  // std::vector<std::vector<valueType>>    { {1, 1, 1, 3},
  //    { 1,2,1,4 },
  //    { 2,3,2,7 } };
  // std::vector<std::vector<Fraction>> ATinv =
  // std::vector<std::vector<Fraction>>{ {Fraction(1), Fraction(2), Fraction(0),
  // Fraction (-2)},
  // { Fraction(0),Fraction(0),Fraction(2),Fraction(1) },
  // { Fraction(0),Fraction(0),Fraction(0),Fraction(0) } };
  // std::vector<std::vector<Fraction>> ATinv =
  // std::vector<std::vector<Fraction>>    { {1, 2, 0, -2},
  //{ 0,0,0,0 },
  //{ 0,0,0,0 } };
  int rowNum = ATinv.size();
  int colNum = ATinv[0].size();
  std::vector<std::pair<int, int>> majorVec;
  convertTrapezoidalMatrix(ATinv, majorVec);
  std::vector<int> varSymFlag(colNum);
  std::generate(varSymFlag.begin(), varSymFlag.end(), [] {
    static int i = 0;
    return i++;
  });
  int majorLen = majorVec.size();
  aggregateMajorVar(majorVec, varSymFlag, ATinv);
  std::vector<std::vector<Fraction>> reuseVec(colNum - majorLen,
                                              std::vector<Fraction>(colNum, 0));
  buildFractionReuseVec(majorVec, varSymFlag, ATinv, reuseVec);
  std::shared_ptr<std::vector<std::vector<int>>> ret =
      std::make_shared<std::vector<std::vector<int>>>(
          std::vector<std::vector<int>>(colNum - majorLen,
                                        std::vector<int>(colNum, 0)));
  buildIntReuseVec(majorVec, ATinv, reuseVec, ret);
  return ret;
}