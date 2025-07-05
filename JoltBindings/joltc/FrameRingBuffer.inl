#ifdef JPH_BUILD_SHARED_LIBRARY // [IMPORTANT] To avoid unexpected re-compiling of these function definitions with "define JOLTC_EXPORT __declspec(dllimport)" from user code.  
template <typename T>
FrameRingBuffer<T>::FrameRingBuffer(int n) : RingBuffer<T>(n) {
    StFrameId = EdFrameId = 0;
}

template <typename T>
FrameRingBuffer<T>::~FrameRingBuffer() {
    // Calls base destructor (implicitly)
}

template <typename T>
T* FrameRingBuffer<T>::Pop() {
    auto ret = RingBuffer<T>::Pop(); 
    if (nullptr != ret) {
        StFrameId++;
    }
    return ret;
}

template <typename T>
T* FrameRingBuffer<T>::PopTail() {
    auto ret = RingBuffer<T>::PopTail(); 
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
    return RingBuffer<T>::GetByOffset(frameId - StFrameId);
}

template <typename T>
void FrameRingBuffer<T>::Clear() {
    RingBuffer<T>::Clear();
    StFrameId = EdFrameId = 0;
}

template <typename T>
T* FrameRingBuffer<T>::DryPut() {
    auto ret = RingBuffer<T>::DryPut(); 
    EdFrameId++;
    return ret;
}
#endif
