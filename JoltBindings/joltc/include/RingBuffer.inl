template <typename T>
inline RingBuffer<T>::RingBuffer(int n) {
    Cnt = St = Ed = 0;
    N = n;
    Eles.reserve(n);
    Eles.assign(n, nullptr);
}

template <typename T>
inline RingBuffer<T>::~RingBuffer() {
    while (0 < Cnt) {
        T* front = Pop();
        delete front;
        front = nullptr;
    }
    Clear();
}

template <typename T>
inline int RingBuffer<T>::GetArrIdxByOffset(int offsetFromSt) {
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
inline T* RingBuffer<T>::GetByOffset(int offsetFromSt) {
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
inline T* RingBuffer<T>::GetFirst() {
    if (0 == Cnt) return nullptr;
    return Eles[St];
}

template <typename T>
inline T* RingBuffer<T>::GetLast() {
    if (0 == Cnt) return nullptr;
    if (0 < Ed) return Eles[Ed - 1];
    else return Eles[N - 1];
}

template <typename T>
inline T* RingBuffer<T>::Pop() {
    if (0 == Cnt) return nullptr;
    auto holder = GetFirst();
    Cnt--; St++;

    if (St >= N) {
        St -= N;
    }
    return holder;
}

template <typename T>
inline T* RingBuffer<T>::PopTail() {
    if (0 == Cnt) return nullptr;
    auto holder = GetLast();
    Cnt--; Ed--;

    if (Ed < 0) {
        Ed += N;
    }
    return holder;
}

template <typename T>
inline void RingBuffer<T>::Clear() {
    Cnt = 0;
    St = Ed = 0;
    Eles.clear();
}

template <typename T>
inline T* RingBuffer<T>::DryPut() { 
    T* candidateSlot = (N > Ed ? Eles[Ed] : Eles[0]);
    if (nullptr == candidateSlot) {
        candidateSlot = new T();
    }
    Cnt++;
    Ed++;

    if (Ed >= N) {
        Ed -= N; // Deliberately not using "%" operator for performance concern
    }
    return candidateSlot;
}
