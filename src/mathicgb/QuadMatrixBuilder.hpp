#ifndef MATHICGB_QUAD_MATRIX_BUILDER_GUARD
#define MATHICGB_QUAD_MATRIX_BUILDER_GUARD

#define MATHICGB_USE_QUADMATRIX_STD_HASH

#include "MonomialMap.hpp"

#include "SparseMatrix.hpp"
#include "PolyRing.hpp"
#include <vector>
#include <map>
#include <limits>
#include <string>
#include <ostream>
#include <memtailor.h>
#ifdef MATHICGB_USE_QUADMATRIX_STD_HASH
#include <unordered_map>
#endif
class FreeModuleOrder;
class QuadMatrix;

/** Builder for QuadMatrix. This is not quite the builder pattern in
  that the interface is not virtual and the implementation cannot be
  swapped out - it only follows the builder pattern in that it is a
  class that allows step-wise construction of a final product. */
class QuadMatrixBuilder {
 public:
  typedef SparseMatrix::RowIndex RowIndex;
  typedef SparseMatrix::ColIndex ColIndex;
  typedef SparseMatrix::Scalar Scalar;

  QuadMatrixBuilder(const PolyRing& ring, size_t memoryQuantum = 0);

  /// Inserts the rows from builder. To avoid an assert either the matrix must
  /// have no column monomials specified or the monomials that are specified
  /// must match exactly to the column monomials for this object --- including
  /// the ordering of the monomials.
  void takeRowsFrom(QuadMatrix&& matrix);

  /// The index of a column that can be either on the left or the
  /// right side. The largest representable ColIndex is an invalid
  /// index. This is the default value. The only allowed method to
  /// call for an invalid index is valid().
  class LeftRightColIndex {
  public:
    LeftRightColIndex():
      mRawIndex(std::numeric_limits<ColIndex>::max()), mLeft(false) {}
    LeftRightColIndex(ColIndex index, bool left):
      mRawIndex(index), mLeft(left) {
    }

    ColIndex leftIndex() const {
      MATHICGB_ASSERT(left());
      return index();
    }

    ColIndex rightIndex() const {
      MATHICGB_ASSERT(right());
      return index();
    }

    /// Use leftIndex() or rightIndex() instead if you know what side
    /// you are expecting, as this does an assert on your expectation.
    ColIndex index() const {
      MATHICGB_ASSERT(valid());
      return mRawIndex;
    }

    bool left() const {
      MATHICGB_ASSERT(valid());
      return mLeft;
    }

    bool right() const {
      MATHICGB_ASSERT(valid());
      return !left();
    }

    bool valid() const {
      return mRawIndex != std::numeric_limits<ColIndex>::max();
    }

    bool operator==(const LeftRightColIndex& index) const {
      return mRawIndex == index.mRawIndex && mLeft == index.mLeft;
    }

    bool operator!=(const LeftRightColIndex& index) const {
      return !(*this == index);
    }

  private:
    ColIndex mRawIndex;
    bool mLeft;
  };

  ColIndex leftColCount() const {
    return static_cast<ColIndex>(mMonomialsLeft.size());
  }

  ColIndex rightColCount() const {
    return static_cast<ColIndex>(mMonomialsRight.size());
  }


  // **** Appending entries to top matrices.
  // Same interface as SparseMatrix except with two matrices and here
  // you have to create columns before you can use them.

  void appendEntryTopLeft(ColIndex col, Scalar scalar) {
    mTopLeft.appendEntry(col, scalar);
  }

  void appendEntryTopRight(ColIndex col, Scalar scalar) {
    mTopRight.appendEntry(col, scalar);
  }

  void appendEntryTop(LeftRightColIndex col, Scalar scalar) {
    MATHICGB_ASSERT(col.valid());
    if (col.left())
      appendEntryTopLeft(col.leftIndex(), scalar);
    else
      appendEntryTopRight(col.rightIndex(), scalar);
  }

  void rowDoneTopLeftAndRight() {
    mTopLeft.rowDone();
    mTopRight.rowDone();
  }

  // **** Appending entries to bottom matrices
  // Same interface as SparseMatrix except with two matrices and here
  // you have to create columns before you can use them.

  void appendEntryBottomLeft(ColIndex col, Scalar scalar) {
    mBottomLeft.appendEntry(col, scalar);
  }

  void appendEntryBottomRight(ColIndex col, Scalar scalar) {
    mBottomRight.appendEntry(col, scalar);
  }

  void appendEntryBottom(LeftRightColIndex col, Scalar scalar) {
    MATHICGB_ASSERT(col.valid());
    if (col.left())
      appendEntryBottomLeft(col.leftIndex(), scalar);
    else
      appendEntryBottomRight(col.rightIndex(), scalar);
  }

  void rowDoneBottomLeftAndRight() {
    mBottomLeft.rowDone();
    mBottomRight.rowDone();
  }

  // *** Creating and reordering columns
  // You have to create columns before you can append entries in those columns.
  // All passed in monomials are copied so that ownership of the memory is
  // not taken over. The creation methods return a LeftRightColIndex instead
  // of just a ColIndex to allow more of a chance for asserts to catch errors
  // and to avoid the need for the client to immediately construct a
  // LeftRightColIndex based on the return value.

  /** Creates a new column associated to the monomial
    monomialToBeCopied to the left matrices. There must not already
    exist a column for this monomial on the left or on the right. */
  LeftRightColIndex createColumnLeft(const_monomial monomialToBeCopied);

  /** Creates a new column associated to the monomial monomialToBeCopied
    to the right matrices. There must not already exist a column for
    this monomial on the left or on the right. */
  LeftRightColIndex createColumnRight(const_monomial monomialToBeCopied);

  /// As calling sortColumnsLeft() and sortColumnsRight(), but performs
  /// the operations in parallel using up to threadCount threads.
  void sortColumnsLeftRightParallel
    (const FreeModuleOrder& order, int threadCount);

  /** Sorts the left columns to be decreasing with respect to
    order. Also updates the column indices already in the matrix to
    reflect the new ordering. */
  void sortColumnsLeft(const FreeModuleOrder& order);

  /** Sorts the right columns to be decreasing with respect to
    order. Also updates the column indices already in the matrix to
    reflect the new ordering. */
  void sortColumnsRight(const FreeModuleOrder& order);


  // *** Querying columns

  typedef const MonomialMap<LeftRightColIndex>::Reader ColReader;
  const MonomialMap<LeftRightColIndex>& columnToIndexMap() const {
    return mMonomialToCol;
  }

  /** Returns a column for the findThis monomial. Searches on both the
    left and right side. Returns an invalid index if no such column
    exists. */
  LeftRightColIndex findColumn(const_monomial findThis) const {
    auto it = ColReader(mMonomialToCol).find(findThis);
    if (it != 0)
      return *it;
    else
      return LeftRightColIndex();
  }

  /// As findColumn(), but looks for a*b. This is faster than computing a*b
  /// and then looking that up.
  LeftRightColIndex findColumnProduct(
    const const_monomial a,
    const const_monomial b
  ) const {
    const auto it = ColReader(mMonomialToCol).findProduct(a, b);
    if (it != 0)
      return *it;
    else
      return LeftRightColIndex();
  }

  /// As findColumnProduct(), but looks for a1*b and a2*b at the same time.
  MATHICGB_INLINE std::pair<LeftRightColIndex, LeftRightColIndex>
  findTwoColumnsProduct(
    const const_monomial a1,
    const const_monomial a2,
    const const_monomial b
  ) const {
    const auto it = ColReader(mMonomialToCol).findTwoProducts(a1, a2, b);
    return std::make_pair(
      it.first != 0 ? *it.first : LeftRightColIndex(),
      it.second != 0 ? *it.second : LeftRightColIndex());
  }

  const_monomial monomialOfLeftCol(ColIndex col) const {
    MATHICGB_ASSERT(col < mMonomialsLeft.size());
    return mMonomialsLeft[col];
  }

  const_monomial monomialOfRightCol(ColIndex col) const {
    MATHICGB_ASSERT(col < mMonomialsRight.size());
    return mMonomialsRight[col];
  }

  const_monomial monomialOfCol(LeftRightColIndex col) const {
    MATHICGB_ASSERT(col.valid());
    if (col.left())
      return monomialOfLeftCol(col.leftIndex());
    else
      return monomialOfRightCol(col.rightIndex());
  }

  const SparseMatrix& topLeft() const {return mTopLeft;}
  const SparseMatrix& topRight() const {return mTopRight;}
  const SparseMatrix& bottomLeft() const {return mBottomLeft;}
  const SparseMatrix& bottomRight() const {return mBottomRight;}

  // String representation intended for debugging.
  void print(std::ostream& out) const;

  // String representation intended for debugging.
  std::string toString() const;

  const PolyRing& ring() const {return mMonomialToCol.ring();}

  /// Returns the built matrix and sets the builder to a state
  /// with no columns and no rows.
  QuadMatrix buildMatrixAndClear();

private:
  typedef std::vector<monomial> MonomialsType;
  MonomialsType mMonomialsLeft; /// stores one monomial per left column
  MonomialsType mMonomialsRight; /// stores one monomial per right column

  /// Used for fast determination of which column has a given monomial.
  MonomialMap<LeftRightColIndex> mMonomialToCol;

  SparseMatrix mTopLeft;
  SparseMatrix mTopRight;
  SparseMatrix mBottomLeft;
  SparseMatrix mBottomRight;
};

#endif
