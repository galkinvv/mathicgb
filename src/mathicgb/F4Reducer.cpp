#include "stdinc.h"
#include "F4Reducer.hpp"

#include "F4MatrixBuilder.hpp"
#include "F4MatrixReducer.hpp"

F4Reducer::F4Reducer(const PolyRing& ring, std::unique_ptr<Reducer> fallback):
  mFallback(std::move(fallback)), mRing(ring) {
}

std::unique_ptr<Poly> F4Reducer::classicReduce
(const Poly& poly, const PolyBasis& basis) {
  std::unique_ptr<Poly> p;
  p = mFallback->classicReduce(poly, basis);
  mSigStats = mFallback->sigStats();
  mClassicStats = mFallback->classicStats();
  return p;
}

std::unique_ptr<Poly> F4Reducer::classicTailReduce
(const Poly& poly, const PolyBasis& basis) {
  std::unique_ptr<Poly> p;
  p = mFallback->classicTailReduce(poly, basis);
  mSigStats = mFallback->sigStats();
  mClassicStats = mFallback->classicStats();
  return p;
}

std::unique_ptr<Poly> F4Reducer::classicReduceSPoly
(const Poly& a, const Poly& b, const PolyBasis& basis) {
  std::unique_ptr<Poly> p;
  {
    p = mFallback->classicReduceSPoly(a, b, basis);
    mSigStats = mFallback->sigStats();
    mClassicStats = mFallback->classicStats();
  }

  QuadMatrix qm;
  {
    F4MatrixBuilder builder(basis);
    builder.addTwoRowsForSPairToMatrix(a, b);
    builder.buildMatrixAndClear(qm);

    // there has to be something to reduce
    MATHICGB_ASSERT(qm.bottomLeft.rowCount() > 0);
  }

  SparseMatrix reduced;
  {
    F4MatrixReducer red;
    red.reduce(basis.ring(), qm, reduced);
    if (reduced.rowCount() > 0) {
      MATHICGB_ASSERT(reduced.rowCount() == 1);
      Poly q(&basis.ring());
      reduced.rowToPolynomial(0, qm.rightColumnMonomials, q);
      MATHICGB_ASSERT(q == *p);
    } else
      MATHICGB_ASSERT(p->isZero());
  }

  return p;
}

Poly* F4Reducer::regularReduce
(const_monomial sig,
 const_monomial multiple,
 size_t basisElement,
 const GroebnerBasis& basis) {
  Poly* p = mFallback->regularReduce(sig, multiple, basisElement, basis);
  mSigStats = mFallback->sigStats();
  mClassicStats = mFallback->classicStats();
  return p;
}

std::string F4Reducer::description() const {
  return "F4 reducer";
}

size_t F4Reducer::getMemoryUse() const {
  return 0; // @todo: implement
}