template <typename T>
inline RingBufferMt<T>::RingBufferMt(int n) {
    dirtyPuttingCnt = 0;
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
    int oldCnt = Cnt.fetch_sub(1);
    if (0 >= oldCnt) {
        // [Cnt-protection]
        ++Cnt;
        return nullptr;
    }

    // [WARNING] It's OK that "Pop" passes the empty-check due to thread switch immediately after "Cnt.fetch_add" from other threads' calls of "DryPut" -- in that case we might be popping a pointer to the being-written-element, and synchronization of such element should be a caller's responsibility AS LONG AS we maintain correct relationships among [St, Ed, Cnt, dirtyPuttingCnt].  

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

        // By now, "expectedOldSt" holds the updated value (from other threads) which may or may not have been subtracted by N after an extrusion occurred. 
        if (expectedOldSt < N) {
            // If the updated "expectedOldSt" (from other threads) is already in valid range, we deem the current thread result as success too.
            success = true;
            break;
        } else {
           // Otherwise, the updated "expectedOldSt" (from other threads) is a result of thread-switch right after "St.fetch_add(1)", we can try to update the "proposedNewSt" accordingly -- only one success is needed regardless of which thread succeeds.
            // At most "- N" is needed due to [Cnt-protection].
           proposedNewSt = expectedOldSt - N;   
        }

        --retryCnt;
    } while(0 < retryCnt);

    if (!success) {
        // Recovery
        --St;
        ++Cnt;
#ifndef NDEBUG
        std::ostringstream oss;
        oss << "[FAILED TO UPDATE St/Pop] thId=" << std::this_thread::get_id() << ", expectedOldSt=" << expectedOldSt << ", proposedNewSt=" << proposedNewSt << ", holderIdx=" << holderIdx; 
        Debug::Log(oss.str(), DColor::Red);
#endif
        return nullptr;
    } 
    
    return holder;
}

template <typename T>
inline T* RingBufferMt<T>::PopTail() {
    int oldCnt = Cnt.fetch_sub(1);
    if (0 >= oldCnt) {
        // [Cnt-protection]
        ++Cnt;
        return nullptr;
    }

    // [WARNING] It's OK that "PopTail" passes the empty-check due to thread switch immediately after "Cnt.fetch_add" from other threads' calls of "DryPut" -- in that case we might be popping a pointer to the being-written-element, and synchronization of such element should be a caller's responsibility AS LONG AS we maintain correct relationships among [St, Ed, Cnt, dirtyPuttingCnt].  

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

        // By now, "expectedOldEd" holds the updated value (from other threads) which may or may not have been added by N after an extrusion occurred. 
        if (expectedOldEd >= 0) {
            // If the updated "expectedOldEd" (from other threads) is already in valid range, we deem the current thread result as success too.
            success = true;
            break;
        } else {
            // Otherwise, the updated "expectedOldEd" (from other threads) is a result of thread-switch right after "Ed.fetch_sub(1)", we can try to update the "proposedNewEd" accordingly -- only one success is needed regardless of which thread succeeds.
            // At most "+ N" is needed due to [Cnt-protection].
            proposedNewEd = expectedOldEd + N;
        }

        --retryCnt;
    } while (0 < retryCnt);

    if (!success) {
        // Recovery
#ifndef NDEBUG
        std::ostringstream oss;
        oss << "[FAILED TO UPDATE Ed/PopTail] thId=" << std::this_thread::get_id() << ", expectedOldEd=" << expectedOldEd << ", proposedNewEd=" << proposedNewEd << ", holderIdx=" << holderIdx;
        Debug::Log(oss.str(), DColor::Red);
#endif
        ++Ed;
        ++Cnt;
        return nullptr;
    }

    return holder;
}

template <typename T>
inline void RingBufferMt<T>::Clear() {
    dirtyPuttingCnt = 0;
    Cnt = 0;
    St = Ed = 0;
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
    int oldDirtyPuttingCnt = dirtyPuttingCnt.fetch_add(1);
    if (oldDirtyPuttingCnt >= N) {
        // [Cnt-protection] The worst case is that N threads are concurrently calling "DryPut()", and all switched after calling the above "Cnt.fetch_add(1) & Ed.fetch_add(1)" statements, then "holderIdx" should be capped by "2*N" in "the last thread" to AVOID evicting a "to-be-filled holder of the first thread" during this concurrency sprint. 
#ifndef NDEBUG
        std::ostringstream oss;
        oss << "[TOO MANY DIRTY] thId=" << std::this_thread::get_id() << ", oldDirtyPuttingCnt= " << oldDirtyPuttingCnt << ", N=" << this->N;
        Debug::Log(oss.str(), DColor::Red);
#endif
        --oldDirtyPuttingCnt;
        return nullptr;
    }

    int holderIdx = Ed.fetch_add(1);
    int expectedOldEd = holderIdx+1;
    if (holderIdx >= N) {
        // [Holder-protection]
        holderIdx -= N; // Locates the "thread-specific holderIdx" during a concurrency sprint. 
    }

    T* holder = Eles[holderIdx];

    int proposedNewEd = expectedOldEd;
    // [WARNING] The following boundary handling is asymmetric to "PopTail"!
    if (proposedNewEd > N) {
        proposedNewEd -= N;
    }
    
    int retryCnt = 2;
    bool success = false;
    do {
        success = Ed.compare_exchange_weak(expectedOldEd, proposedNewEd);
        if (success) break;

        // By now, "expectedOldEd" holds the updated value (from other threads) which may or may not have been subtracted by N after an extrusion occurred. 
        // [WARNING] The following boundary handling is asymmetric to "PopTail"!
        if (expectedOldEd > N) {
            // If the updated "expectedOldEd" (from other threads) is a result of thread-switch right after "Ed.fetch_add(1)", we can try to update the "proposedNewEd" accordingly -- only one success is needed regardless of which thread succeeds.
            // At most "- N" is needed due to [Cnt-protection].
            proposedNewEd = expectedOldEd - N; // e.g. when expectedOldEd == N+2, proposedNewEd = 2
        } else {
            // If the updated "expectedOldEd" (from other threads) is already in valid range, we deem the current thread result as success too.
            success = true;
            break;
        }

        --retryCnt;
    } while (0 < retryCnt);

    if (success) {
        if (nullptr == holder) {
            Eles[holderIdx] = new T();
            holder = Eles[holderIdx];
        } 
        int oldCnt = Cnt.fetch_add(1); // [Cnt-protection] Decremented first when popping, incremented last when putting.
        bool overflowed = (N <= oldCnt);
        if (overflowed) {
            /*
             [EdgeCase setup#1] @Cnt=N=8, th#1, th#42, th#99 are calling "DryPut()" while th#2, th#41, th#88 are calling "PopTail()" altogether simultaneously.
             1.1. If the call sequence starts with "th#1 {Cnt.fetch_add}" -> "th#42 {Cnt.fetch_add}" -> "th#2 {Cnt.fetch_sub}", then 
                - both "th#1" and "th#42" will call "Pop" because they see "oldCnt = 8" and "oldCnt = 9" respectively, and 
                - "th#2" will succeed in its own "PopTail" and returns the element inserted by "th#42 {DryPut}".

             1.2. If the call sequence starts with "th#1 {Cnt.fetch_add, overflowed-Pop}" -> "th#42 {Cnt.fetch_add}" -> "th#2 {Cnt.fetch_sub}", then 
                - "th#1" will decide to call "Pop" because it sees "oldCnt = 8" till success of its whole "DryPut", and 
                - "th#42" will also decide to call "Pop" because it sees "oldCnt = 8" too, and 
                - "th#2" will succeed in its own "PopTail" and returns the element inserted by "th#42 {DryPut}".

             1.3. If the call sequence starts with "th#1 {Cnt.fetch_add, overflowed-Pop}" -> "th#2 {Cnt.fetch_sub}" -> "th#42 {Cnt.fetch_add}", then 
                - "th#1" will decide to call "Pop" because it sees "oldCnt = 8" till the whole "DryPut" succeeds, and 
                - "th#2" will succeed in its own "PopTail" BUT the returned element will depend on the order of ["th#2 {Ed.fetch_sub}", "th#42 {Ed.fetch_add}"], and 
                - "th#42" would NOT call "Pop()" because it sees "oldCnt = 7". 

             1.4. If the call sequence starts with "th#1 {Cnt.fetch_add}" -> "th#2 {Cnt.fetch_sub}" -> "th#1 {overflowed-Pop}" -> "th#42 {Cnt.fetch_add}", then 
                - "th#1" will decide to call "Pop()" because it sees "oldCnt = 8", and 
                - "th#2" will succeed in its own "PopTail" BUT the returned element will depend on the order of ["th#2 {Ed.fetch_sub}", "th#42 {Ed.fetch_add}"], and 
                - "th#1" will succeed in its own "overflowed-Pop", and
                - "th#42" would NOT call "Pop()" because it sees "oldCnt = 7". 

             1.5. If the call sequence starts with "th#1 {Cnt.fetch_add}" -> "th#2 {Cnt.fetch_sub}" -> "th#42 {Cnt.fetch_add}" -> "th#1 {overflowed-Pop}", then 
                - "th#1" will decide to call "Pop" because it sees "oldCnt = 8", and 
                - "th#2" will succeed in its own "PopTail" BUT the returned element will depend on the order of ["th#2 {Ed.fetch_sub}", "th#42 {Ed.fetch_add}"], and 
                - "th#42" will also decide to call "Pop" because it sees "oldCnt = 8" too, and 
                - "th#1" will succeed in its own "overflowed-Pop".

             1.6. If the call sequence starts with "th#2 {Cnt.fetch_sub}" -> "th#1 {Cnt.fetch_add}", then 
                - "th#2" will succeed in its own "PopTail" BUT the returned element will depend on the order of ["th#1 {Ed.fetch_add}", "th#2 {Ed.fetch_sub}"], and 
                - "th#1" would NOT call "Pop()" because it sees "oldCnt = 7".


             [EdgeCase setup#2] @Cnt=0,N=8, th#1, th#42, th#99 are calling "DryPut()" while th#2, th#41, th#88 are calling "PopTail()" altogether simultaneously.
             2.1. If the call sequence starts with "th#2 {Cnt.fetch_sub}" -> "th#1 {Cnt.fetch_add}", then 
                - "th#2" would fail in its own "PopTail", and
                - "th#1" will succeed in its whole "DryPut" (would NOT call "Pop" because "oldCnt == -1").

             2.2. If the call sequence starts with "th#1 {Cnt.fetch_add}" -> "th#2 {Cnt.fetch_sub}", then 
                - "th#1" will succeed in its whole "DryPut" (would NOT call "Pop" because "oldCnt == 0").
                - "th#2" will succeed in its own "PopTail".
            */
            Pop();
        }
    } else {
        // Recovery, but the oldCnt-full-induced-popped element couldn't be recovered by the current implementation 
        --Ed;
#ifndef NDEBUG
        std::ostringstream oss;
        oss << "[FAILED TO UPDATE Ed/DryPut] thId=" << std::this_thread::get_id() << ", oldDirtyPuttingCnt= " + oldDirtyPuttingCnt << ", expectedOldEd=" << expectedOldEd << ", proposedNewEd=" << proposedNewEd << ", holderIdx=" << holderIdx;
        Debug::Log(oss.str(), DColor::Red);
#endif
    }
     
    --dirtyPuttingCnt;
    return holder;
}
