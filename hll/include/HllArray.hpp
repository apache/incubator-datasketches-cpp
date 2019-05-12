/*
 * Copyright 2018, Yahoo! Inc. Licensed under the terms of the
 * Apache License 2.0. See LICENSE file at the project root for terms.
 */

#ifndef _HLLARRAY_HPP_
#define _HLLARRAY_HPP_

#include "HllSketchImpl.hpp"
#include "HllUtil.hpp"
#include "PairIterator.hpp"

namespace datasketches {

template<typename A>
class AuxHashMap;

template<typename A = std::allocator<char>>
class HllArray : public HllSketchImpl<A> {
  public:
    explicit HllArray(int lgConfigK, TgtHllType tgtHllType, bool startFullSize);
    explicit HllArray(const HllArray<A>& that);

    //static HllArray* newHll(int lgConfigK, TgtHllType tgtHllType);
    static HllArray* newHll(const void* bytes, size_t len);
    static HllArray* newHll(std::istream& is);

    virtual std::pair<std::unique_ptr<uint8_t, std::function<void(uint8_t*)>>, const size_t> serialize(bool compact) const;
    virtual void serialize(std::ostream& os, bool compact) const;

    virtual ~HllArray();
    virtual std::function<void(HllSketchImpl<A>*)> get_deleter() const = 0;

    virtual HllArray* copy() const = 0;
    virtual HllArray* copyAs(TgtHllType tgtHllType) const;

    virtual HllSketchImpl<A>* couponUpdate(int coupon) = 0;

    virtual double getEstimate() const;
    virtual double getCompositeEstimate() const;
    virtual double getLowerBound(int numStdDev) const;
    virtual double getUpperBound(int numStdDev) const;

    void addToHipAccum(double delta);

    void decNumAtCurMin();

    int getCurMin() const;
    int getNumAtCurMin() const;
    double getHipAccum() const;

    virtual int getHllByteArrBytes() const = 0;

    //virtual std::unique_ptr<PairIterator<A>> getIterator() const = 0;
    //virtual std::unique_ptr<PairIterator<A>> getAuxIterator() const;
    virtual PairIterator_with_deleter<A> getIterator() const = 0;
    virtual PairIterator_with_deleter<A> getAuxIterator() const;

    virtual int getUpdatableSerializationBytes() const;
    virtual int getCompactSerializationBytes() const;

    virtual bool isOutOfOrderFlag() const;
    virtual bool isEmpty() const;
    virtual bool isCompact() const;

    virtual void putOutOfOrderFlag(bool flag);

    double getKxQ0() const;
    double getKxQ1() const;

    virtual int getMemDataStart() const;
    virtual int getPreInts() const;

    virtual int getSlot(int slotNo) const = 0;
    virtual void putSlot(int slotNo, int value) = 0;

    void putCurMin(int curMin);
    void putHipAccum(double hipAccum);
    void putKxQ0(double kxq0);
    void putKxQ1(double kxq1);
    void putNumAtCurMin(int numAtCurMin);

    static int hllArrBytes(TgtHllType tgtHllType, int lgConfigK);
    static int hll4ArrBytes(int lgConfigK);
    static int hll6ArrBytes(int lgConfigK);
    static int hll8ArrBytes(int lgConfigK);

    virtual AuxHashMap<A>* getAuxHashMap() const;

  protected:
    // TODO: does this need to be static?
    static void hipAndKxQIncrementalUpdate(HllArray& host, int oldValue, int newValue);
    double getHllBitMapEstimate(int lgConfigK, int curMin, int numAtCurMin) const;
    double getHllRawEstimate(int lgConfigK, double kxqSum) const;

    double hipAccum;
    double kxq0;
    double kxq1;
    uint8_t* hllByteArr; //init by sub-classes
    int curMin; //always zero for Hll6 and Hll8, only used / tracked by Hll4Array
    int numAtCurMin; //interpreted as num zeros when curMin == 0
    bool oooFlag; //Out-Of-Order Flag

    //friend class Conversions;
    friend class HllSketchImplFactory<A>;
};

}

//#include "HllArray-internal.hpp"

#endif /* _HLLARRAY_HPP_ */