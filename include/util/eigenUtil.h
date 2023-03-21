#pragma once
#include "Eigen/Core"
#include "Eigen/Dense"
#include "include/datastruct/mapping.h"
typedef double valueType;

void printMatrix(
    Eigen::Matrix<valueType, Eigen::Dynamic, Eigen::Dynamic> matrix);
int GCD(int a, int b);

struct Fraction {
  int numerator, denominator;
  Fraction() : numerator(0), denominator(0) {}
  Fraction(int nume, int deno) : numerator(nume), denominator(deno) {}
  Fraction(int nume) : numerator(nume), denominator(1) {}

  void simplify(Fraction &tmp) {
    int gcd = GCD(tmp.numerator, tmp.denominator);
    tmp.numerator /= gcd;
    tmp.denominator /= gcd;
  }
  Fraction operator+(Fraction other) {
    Fraction tmp;
    tmp.numerator = this->numerator * other.denominator +
                    other.numerator * this->denominator;
    tmp.denominator = this->denominator * other.denominator;
    simplify(tmp);
    return tmp;
  }
  Fraction operator-(Fraction other) {
    Fraction tmp;
    tmp.numerator = this->numerator * other.denominator -
                    other.numerator * this->denominator;
    tmp.denominator = this->denominator * other.denominator;
    simplify(tmp);
    return tmp;
  }
  Fraction operator/(Fraction other) {
    Fraction tmp;
    tmp.numerator = this->numerator * other.denominator;
    tmp.denominator = this->denominator * other.numerator;
    simplify(tmp);
    return tmp;
  }
  Fraction operator*(Fraction other) {
    Fraction tmp;
    tmp.numerator = this->numerator * other.numerator;
    tmp.denominator = this->denominator * other.denominator;
    simplify(tmp);
    return tmp;
  }
  Fraction &operator-=(Fraction other) {
    Fraction tmp;
    this->numerator = this->numerator * other.denominator -
                      other.numerator * this->denominator;
    this->denominator = this->denominator * other.denominator;
    simplify(*this);
    return *this;
  }
  Fraction operator-() {
    Fraction tmp;
    tmp.numerator = -this->numerator;
    tmp.denominator = this->denominator;
    return tmp;
  }
  bool isZero() { return numerator == 0; }
};

void compATinv(MAPPING::Transform &T, MAPPING::Access &A,
               std::vector<std::vector<Fraction>> &ATinv);

std::pair<int, int>
findFirstNoZeroRow(std::vector<std::vector<Fraction>> &ATinv, int startRow);
void rowSubRow(std::vector<std::vector<Fraction>> &ATinv, int row1, int row2,
               Fraction rate);
void rowDivNum(std::vector<std::vector<Fraction>> &ATinv, int row1,
               Fraction rate);
void convertTrapezoidalMatrix(std::vector<std::vector<Fraction>> &ATinv,
                              std::vector<std::pair<int, int>> &majorVec);
void swapCol(std::vector<std::vector<Fraction>> &matrix, int col1, int col2);
void aggregateMajorVar(std::vector<std::pair<int, int>> &majorVec,
                       std::vector<int> &varSymFlag,
                       std::vector<std::vector<Fraction>> &ATinv);

void buildFractionReuseVec(std::vector<std::pair<int, int>> &majorVec,
                           std::vector<int> &varSymFlag,
                           std::vector<std::vector<Fraction>> &ATinv,
                           std::vector<std::vector<Fraction>> &reuseVec);
void buildIntReuseVec(std::vector<std::pair<int, int>> &majorVec,
                      std::vector<std::vector<Fraction>> &ATinv,
                      std::vector<std::vector<Fraction>> &reuseVec,
                      std::shared_ptr<std::vector<std::vector<int>>> ret);
std::shared_ptr<std::vector<std::vector<int>>>
compReuseVec(MAPPING::Transform &T, MAPPING::Access &A);