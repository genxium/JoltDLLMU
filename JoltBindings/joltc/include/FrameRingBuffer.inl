template <typename T>
inline FrameRingBuffer<T>::FrameRingBuffer(int n) : RingBuffer(n) {
    StFrameId = EdFrameId = 0;
}

template <typename T>
inline FrameRingBuffer<T>::~FrameRingBuffer() {
    // Calls base destructor (implicitly)
}

template <typename T>
inline bool FrameRingBuffer<T>::Put(T item) {
    bool ret = RingBuffer<T>::Put(item); 
    EdFrameId++;
    return ret;
}

template <typename T>
inline T* FrameRingBuffer<T>::Pop() {
    auto ret = RingBuffer<T>::Pop(); 
    if (nullptr != ret) {
        StFrameId++;
    }
    return ret;
}

template <typename T>
inline T* FrameRingBuffer<T>::PopTail() {
    auto ret = RingBuffer<T>::PopTail(); 
    if (nullptr != ret) {
        EdFrameId--;
    }
    return ret;
}

template <typename T>
inline void FrameRingBuffer<T>::DryPut() {
    while (0 < Cnt && Cnt >= N) {
        // Make room for the new element
        Pop();
    }
    EdFrameId++;
    Cnt++;
    Ed++;

    if (Ed >= N) {
        Ed -= N; // Deliberately not using "%" operator for performance concern
    }
}

template <typename T>
inline T* FrameRingBuffer<T>::GetByFrameId(int frameId) {
    if (frameId >= EdFrameId || frameId < StFrameId) {
        return nullptr;
    }
    return GetByOffset(frameId - StFrameId);
}

template <typename T>
inline std::tuple<int, int, int> FrameRingBuffer<T>::SetByFrameId(T item, int frameId) {
    int oldStFrameId = StFrameId;
    int oldEdFrameId = EdFrameId;

    if (frameId < oldStFrameId) {
        return std::tuple<int, int, int>(FAILED_TO_SET, oldStFrameId, oldEdFrameId);
    }

    // By now "StFrameId <= frameId"
    if (oldEdFrameId > frameId) {
        int arrIdx = GetArrIdxByOffset(frameId - StFrameId);

        if (-1 != arrIdx) {
            Eles[arrIdx] = item;
            return std::tuple<int, int, int>(CONSECUTIVE_SET, oldStFrameId, oldEdFrameId);
        }
    }

    // By now "EdFrameId <= frameId"
    int ret = CONSECUTIVE_SET;

    if (oldEdFrameId < frameId) {
        St = Ed = 0;
        StFrameId = EdFrameId = frameId;
        Cnt = 0;
        ret = NON_CONSECUTIVE_SET;
    }

    // By now "EdFrameId == frameId"
    Put(item);

    return std::tuple<int, int, int>(ret, oldStFrameId, oldEdFrameId);
} 

template <typename T>
inline void FrameRingBuffer<T>::Clear() {
    RingBuffer<T>::Clear();
}
