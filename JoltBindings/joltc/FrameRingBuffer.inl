template <typename T>
FrameRingBuffer<T>::FrameRingBuffer(int n) : RB_T(n) {
    StFrameId = EdFrameId = 0;
}

template <typename T>
FrameRingBuffer<T>::~FrameRingBuffer() {
    // Calls base destructor (implicitly)
}

template <typename T>
T* FrameRingBuffer<T>::Pop() {
    auto ret = RB_T::Pop(); 
    if (nullptr != ret) {
        StFrameId++;
    }
    return ret;
}

template <typename T>
T* FrameRingBuffer<T>::PopTail() {
    auto ret = RB_T::PopTail(); 
    if (nullptr != ret) {
        EdFrameId--;
    }
    return ret;
}

template <typename T>
T* FrameRingBuffer<T>::GetByFrameId(int frameId) {
    if (frameId >= EdFrameId || frameId < StFrameId) {
        return nullptr;
    }
    return RB_T::GetByOffset(frameId - StFrameId);
}

template <typename T>
void FrameRingBuffer<T>::Clear() {
    RB_T::Clear();
    StFrameId = EdFrameId = 0;
}

template <typename T>
T* FrameRingBuffer<T>::DryPut() {
    auto ret = RB_T::DryPut(); 
    EdFrameId++;
    return ret;
}
