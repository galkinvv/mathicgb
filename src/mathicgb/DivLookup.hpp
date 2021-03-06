// MathicGB copyright 2012 all rights reserved. MathicGB comes with ABSOLUTELY
// NO WARRANTY and is licensed as GPL v2.0 or later - see LICENSE.txt.
#ifndef MATHICGB_DIV_LOOKUP_GUARD
#define MATHICGB_DIV_LOOKUP_GUARD

#include "SigPolyBasis.hpp"
#include "DivisorLookup.hpp"
#include "PolyRing.hpp"
#include <string>
#include <vector>
#include <iostream>

MATHICGB_NAMESPACE_BEGIN

/** Configuration class for interface to KDTree, DivList */
/* As such, it has entries that both will expect */
/* It also contains enough for the Naive monomial table use */

template<bool AllowRemovals, bool UseDivMask>
class DivLookupConfiguration;

template<bool AR, bool DM>
class DivLookupConfiguration {

public:
  DivLookupConfiguration(
    const PolyRing& ring,
    bool minimizeOnInsert,
    bool sortOnInsert,
    bool useDivisorCache,
    double rebuildRatio,
    size_t minRebuild,
    int type,
    bool preferSparseReducers
  ):
    mBasis(0),
    mSigBasis(0),
    mRing(&ring),
    _varCount(ring.getNumVars()),
    _minimize_on_insert(minimizeOnInsert),
    _sortOnInsert(sortOnInsert),
    _useDivisorCache(useDivisorCache),
    _useAutomaticRebuild((rebuildRatio > 0.0 || minRebuild > 0) && UseDivMask),
    _rebuildRatio(rebuildRatio),
    _minRebuild(minRebuild),
    _expQueryCount(0),
    _type(type),
    _preferSparseReducers(preferSparseReducers)
  {
    MATHICGB_ASSERT(rebuildRatio >= 0);
  }

  void setBasis(const PolyBasis& basis) {
    if (mBasis == &basis)
      return;
    MATHICGB_ASSERT(mBasis == 0);
    MATHICGB_ASSERT(mRing == &basis.ring());
    mBasis = &basis;
  }

  void setSigBasis(const SigPolyBasis& sigBasis) {
    if (mSigBasis == &sigBasis)
      return;
    MATHICGB_ASSERT(mSigBasis == 0);
    MATHICGB_ASSERT(mBasis == 0 || mBasis == &sigBasis.basis());
    MATHICGB_ASSERT(mRing == &sigBasis.basis().ring());
    mSigBasis = &sigBasis;
    setBasis(sigBasis.basis());
  }

  //////////////////////////
  // Functions and types required by KDTree, or by DivList, or by ...
  //////////////////////////

  typedef int Exponent;
  typedef const_monomial Monomial;
  struct Entry {
    const_monomial monom;
    size_t index;

    Entry(): monom(0), index(static_cast<size_t>(-1)) {}
    Entry(const_monomial monom0, size_t index0):
      monom(monom0), index(index0) {}
  };

  Exponent getExponent(const Monomial& m, size_t var) const
  {
    ++_expQueryCount;
    return mRing->monomialExponent(m, var);
  }
  Exponent getExponent(const Entry& e, size_t var) const
  {
    ++_expQueryCount;
    return mRing->monomialExponent(e.monom, var);
  }

  bool divides(const Monomial& a, const Monomial& b) const
  {
    for (size_t var = 0; var < getVarCount(); ++var)
      if (getExponent(b, var) < getExponent(a, var))
        return false;
    return true;
  }

  bool divides(const Entry& a, const Monomial& b) const
  {
    for (size_t var = 0; var < getVarCount(); ++var)
      if (getExponent(b, var) < getExponent(a, var))
        return false;
    return true;
  }

  bool divides(const Monomial& a, const Entry& b) const
  {
    for (size_t var = 0; var < getVarCount(); ++var)
      if (getExponent(b, var) < getExponent(a, var))
        return false;
    return true;
  }

  bool divides(const Entry& a, const Entry& b) const
  {
    for (size_t var = 0; var < getVarCount(); ++var)
      if (getExponent(b, var) < getExponent(a, var))
        return false;
    return true;
  }

  bool isLessThan(const Monomial& a, const Monomial& b) const
  {
    for (size_t var = 0; var < getVarCount(); ++var) {
      if (getExponent(a, var) < getExponent(b, var))
        return true;
      if (getExponent(b, var) < getExponent(a, var))
        return false;
        }
    return false;
  }

  bool isLessThan(const Entry& a, const Monomial& b) const
  {
    for (size_t var = 0; var < getVarCount(); ++var) {
      if (getExponent(a, var) < getExponent(b, var))
        return true;
      if (getExponent(b, var) < getExponent(a, var))
        return false;
        }
    return false;
  }

  bool isLessThan(const Monomial& a, const Entry& b) const
  {
    for (size_t var = 0; var < getVarCount(); ++var) {
      if (getExponent(a, var) < getExponent(b, var))
        return true;
      if (getExponent(b, var) < getExponent(a, var))
        return false;
        }
    return false;
  }

  bool isLessThan(const Entry& a, const Entry& b) const
  {
    for (size_t var = 0; var < getVarCount(); ++var) {
      if (getExponent(a, var) < getExponent(b, var))
        return true;
      if (getExponent(b, var) < getExponent(a, var))
        return false;
        }
    return false;
  }

  size_t getVarCount() const {return _varCount;}

  static const bool UseTreeDivMask = DM;
  static const bool UseLinkedList = false;
  static const bool UseDivMask = DM;
  static const size_t LeafSize = 1;
  static const bool PackedTree = true;
  static const bool AllowRemovals = AR;

  bool getSortOnInsert() const {return _sortOnInsert;}
  bool getUseDivisorCache() const {return _useDivisorCache;}
  bool getMinimizeOnInsert() const {return _minimize_on_insert;}

  bool getDoAutomaticRebuilds() const {return _useAutomaticRebuild;}
  double getRebuildRatio() const {return _rebuildRatio;}
  size_t getRebuildMin() const {return _minRebuild;}

///////////////////////////
// Stats gathering ////////
///////////////////////////
public:
  struct Stats {
    size_t n_member;
    size_t n_inserts;  // includes koszuls
    size_t n_insert_already_there;
    size_t n_compares;
    unsigned long long n_expQuery;

    Stats():
      n_member(0),
      n_inserts(0),
      n_insert_already_there(0),
      n_compares(0),
      n_expQuery(0)
    {}

  };

  void displayStats(std::ostream &o) const;

///////////////////////////
  const SigPolyBasis* sigBasis() const {return mSigBasis;}
  const PolyBasis* basis() const {return mBasis;}
  const PolyRing* getPolyRing() const {return mRing;}
  unsigned long long getExpQueryCount() const {return _expQueryCount;}
  int type() const {return _type;}
  bool preferSparseReducers() const {return _preferSparseReducers;}

private:
  PolyBasis const* mBasis;
  SigPolyBasis const* mSigBasis;
  const PolyRing* mRing;
  const size_t _varCount;
  const bool _minimize_on_insert;
  const bool _sortOnInsert;
  const bool _useDivisorCache;
  const bool _useAutomaticRebuild;
  const double _rebuildRatio;
  const size_t _minRebuild;
  mutable unsigned long long _expQueryCount;
  mutable Stats _stats;
  const int _type;
  bool const _preferSparseReducers;
};

template<class Finder>
class DivLookup : public DivisorLookup {
 public:
  typedef typename Finder::Monomial Monomial;
  typedef typename Finder::Entry Entry;
  typedef typename Finder::Configuration Configuration;
  typedef Configuration C;

  DivLookup(const Configuration &C) :
        _finder(C)
  {
    MATHICGB_ASSERT(!C.UseTreeDivMask || C.UseDivMask);
  }

  ~DivLookup() {}

  virtual void setBasis(const PolyBasis& basis) {
    _finder.getConfiguration().setBasis(basis);
  }

  virtual void setSigBasis(const SigPolyBasis& sigBasis) {
    _finder.getConfiguration().setSigBasis(sigBasis);
  }


  virtual int type() const {return _finder.getConfiguration().type();}

  virtual void lowBaseDivisors(
    std::vector<size_t>& divisors,
    size_t maxDivisors,
    size_t newGenerator
  ) const {
    const SigPolyBasis* GB = _finder.getConfiguration().sigBasis();

    const_monomial sigNew = GB->getSignature(newGenerator);

    MATHICGB_ASSERT(newGenerator < GB->size());
    LowBaseDivisor searchObject(*GB, divisors, maxDivisors, newGenerator);
    _finder.findAllDivisors(sigNew, searchObject);
  }

  virtual size_t highBaseDivisor(size_t newGenerator) const {
    const SigPolyBasis* basis = _finder.getConfiguration().sigBasis();
    MATHICGB_ASSERT(newGenerator < basis->size());

    HighBaseDivisor searchObject(*basis, newGenerator);
    _finder.findAllDivisors
      (basis->getLeadMonomial(newGenerator), searchObject);
    return searchObject.highDivisor();
  }

  virtual size_t minimalLeadInSig(const_monomial sig) const {
    MinimalLeadInSig searchObject(*_finder.getConfiguration().sigBasis());
    _finder.findAllDivisors(sig, searchObject);
    return searchObject.minLeadGen();
  }

  virtual size_t classicReducer(const_monomial mon) const {
    const auto& conf = _finder.getConfiguration();
    ClassicReducer searchObject(*conf.basis(), conf.preferSparseReducers());
    _finder.findAllDivisors(mon, searchObject);
    return searchObject.reducer();
  }

  virtual size_t divisor(const_monomial mon) const {
    const Entry* entry = _finder.findDivisor(mon);
    return entry == 0 ? static_cast<size_t>(-1) : entry->index;
  }

  virtual void divisors(const_monomial mon, EntryOutput& consumer) const {
    PassOn out(consumer);
    _finder.findAllDivisors(mon, out);
  }

  virtual void multiples(const_monomial mon, EntryOutput& consumer) const {
    PassOn out(consumer);
    _finder.findAllMultiples(mon, out);
  }

  virtual void removeMultiples(const_monomial mon) {
    _finder.removeMultiples(mon);
  }

  virtual void remove(const_monomial mon) {
    _finder.removeElement(mon);
  }

  virtual size_t size() const {
    return _finder.size();
  }

  const C& getConfiguration() const {return _finder.getConfiguration();}
  C& getConfiguration() {return _finder.getConfiguration();}

  std::string getName() const;
  const PolyRing * getPolyRing() const { return getConfiguration().getPolyRing(); }

  size_t getMemoryUse() const;

private:
  // Class used in multiples() and divisors()
  struct PassOn {
  public:
    PassOn(EntryOutput& out): mOut(out) {}
    bool proceed(const Entry& entry) {
      return mOut.proceed(entry.index);
    }
  private:
    EntryOutput& mOut;
  };

  // Class used in lowBaseDivisor()
  class LowBaseDivisor {
  public:
    LowBaseDivisor(
      const SigPolyBasis& basis,
      std::vector<size_t>& divisors,
      size_t maxDivisors,
      size_t newGenerator
    ):
      mSigBasis(basis),
      mDivisors(divisors),
      mMaxDivisors(maxDivisors),
      mNewGenerator(newGenerator)
    {
      mDivisors.clear();
      mDivisors.reserve(maxDivisors + 1);
    }

    bool proceed(const Entry& entry) {
      if (entry.index >= mNewGenerator)
        return true;
      for (size_t j = 0; j <= mDivisors.size(); ++j) {
        if (j == mDivisors.size()) {
          mDivisors.push_back(entry.index);
          break;
        }
        int cmp = mSigBasis.ratioCompare(entry.index, mDivisors[j]);
        if (cmp == EQ && (entry.index < mDivisors[j]))
          cmp = GT; // prefer minimum index to ensure deterministic behavior
        if (cmp == GT) {
          mDivisors.insert(mDivisors.begin() + j, entry.index);
          break;
        }
      }
      if (mDivisors.size() > mMaxDivisors)
        mDivisors.pop_back();
      MATHICGB_ASSERT(mDivisors.size() <= mMaxDivisors);
      return true;
    }
  private:
    const SigPolyBasis& mSigBasis;
    std::vector<size_t>& mDivisors;
    const size_t mMaxDivisors;
    const size_t mNewGenerator;
  };

  // Class used in highBaseDivisor()
  class HighBaseDivisor {
  public:
    HighBaseDivisor(const SigPolyBasis& basis, size_t newGenerator):
      mSigBasis(basis),
      mNewGenerator(newGenerator),
      mHighDivisor(static_cast<size_t>(-1)) {}
    bool proceed(const Entry& entry) {
      if (entry.index >= mNewGenerator)
        return true;
      if (mHighDivisor != static_cast<size_t>(-1)) {
        int cmp = mSigBasis.ratioCompare(mHighDivisor, entry.index);
        if (cmp == LT)
          return true;
        if (cmp == EQ && (entry.index > mHighDivisor))
          return true; // prefer minimum index to ensure deterministic behavior
      }
      mHighDivisor = entry.index;
      return true;
    }
    size_t highDivisor() const {return mHighDivisor;}
  private:
    const SigPolyBasis& mSigBasis;
    const size_t mNewGenerator;
    size_t mHighDivisor;
  };

  // Class used in minimalLeadInSig()
  class MinimalLeadInSig {
  public:
    MinimalLeadInSig(const SigPolyBasis& basis):
      mSigBasis(basis),
      mMinLeadGen(static_cast<size_t>(-1)) {}

    bool proceed(const Entry& entry) {
      // Given signature sig, we want to minimize (S/G)g where
      // g and G are the lead term and signature taken over basis elements
      // whose signature G divide S. The code here instead maximizes G/g,
      // which is equivalent and also faster since the basis has a data
      // structure to accelerate comparisons between the ratio of
      // signature to lead term.
      //
      // In case of ties, we select the sparser elements. If there is
      // still a tie, we select the basis element with the largest
      // signature. There can be no further ties since all basis
      // elements have distinct signatures.

      if (mMinLeadGen != static_cast<size_t>(-1)) {
        const int ratioCmp = mSigBasis.ratioCompare(entry.index, mMinLeadGen);
        if (ratioCmp == LT)
          return true;
        if (ratioCmp == EQ) {
          // If same lead monomial in signature, pick the one with fewer terms
          // as that one might be less effort to reduce.
          const size_t minTerms = mSigBasis.poly(mMinLeadGen).nTerms();
          const size_t terms = mSigBasis.poly(entry.index).nTerms();
          if (minTerms > terms)
            return true;
          if (minTerms == terms) {
            // If same number of terms, pick the one with larger signature
            // before being multiplied into the same signature. That one
            // might be more reduced as the constraint on regular reduction
            // is less. Also, as no two generators have same signature, this
            // ensures deterministic behavior.
            const const_monomial minSig = mSigBasis.getSignature(mMinLeadGen);
            const const_monomial genSig = mSigBasis.getSignature(entry.index);
            int sigCmp = mSigBasis.monoid().compare(minSig, genSig);
            MATHICGB_ASSERT(sigCmp != EQ); // no two generators have same signature
            if (sigCmp == GT)
              return true;
          }
        }
      }
      mMinLeadGen = entry.index;
      return true;
    }

    size_t minLeadGen() const {return mMinLeadGen;}
  private:
    const SigPolyBasis& mSigBasis;
    size_t mMinLeadGen;
  };

  // Class used in ClassicReducer.
  class ClassicReducer {
  public:
    ClassicReducer(const PolyBasis& basis, const bool preferSparse):
      mBasis(basis),
      mPreferSparse(preferSparse),
      mReducer(static_cast<size_t>(-1)) {}

    bool proceed(const Entry& entry) {
      if (mReducer == static_cast<size_t>(-1)) {
        mReducer = entry.index;
        return true;
      }

      if (mPreferSparse) {
        const auto oldTermCount = mBasis.poly(mReducer).nTerms();
        const auto newTermCount = mBasis.poly(entry.index).nTerms();
        if (oldTermCount > newTermCount) {
          mReducer = entry.index; // prefer sparser
          return true;
        }
        if (oldTermCount < newTermCount)
          return true;
        // break ties by age
      }

      if (mReducer > entry.index)
        mReducer = entry.index; // prefer older
      return true;
    }

    size_t reducer() const {return mReducer;}

  private:
    const PolyBasis& mBasis;
    const bool mPreferSparse;
    size_t mReducer;
  };

  class DOCheckAll {
  public:
    DOCheckAll(
      const SigPolyBasis& basis,
      const_monomial sig,
      const_monomial monom,
      bool preferSparseReducers
    ):
      mRatioCmp(sig, monom, basis),
      mSigBasis(basis),
      mReducer(static_cast<size_t>(-1)),
      mPreferSparseReducers(preferSparseReducers) {}

    bool proceed(const Entry& e)
    {
      if (mRatioCmp.compare(e.index) != GT) {
        mSigBasis.basis().wasNonSignatureReducer(e.index);
        return true;
      }

      mSigBasis.basis().wasPossibleReducer(e.index);
      if (mReducer != static_cast<size_t>(-1)) {
        if (mPreferSparseReducers) {
          // pick sparsest
          size_t const newTermCount = mSigBasis.poly(e.index).nTerms();
          size_t const oldTermCount = mSigBasis.poly(mReducer).nTerms();
          if (newTermCount > oldTermCount)
            return true; // what we already have is sparser
          // resolve ties by picking oldest
          if (newTermCount == oldTermCount && e.index > mReducer)
            return true; // same sparsity and what we already have is older
        } else {
          // pick oldest
          if (e.index > mReducer)
            return true; // what we already have is older
        }
      }
      mReducer = e.index;
      return true;
    }

    size_t reducer() {return mReducer;}

  private:
    SigPolyBasis::StoredRatioCmp const mRatioCmp;
    SigPolyBasis const& mSigBasis;
    size_t mReducer;
    bool const mPreferSparseReducers;
  };

public:
  virtual void insert(const_monomial mon, size_t val) {
        _finder.insert(Entry(mon, val));
  }

  virtual size_t regularReducer(const_monomial sig, const_monomial mon) const
  {
    DOCheckAll out(
      *getConfiguration().sigBasis(),
      sig,
      mon,
      getConfiguration().preferSparseReducers()
    );
    _finder.findAllDivisors(mon, out);
    return out.reducer();
  }

  unsigned long long getExpQueryCount() const {
    return _finder.getConfiguration().getExpQueryCount();
  }

  size_t n_elems() const { return _finder.size(); }

  void display(std::ostream &o, int level) const;  /**TODO: WRITE ME */
  void dump(int level) const; /**TODO: WRITE ME */
 private:
  Finder _finder;
};

template<typename C>
inline std::string DivLookup<C>::getName() const {
  return "DL " + _finder.getName();
}

template<typename C>
size_t DivLookup<C>::getMemoryUse() const
{
//#warning "implement getMemoryUse for DivLookup"
  return 4 * sizeof(void *) * _finder.size();  // NOT CORRECT!!
}

MATHICGB_NAMESPACE_END
#endif
