template <typename T, typename AllocatorType>
inline RingBuffer<T, AllocatorType>::RingBuffer(int n, AllocatorType* theAllocator, ALLOC_T_FUNC<T, AllocatorType> theAllocTFunc, FREE_T_FUNC<T, AllocatorType> theFreeTFunc) {
    Cnt = St = Ed = 0;
    N = n;
    Eles.reserve(n);
    Eles.assign(n, nullptr);
    allocator = theAllocator;
    allocTFunc = theAllocTFunc;
    freeTFunc = theFreeTFunc;
}

template <typename T, typename AllocatorType>
inline RingBuffer<T, AllocatorType>::~RingBuffer() {
    // [WARNING] It's inconvenient to use Google Protobuf Arena Allocation on this "RingBuffer<T>" for general purpose, therefore individual allocation in "DryPut()" and deallocation here are used.

    while (0 < Cnt) {
        T* front = Pop();
        if (nullptr == freeTFunc) {
            delete front;
        } else {
            freeTFunc(front, allocator);
        }
    }

    Clear();
}

template <typename T, typename AllocatorType>
inline int RingBuffer<T, AllocatorType>::GetArrIdxByOffset(int offsetFromSt) {
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

template <typename T, typename AllocatorType>
inline T* RingBuffer<T, AllocatorType>::GetByOffset(int offsetFromSt) {
    int arrIdx = GetArrIdxByOffset(offsetFromSt);

    if (-1 == arrIdx) {
        return nullptr;
    }

    if (0 > arrIdx || arrIdx >= N) {
        return nullptr;
    }
    return Eles[arrIdx];
}

template <typename T, typename AllocatorType>
inline T* RingBuffer<T, AllocatorType>::GetFirst() {
    if (0 == Cnt) return nullptr;
    return Eles[St];
}

template <typename T, typename AllocatorType>
inline T* RingBuffer<T, AllocatorType>::GetLast() {
    if (0 == Cnt) return nullptr;
    if (0 < Ed) return Eles[Ed - 1];
    else return Eles[N - 1];
}

template <typename T, typename AllocatorType>
inline T* RingBuffer<T, AllocatorType>::Pop() {
    if (0 == Cnt) return nullptr;
    auto holder = GetFirst();
    Cnt--; St++;

    if (St >= N) {
        St -= N;
    }
    return holder;
}

template <typename T, typename AllocatorType>
inline T* RingBuffer<T, AllocatorType>::PopTail() {
    if (0 == Cnt) return nullptr;
    auto holder = GetLast();
    Cnt--; Ed--;

    if (Ed < 0) {
        Ed += N;
    }
    return holder;
}

template <typename T, typename AllocatorType>
inline void RingBuffer<T, AllocatorType>::Clear() {
    Cnt = 0;
    St = Ed = 0;
}

template <typename T, typename AllocatorType>
inline T* RingBuffer<T, AllocatorType>::DryPut() { 
    bool isFull = (0 < Cnt && Cnt == N); 
    T* candidateSlot = nullptr;
    if (isFull) {
        candidateSlot = Pop();
    } else {
        T** pSlot = (N > Ed ? &Eles[Ed] : &Eles[0]);
        if (nullptr == *pSlot) {
            if (nullptr == allocTFunc) {
                *pSlot = new T();
            } else {
                *pSlot = allocTFunc(allocator);
            }
        }
        candidateSlot = *pSlot;
    }

    Cnt++;
    Ed++;

    if (Ed > N) {
        Ed -= N; // Deliberately not using "%" operator for performance concern
    }
    return candidateSlot;
}
