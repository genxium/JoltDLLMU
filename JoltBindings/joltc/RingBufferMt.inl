#ifdef JPH_BUILD_SHARED_LIBRARY // [IMPORTANT] To avoid unexpected re-compiling of these function definitions with "define JOLTC_EXPORT __declspec(dllimport)" from user code.  

template <typename T>
inline RingBufferMt<T>::RingBufferMt(int n) {
    Cnt = St = Ed = 0;
    N = n;
    Eles.reserve(n);
    Eles.assign(n, nullptr);
}

template <typename T>
inline RingBufferMt<T>::~RingBufferMt() {
    while (0 < Cnt) {
        T* front = Pop();
        delete front;
        front = nullptr;
    }
    Clear();
}

template <typename T>
inline int RingBufferMt<T>::GetArrIdxByOffset(int offsetFromSt) {
    if (0 == Cnt || 0 > offsetFromSt) {
        return -1;
    }
    int arrIdx = St + offsetFromSt;

    if (St < Ed) {
        // case#1: 0...st...ed...N-1
        if (St <= arrIdx && arrIdx < Ed) {
            return arrIdx;
        }
    } else {
        // if St >= Ed
        // case#2: 0...ed...st...N-1
        if (arrIdx >= N) {
            arrIdx -= N;

        }
        if (arrIdx >= St || arrIdx < Ed) {
            return arrIdx;

        }
    }

    return -1;
}

template <typename T>
inline T* RingBufferMt<T>::GetByOffset(int offsetFromSt) {
    int arrIdx = GetArrIdxByOffset(offsetFromSt);

    if (-1 == arrIdx) {
        return nullptr;
    }

    if (0 > arrIdx || arrIdx >= N) {
        return nullptr;
    }
    return Eles[arrIdx];
}

template <typename T>
inline T* RingBufferMt<T>::GetFirst() {
    if (0 == Cnt) return nullptr;
    return Eles[St];
}

template <typename T>
inline T* RingBufferMt<T>::GetLast() {
    if (0 == Cnt) return nullptr;
    if (0 < Ed) return Eles[Ed - 1];
    else return Eles[N - 1];
}

template <typename T>
inline T* RingBufferMt<T>::Pop() {
    // [Thread-safe proposal] 

    int oldCnt = Cnt.fetch_sub(1);
    if (0 >= oldCnt) {
        ++Cnt;
        return nullptr;
    }
    
    // [Cnt-protection] When popping, "Cnt" is decremented first; when putting, "Cnt" is incremented last.

    int holderIdx = St.fetch_add(1);
    int expectedOldSt = holderIdx + 1;
    if (holderIdx >= N) {
        // [Holder-protection] In the case of fast consecutive popping, "St.fetch_add(1)" might be called many times before a successful "compare_exchange_xxx" below occurs.
        holderIdx -= N;
    }

    auto holder = Eles[holderIdx];
    
    int proposedNewSt = expectedOldSt;
    if (proposedNewSt >= N) {
        proposedNewSt -= N;
    }
    int retryCnt = 2;
    bool success = false;
    do {
        success = St.compare_exchange_weak(expectedOldSt, proposedNewSt); 
        if (success) break;

        // By now, "expectedOldSt" holds the updated value (from other threads) which is guaranteed to be less than N by [Cnt-protection]. However, this updated value may or may not have been subtracted by N after an extrusion occurred. 

        if (expectedOldSt < N) {
            // If the updated "expectedOldSt" (from other threads) is already in valid range, we deem the current thread result as success too.
            success = true;
            break;
        } else {
           // Otherwise, the updated "expectedOldSt" (from other threads) is a result of thread-switch right after "St.fetch_add(1)", we can try to update the "proposedNewSt" accordingly -- only one success is needed regardless of which thread succeeds.
           proposedNewSt = expectedOldSt - N;   
        }

        --retryCnt;
    } while(0 < retryCnt && expectedOldSt >= N);

    if (!success) {
        // Recovery
        --St;
        ++Cnt;
        return nullptr;
    } 
    
    return holder;
}

template <typename T>
inline T* RingBufferMt<T>::PopTail() {
    // [Thread-safe proposal] 

    int oldCnt = Cnt.fetch_sub(1);
    if (0 >= oldCnt) {
        ++Cnt;
        return nullptr;
    }

    // [Cnt-protection] When popping, "Cnt" is decremented first; when putting, "Cnt" is incremented last.

    int holderIdx = Ed.fetch_sub(1) - 1;
    int expectedOldEd = holderIdx;
    if (holderIdx < 0) {
        // [Holder-protection] In the case of fast consecutive popping, "Ed.fetch_sub(1)" might be called many times before a successful "compare_exchange_xxx" below occurs.
        holderIdx += N;
    }

    auto holder = Eles[holderIdx];

    int proposedNewEd = expectedOldEd;
    if (proposedNewEd < 0) {
        // [WARNING] It's intentional to follow [DLLMU-v2.3.4](https://github.com/genxium/DelayNoMoreUnity/blob/v2.3.4/shared/RingBuffer.cs#L56) here, the handling is correct because when "Ed" decrements from "0" to "-1", we popped "Eles[N-1] (i.e. holderIdx == N-1)", therefore "Ed" should be set to exactly "N-1" as a new open slot.
        proposedNewEd += N;
    }
    int retryCnt = 2;
    bool success = false;
    do {
        success = Ed.compare_exchange_weak(expectedOldEd, proposedNewEd);
        if (success) break;

        if (expectedOldEd >= 0) {
            // If the updated "expectedOldEd" (from other threads) is already in valid range, we deem the current thread result as success too.
            success = true;
            break;
        } else {
            // Otherwise, the updated "expectedOldEd" (from other threads) is a result of thread-switch right after "Ed.fetch_sub(1)", we can try to update the "proposedNewEd" accordingly -- only one success is needed regardless of which thread succeeds.
            proposedNewEd = expectedOldEd + N;
        }

        --retryCnt;
    } while (0 < retryCnt && expectedOldEd < 0);

    if (!success) {
        // Recovery
        ++Ed;
        ++Cnt;
        return nullptr;
    }

    return holder;
}

template <typename T>
inline void RingBufferMt<T>::Clear() {
    Cnt = 0;
    St = Ed = 0;
    Eles.assign(N, nullptr);
}

template <typename T>
inline T* RingBufferMt<T>::DryPut() { 
    /**
    Different from "PopTail", the [DryPut implementation of DLLMU-v2.3.4](https://github.com/genxium/DelayNoMoreUnity/blob/v2.3.4/shared/FrameRingBuffer.cs#L43) is incorrect, the edge case still works but by a coincidence of NOT checking index strictly.
    - Given N = 32, Cnt = 8, St = StFrameId = 23, Ed = EdFrameId = 31; nextToPutFrameId = 31
    - Pre-overload
        - Calls "DryPut()", then Cnt = 9, St = StFrameId = 23, Ed = 0, EdFrameId = 32; nextToPutFrameId = 31
        - Calls "GetByFrameId(nextToPutFrameId = 31) -> GetByOffset(8) -> GetArrIdxByOffset(8)", arrIdx = St + 8 = 31, and in branch "St > Ed" arrIdx is valid because "arrIdx > St".
    - Post-overload
        - Calls "DryPut()" again, then Cnt = 10, St = StFrameId = 24, Ed = 1, EdFrameId = 33; nextToPutFrameId = 32
        - Calls "GetByFrameId(nextToPutFrameId = 32) -> GetByOffset(9) -> GetArrIdxByOffset(9)", arrIdx = St + 9 = 32 initially, and in branch "St > Ed" arrIdx is re-assigned to be 0, again becoming valid because "arrIdx < Ed".

    Carrying a working bug is not too bad for DLLMU-v2.3.4, but it's to be fixed here.
    */
    int oldCnt = Cnt.fetch_add(1);
    bool isFull = (0 < oldCnt && oldCnt >= N);
    T* candidateSlot = nullptr;
    if (isFull) {
        Pop();
    }

    int holderIdx = Ed.fetch_add(1);
    int expectedOldEd = holderIdx+1;
    // [Holder-protection]
    if (holderIdx >= N) {
        holderIdx = 0;
    }

    auto holder = Eles[holderIdx];

    int proposedNewEd = expectedOldEd;
    // [WARNING] The following boundary handling is asymmetric to "PopTail"!
    if (proposedNewEd > N) {
        proposedNewEd = 1;
    }
    
    int retryCnt = 2;
    bool success = false;
    do {
        success = Ed.compare_exchange_weak(expectedOldEd, proposedNewEd);
        if (success) break;
        // [WARNING] The following boundary handling is asymmetric to "PopTail"!
        if (expectedOldEd > N) {
            // If the updated "expectedOldEd" (from other threads) is a result of thread-switch right after "Ed.fetch_add(1)", we can try to update the "proposedNewEd" accordingly -- only one success is needed regardless of which thread succeeds.
            proposedNewEd = expectedOldEd - N; // e.g. when expectedOldEd == N+2, proposedNewEd = 2
        } else {
            // If the updated "expectedOldEd" (from other threads) is already in valid range, we deem the current thread result as success too.
            success = true;
            break;
        }

        --retryCnt;
    } while (0 < retryCnt && expectedOldEd < 0);

    if (success) {
        T** pSlot = &Eles[holderIdx];
        if (nullptr == *pSlot) {
            *pSlot = new T();
        }
        candidateSlot = *pSlot;
    }

    if (nullptr == candidateSlot) {
        // Recovery
        --Cnt;
        --Ed;
        return nullptr;
    } 
    
    return candidateSlot;
}
#endif
