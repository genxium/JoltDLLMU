template <typename T>
inline FrameRingBuffer<T>::FrameRingBuffer(int n) : RingBuffer(n) {
    StFrameId = EdFrameId = 0;
}

template <typename T>
inline FrameRingBuffer<T>::~FrameRingBuffer() {
    // Calls base destructor (implicitly)
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
inline T* FrameRingBuffer<T>::GetByFrameId(int frameId) {
    if (frameId >= EdFrameId || frameId < StFrameId) {
        return nullptr;
    }
    return GetByOffset(frameId - StFrameId);
}

template <typename T>
inline void FrameRingBuffer<T>::Clear() {
    RingBuffer<T>::Clear();
    StFrameId = EdFrameId = 0;
}

template <typename T>
inline T* FrameRingBuffer<T>::DryPut() {
    auto ret = RingBuffer<T>::DryPut(); 
    EdFrameId++;
    return ret;
}
