// MathicGB copyright 2012 all rights reserved. MathicGB comes with ABSOLUTELY
// NO WARRANTY and is licensed as GPL v2.0 or later - see LICENSE.txt.
#ifndef MATHICGB_DIVISOR_LOOKUP_GUARD
#define MATHICGB_DIVISOR_LOOKUP_GUARD

#include "PolyRing.hpp"
#include <vector>

MATHICGB_NAMESPACE_BEGIN

class PolyBasis;
class SigPolyBasis;

// Supports queries on the lead terms of the monomials in a PolyBasis.
// todo: rename to MonomialLookup.
class DivisorLookup
{
public:
  // Call after construction. Can be called multiple times, but only if the
  // parameter object is the same each time.
  virtual void setBasis(const PolyBasis& basis) = 0;

  // Call after construction. Can be called multiple times, but only if the
  // parameter object is the same each time.
  virtual void setSigBasis(const SigPolyBasis& sigBasis) = 0;

  virtual ~DivisorLookup() {}

  virtual void insert(const_monomial mon, size_t index) = 0;

  // Returns the index of a basis element that regular reduces mon in
  // signature sig. Returns -1 if no such element exists. A basis element
  // u is a regular reducer if leadTerm(u) divides mon
  // and (mon / leadTerm(u)) * signature(u) < sig.
  virtual size_t regularReducer
    (const_monomial sig, const_monomial mon) const = 0;

  // Returns the index of a basis element whose lead term divides mon. The
  // strategy used to break ties is up to the implementation of the interface,
  // but the outcome must be deterministic.
  virtual size_t classicReducer(const_monomial mon) const = 0;

  virtual std::string getName() const = 0;

  virtual size_t getMemoryUse() const = 0;

  virtual size_t highBaseDivisor(size_t newGenerator) const = 0;
  virtual void lowBaseDivisors(
    std::vector<size_t>& divisors,
    size_t maxDivisors,
    size_t newGenerator) const = 0;
  virtual size_t minimalLeadInSig(const_monomial sig) const = 0;

  virtual int type() const = 0;

  static void displayDivisorLookupTypes(std::ostream &o);

  class Factory {
  public:
    virtual std::unique_ptr<DivisorLookup> create
      (bool preferSparseReducers, bool allowRemovals) const = 0;
  };
  static std::unique_ptr<Factory> makeFactory
    (const PolyRing& ring, int type);
  // choices for type: 1: divlist, 2:kdtree.

  class EntryOutput {
  public:
    // Stop whatever is happening if proceed returns false.
    virtual bool proceed(size_t index) = 0;
  };

  // Calls consumer.proceed(index) for each element whose lead term
  // divides mon. Stops the search if proceed returns false.
  virtual void multiples(const_monomial mon, EntryOutput& consumer) const = 0;

  // Returns the index of a basis element whose lead term divides mon.
  virtual size_t divisor(const_monomial mon) const = 0;

  // Calls consumer.proceed(index) for each element whose term
  // mon divides. Stops the search if proceed returns false.
  virtual void divisors(const_monomial mon, EntryOutput& consumer) const = 0;

  // Removes multiples of mon. An element equal to mon counts as a multiple.
  virtual void removeMultiples(const_monomial mon) = 0;

  // Removes entries whose monomial are equal to mon.
  virtual void remove(const_monomial mon) = 0;

  // Returns how many elements are in the data structure.
  virtual size_t size() const = 0;
};

MATHICGB_NAMESPACE_END
#endif
