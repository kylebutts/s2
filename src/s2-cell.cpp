
#include <cstdint>
#include <vector>

#include "s2/s2cell_id.h"
#include "s2/s2latlng.h"

#include <Rcpp.h>
using namespace Rcpp;

class S2CellOperatorException: public std::runtime_error {
public:
  S2CellOperatorException(std::string msg): std::runtime_error(msg.c_str()) {}
};

template<class VectorType, class ScalarType>
class UnaryS2CellOperator {
public:
  VectorType processVector(Rcpp::NumericVector cellIdVector) {
    VectorType output(cellIdVector.size());

    Rcpp::IntegerVector problemId;
    Rcpp::CharacterVector problems;

    for (R_xlen_t i = 0; i < cellIdVector.size(); i++) {
      if ((i % 1000) == 0) {
        Rcpp::checkUserInterrupt();
      }

      if (NumericVector::is_na(cellIdVector[i])) {
        output[i] = VectorType::get_na();
      } else {
        try {
          output[i] = this->processCell(*((uint64_t*) &(cellIdVector[i])), i);
        } catch (S2CellOperatorException& e) {
          output[i] = VectorType::get_na();
          problemId.push_back(i);
          problems.push_back(e.what());
        }
      }
    }

    if (problemId.size() > 0) {
      Rcpp::Environment s2NS = Rcpp::Environment::namespace_env("s2");
      Rcpp::Function stopProblems = s2NS["stop_problems_process"];
      stopProblems(problemId, problems);
    }

    return output;
  }

  virtual ScalarType processCell(uint64_t cellId, R_xlen_t i) = 0;
};

// [[Rcpp::export]]
NumericVector cpp_s2_cell_from_string(CharacterVector cellString) {
  R_xlen_t size = cellString.size();
  NumericVector cellId(size);
  double* ptrDouble = REAL(cellId);
  uint64_t* ptrCellId = (uint64_t*) ptrDouble;

  for (R_xlen_t i = 0; i < size; i++) {
    if (CharacterVector::is_na(cellString[i])) {
      ptrDouble[i] = NA_REAL;
    } else {
      ptrCellId[i] = S2CellId::FromToken(as<std::string>(cellString[i])).id();
    }
  }
  
  cellId.attr("class") = CharacterVector::create("s2_cell", "wk_vctr");
  return cellId;
}

// [[Rcpp::export]]
NumericVector cpp_s2_cell_from_lnglat(List lnglat) {
    NumericVector lng = lnglat[0];
    NumericVector lat = lnglat[1];
    R_xlen_t size = lng.size();
    NumericVector cellId(size);
    double* ptrDouble = REAL(cellId);
    uint64_t* ptrCellId = (uint64_t*) ptrDouble;

    for (R_xlen_t i = 0; i < size; i++) {
      if (NumericVector::is_na(lng[i]) || NumericVector::is_na(lat[i])) {
          ptrDouble[i] = NA_REAL;
      } else {
          S2LatLng ll = S2LatLng::FromDegrees(lat[i], lng[i]).Normalized();
          ptrCellId[i] = S2CellId(ll).id();
      }
    }

    cellId.attr("class") = CharacterVector::create("s2_cell", "wk_vctr");
    return cellId;
}

// [[Rcpp::export]]
CharacterVector cpp_s2_cell_to_string(NumericVector cellIdVector) {
  class Op: public UnaryS2CellOperator<CharacterVector, std::string> {
    std::string processCell(uint64_t cellId, R_xlen_t i) {
      return S2CellId(cellId).ToToken();
    }
  };

  Op op;
  return op.processVector(cellIdVector);
}

// [[Rcpp::export]]
LogicalVector cpp_s2_cell_is_valid(NumericVector cellIdVector) {
  class Op: public UnaryS2CellOperator<LogicalVector, int> {
    int processCell(uint64_t cellId, R_xlen_t i) {
      return S2CellId(cellId).is_valid();
    }
  };

  Op op;
  return op.processVector(cellIdVector);
}
