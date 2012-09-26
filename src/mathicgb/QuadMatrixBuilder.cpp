#include "stdinc.h"
#include "QuadMatrixBuilder.hpp"

#include "FreeModuleOrder.hpp"
#include <mathic.h>
#include <sstream>

namespace {
  /// Creates a column and updates the associated data structures that
  /// are passed in. Copies mono - ownership is not taken over. The
  /// purpose of this function is to avoid code duplication. It is a
  /// template in order to avoid referring to private types of
  /// QuadMatrixBuilder.
  template<class ToMono, class ToCol>
  QuadMatrixBuilder::ColIndex createCol
  (const_monomial mono,
   SparseMatrix& top,
   SparseMatrix& bottom,
   ToMono& toMonomial,
   ToCol& toCol,
   const PolyRing& ring,
   const bool left)
  {
    MATHICGB_ASSERT(top.colCount() == bottom.colCount());
    MATHICGB_ASSERT(toMonomial.size() == bottom.colCount());
    MATHICGB_ASSERT(toCol.find(mono) == toCol.end());

    const QuadMatrixBuilder::ColIndex colCount = top.colCount();
    if (colCount == std::numeric_limits<QuadMatrixBuilder::ColIndex>::max())
      throw std::overflow_error("Too many columns in QuadMatrixBuilder");

    toMonomial.reserve(toMonomial.size() + 1);
    monomial copied = ring.allocMonomial();
    ring.monomialCopy(mono, copied);
    try {
      toCol.insert(std::make_pair
                   (copied,
                    QuadMatrixBuilder::LeftRightColIndex(colCount, left)));
    } catch (...) {
      ring.freeMonomial(copied);
      throw;
    }
    toMonomial.push_back(copied); // no throw due to reserve

    top.ensureAtLeastThisManyColumns(colCount + 1);
    bottom.ensureAtLeastThisManyColumns(colCount + 1);
    return colCount;
  }
}

QuadMatrixBuilder::ColIndex QuadMatrixBuilder::createColumnLeft
(const_monomial monomialToBeCopied) {
  return createCol
    (monomialToBeCopied,
     mTopLeft,
     mBottomLeft,
     mMonomialsLeft,
     mMonomialToCol,
     ring(),
     true);
  MATHICGB_ASSERT(mMonomialToCol.size() == leftColCount() + rightColCount());
  MATHICGB_ASSERT
    (findColumn(monomialToBeCopied).leftIndex() == leftColCount() - 1);
}

QuadMatrixBuilder::ColIndex QuadMatrixBuilder::createColumnRight
(const_monomial monomialToBeCopied) {
  return createCol
    (monomialToBeCopied,
     mTopRight,
     mBottomRight,
     mMonomialsRight,
     mMonomialToCol,
     ring(),
     false);
  MATHICGB_ASSERT(mMonomialToCol.size() == leftColCount() + rightColCount());
  MATHICGB_ASSERT
    (findColumn(monomialToBeCopied).rightIndex() == rightColCount() - 1);
}

namespace {
  class ColumnComparer {
  public:
    ColumnComparer(const FreeModuleOrder& order): mOrder(order) {}

    typedef QuadMatrixBuilder::ColIndex ColIndex;
    typedef std::pair<monomial, ColIndex> Pair;
    bool operator()(const Pair& a, const Pair b) const {
      return mOrder.signatureCompare(a.first, b.first) == GT;
    }

  private:
    const FreeModuleOrder& mOrder;
  };

  // The purpose of this function is to avoid code duplication for
  // left/right variants.
  void sortColumns
  (const FreeModuleOrder& order,
   std::vector<monomial>& monomials,
   SparseMatrix& topMatrix,
   SparseMatrix& bottomMatrix)
  {
    typedef SparseMatrix::ColIndex ColIndex;
    MATHICGB_ASSERT(topMatrix.colCount() == bottomMatrix.colCount());
    const ColIndex colCount = topMatrix.colCount();

    // Monomial needs to be non-const as we are going to put these
    // monomials back into the vector of monomials which is not const.
    std::vector<std::pair<monomial, ColIndex> > columns;
    columns.reserve(colCount);
    for (size_t col = 0; col < colCount; ++col)
      columns.push_back(std::make_pair(monomials[col], col));
    std::sort(columns.begin(), columns.end(), ColumnComparer(order));

    // Reorder monomials associated to each column to be in sorted order
    MATHICGB_ASSERT(columns.size() == colCount);
    MATHICGB_ASSERT(monomials.size() == colCount);
    for (size_t col = 0; col < colCount; ++col) {
      MATHICGB_ASSERT(col == 0 ||  order.signatureCompare
                      (columns[col - 1].first, columns[col].first) == GT);
      monomials[col] = columns[col].first;
    }

    // Construct permutation of indices to match permutation of monomials
    std::vector<ColIndex> permutation(colCount);
    for (size_t col = 0; col < colCount; ++col) {
      // The monomial for column columns[col].second is now the
      // monomial for col, so we need the inverse map for indices.
      permutation[columns[col].second] = col;
    }

    // Change indices to match the permutation done on the columns
    typedef SparseMatrix::AllColIndicesIterator Iter;
    for (int doTop = 0; doTop < 2; ++doTop) {
      SparseMatrix& matrix = doTop ? topMatrix : bottomMatrix;
      Iter it = matrix.allColIndicesBegin();
      Iter end = matrix.allColIndicesEnd();
      for (; it != end; ++it) {
        MATHICGB_ASSERT(*it < colCount);
        *it = permutation[*it];
      }
    }
  }  
}

void QuadMatrixBuilder::sortColumnsLeft(const FreeModuleOrder& order) {
  sortColumns(order, mMonomialsLeft, mTopLeft, mBottomLeft);
}

void QuadMatrixBuilder::sortColumnsRight(const FreeModuleOrder& order) {
  sortColumns(order, mMonomialsRight, mTopRight, mBottomRight);
}

void QuadMatrixBuilder::print(std::ostream& out) const {
  mathic::ColumnPrinter printer;
  printer.addColumn(true, "", "");
  printer.addColumn(true, " | ", "");

  // column monomials
  out << "Left columns:";
  for (ColIndex leftCol = 0; leftCol < leftColCount(); ++leftCol) {
    out << ' ';
    ring().monomialDisplay(out, monomialOfLeftCol(leftCol), false, true);
  }

  out << "\nRight columns:";
  for (ColIndex rightCol = 0; rightCol < rightColCount(); ++rightCol) {
    out << ' ';
    ring().monomialDisplay(out, monomialOfRightCol(rightCol), false, true);
  }
  out << '\n';

  // left side
  topLeft().print(printer[0]);
  printer[0] << '\n';
  bottomLeft().print(printer[0]);

  // right side
  topRight().print(printer[1]);
  printer[1] << '\n';
  bottomRight().print(printer[1]);

  out << printer;
}

// String representation intended for debugging.
std::string QuadMatrixBuilder::toString() const {
  std::ostringstream out;
  print(out);
  return out.str();
}
