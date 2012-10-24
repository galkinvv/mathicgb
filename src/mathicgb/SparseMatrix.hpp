#ifndef MATHICGB_SPARSE_MATRIX_GUARD
#define MATHICGB_SPARSE_MATRIX_GUARD

#include "RawVector.hpp"
#include "PolyRing.hpp"
#include <mathic.h>
#include <vector>
#include <ostream>
#include <limits>
#include <sstream>
#include <string>
#include <iterator>
class Poly;

/** A class that implements a sparse matrix.

These are the mathematical concepts involved:
  Sparse matrix: a sequence of sparse rows.
  Sparse row: a seqence of entries.
  Entry: a pair (i,s) where i is a column index and s is a scalar.

You add a row by adding all entries in the row and then calling
rowDone(). You cannot add entries to a row once it has been
created, so in that sense this class is append-only. However, you
are free to change the indices and the scalars in the entries that
are already there. Entries are not automatically reordered by this
class, so your rows will be in increasing order of index only if
you make them like that.

Adding an entry or a row can invalidate all pointers/references to
entries in the matrix and all iterators. This is true even if the
entry has been added but it has not been put in a new row yet by
calling rowDone.

There is no special treatment of entries whose scalar is zero. For
example they still count as entries in relation to entryCount().

Currently this is not a template class so you can get by without
using the typedefs offered, for example using uint16 instead of
SparseMatrix::Scalar. Please use the typedefs to make it easier to
support a wider range of types of matrices in future.
*/
class SparseMatrix {
public:
  typedef size_t RowIndex;
  typedef uint32 ColIndex;
  typedef uint16 Scalar;
  class ConstRowIterator;

  /// Construct a matrix with no rows.
  SparseMatrix(ColIndex colCount = 0): mColCount(colCount) {}

  SparseMatrix(SparseMatrix&& matrix):
    mColIndices(std::move(matrix.mColIndices)),
    mEntries(std::move(matrix.mEntries)),
    mRows(std::move(matrix.mRows)),
    mColCount(matrix.mColCount)
  {
  }

  SparseMatrix& operator=(SparseMatrix&& matrix) {
    this->~SparseMatrix();
    new (this) SparseMatrix(std::move(matrix));
    return *this;
  }

  ~SparseMatrix() {
    delete[] mColIndices.releaseMemory();
    delete[] mEntries.releaseMemory();
  }

  void swap(SparseMatrix& matrix);

  void clear(ColIndex newColCount = 0);

  RowIndex rowCount() const {return mRows.size();}
  ColIndex colCount() const {return mColCount;}

  /// Returns the number of entries in the whole matrix.
  size_t entryCount() const {return mEntries.size();}

   /// Returns the number of entries in the given row.
  ColIndex entryCountInRow(RowIndex row) const {
    MATHICGB_ASSERT(row < rowCount());
    return mRows[row].size();
  }

  /// Returns true if the given row has no entries.
  bool emptyRow(RowIndex row) const {
    MATHICGB_ASSERT(row < rowCount());
    return mRows[row].empty();
  }

  ConstRowIterator rowBegin(RowIndex row) const {
    MATHICGB_ASSERT(row < rowCount());
    const Row& r = mRows[row];
    return ConstRowIterator(r.mIndicesBegin, r.mScalarsBegin);
  }

  ConstRowIterator rowEnd(RowIndex row) const {
    MATHICGB_ASSERT(row < rowCount());
    const Row& r = mRows[row];
    return ConstRowIterator(r.mIndicesEnd, r.mScalarsEnd);
  }

  /// Returns the index of the first entry in the given row. This is
  /// the first entry that you added to the row - so not necessarily the
  /// minimum column index in that row. The row in question must have at
  /// least one entry.
  ColIndex leadCol(RowIndex row) const {
    MATHICGB_ASSERT(row < rowCount());
    MATHICGB_ASSERT(!emptyRow(row));
    return *mRows[row].mIndicesBegin;
  }

  /// Prints the matrix in a human readable format to out.
  void print(std::ostream& out) const;

  std::string toString() const;



  /// Removes the leading trimThisMany columns. The columns are
  /// removed by replacing all column indices col by col -
  /// trimThisMany. No entry can have a column index less than
  /// trimThisMany, even if the scalar of that entry is set to zero.
  void trimLeadingZeroColumns(ColIndex trimThisMany);

  /// Preallocate space for at least count entries.
  void reserveEntries(size_t count);

  /// Preallocate space for at least count rows.
  void reserveRows(size_t count) {mRows.reserve(count);}

  /// Adds a new row that contains all terms that have been appended
  /// since the last time a row was added or the matrix was created.
  void rowDone() {
    MATHICGB_ASSERT(mColIndices.size() == entryCount());
    Row row;
    row.mIndicesEnd = mColIndices.end();
    row.mScalarsEnd = mEntries.end();
    if (mRows.empty()) {
      row.mIndicesBegin = mColIndices.begin();
      row.mScalarsBegin = mEntries.begin();
    } else {
      row.mIndicesBegin = mRows.back().mIndicesEnd;
      row.mScalarsBegin = mRows.back().mScalarsEnd;
    }
    mRows.push_back(row);
  }

  /// Appends an entry to the matrix. Will not appear in the matrix
  /// until rowDone is called. Do not call other methods that add rows
  /// after calling this method until rowDone has been called.
  void appendEntry(ColIndex colIndex, Scalar scalar) {
    MATHICGB_ASSERT(mColIndices.size() == entryCount());
    MATHICGB_ASSERT(colIndex < colCount());

    MATHICGB_ASSERT(mEntries.atCapacity() == mColIndices.atCapacity());
    if (mEntries.atCapacity())
      growEntryCapacity();
    MATHICGB_ASSERT(!mEntries.atCapacity());
    MATHICGB_ASSERT(!mColIndices.atCapacity());

    mColIndices.rawPushBack(colIndex);
    mEntries.rawPushBack(scalar);

    MATHICGB_ASSERT(mColIndices.size() == entryCount());
  }

  void appendRowAndNormalize(const SparseMatrix& matrix, RowIndex row, Scalar modulus);
  
  void appendRow(const SparseMatrix& matrix, RowIndex row);

  void ensureAtLeastThisManyColumns(ColIndex count) {
    if (count > colCount())
      mColCount = count;
  }

  /// Adds one more column to the matrix and returns the index of the new
  /// column.
  ColIndex appendColumn() {
    if (colCount() == std::numeric_limits<ColIndex>::max())
      mathic::reportError("Too many columns in SparseMatrix.");
    ++mColCount;
    return mColCount - 1;
  }

  void appendRowWithModulus(std::vector<uint64> const& v, Scalar modulus);
  
  void appendRow(std::vector<uint64> const& v, ColIndex leadCol = 0);

  void appendRowWithModulusNormalized(std::vector<uint64> const& v, Scalar modulus);

  // Returns true if the row was non-zero. Otherwise the row was not
  // appended.
  bool appendRowWithModulusIfNonZero(std::vector<uint64> const& v, Scalar modulus);

  /// Replaces all column indices i with colMap[i].
  void applyColumnMap(std::vector<ColIndex> colMap);

  /// Let poly be the dot product of colMonomials and the given row.
  void rowToPolynomial
  (RowIndex row, std::vector<monomial> colMonomials, Poly& poly);

  /// Reorders the rows so that the index of the leading column in
  /// each row is weakly increasing going from top to bottom. Quite
  /// slow and it makes a copy internally.
  void sortRowsByIncreasingPivots();

  /// Iterates through the entries in a row.
  class ConstRowIterator {
  public:
    typedef const std::pair<ColIndex, Scalar> value_type;
	typedef ptrdiff_t difference_type;
    typedef size_t distance_type;
    typedef value_type* pointer;
    typedef value_type& reference;
    typedef std::random_access_iterator_tag iterator_category;

    ConstRowIterator& operator++() {
      ++mScalarIt;
      ++mColIndexIt;
      return *this;
    }

    value_type operator*() const {return value_type(index(), scalar());}

    bool operator<(const ConstRowIterator& it) const {
      return mColIndexIt < it.mColIndexIt;
    }

    difference_type operator-(const ConstRowIterator& it) const {
      return mColIndexIt - it.mColIndexIt;
    }

    bool operator==(const ConstRowIterator& it) const {
      return mColIndexIt == it.mColIndexIt;
    }

    bool operator!=(const ConstRowIterator& it) const {return !(*this == it);}
    const Scalar& scalar() const {return *mScalarIt;}
    const ColIndex& index() const {return *mColIndexIt;}

  private:
    friend class SparseMatrix;
    ConstRowIterator(
      const ColIndex* const indicesIt,
      const Scalar* const scalarIt
    ):
      mColIndexIt(indicesIt),
      mScalarIt(scalarIt)
    {
    }

    const ColIndex* mColIndexIt;
    const Scalar* mScalarIt;
  };

private:
  SparseMatrix(const SparseMatrix&); // not available
  void operator=(const SparseMatrix&); // not available

  void growEntryCapacity();

  /// Contains information about a row in the matrix.
  struct Row {
    Row(): mScalarsBegin(0), mScalarsEnd(0), mIndicesBegin(0), mIndicesEnd(0) {}

    Scalar* mScalarsBegin;
    Scalar* mScalarsEnd;
    ColIndex* mIndicesBegin;
    ColIndex* mIndicesEnd;

    bool empty() const {return mIndicesBegin == mIndicesEnd;}
    ColIndex size() const {
      return static_cast<ColIndex>(std::distance(mIndicesBegin, mIndicesEnd));
    }
  };

  /// We need a RawVector here to tie the checks for the need to reallocate
  /// together between mColIndices and mEntries. We only need to check
  /// the capacity once, which, believe it or not, is a significant performance
  /// win. Not least because it decreases the amount of code and therefore
  /// causes different compiler inlining decisions.
  RawVector<Scalar> mEntries;
  RawVector<ColIndex> mColIndices;
  std::vector<Row> mRows;

  ColIndex mColCount;
};

std::ostream& operator<<(std::ostream& out, const SparseMatrix& matrix);

#endif
