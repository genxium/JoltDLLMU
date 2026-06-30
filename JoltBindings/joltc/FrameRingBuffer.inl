template <typename T, typename AllocatorType>
FrameRingBuffer<T, AllocatorType>::FrameRingBuffer(int n, AllocatorType* theAllocator, ALLOC_T_FUNC<T, AllocatorType> theAllocTFunc, FREE_T_FUNC<T, AllocatorType> theFreeTFunc) : RB_T(n, theAllocator, theAllocTFunc, theFreeTFunc) {
    StFrameId = EdFrameId = 0;
}

template <typename T, typename AllocatorType>
FrameRingBuffer<T, AllocatorType>::~FrameRingBuffer() {
    // Calls base destructor (implicitly)
}

template <typename T, typename AllocatorType>
T* FrameRingBuffer<T, AllocatorType>::Pop() {
    T* ret = RB_T::Pop(); 
    if (nullptr != ret) {
        StFrameId++;
    }
    return ret;
}

template <typename T, typename AllocatorType>
T* FrameRingBuffer<T, AllocatorType>::PopTail() {
    T* ret = RB_T::PopTail(); 
    if (nullptr != ret) {
        EdFrameId--;
    }
    return ret;
}

template <typename T, typename AllocatorType>
T* FrameRingBuffer<T, AllocatorType>::GetByFrameId(int frameId) {
    if (frameId >= EdFrameId || frameId < StFrameId) {
        return nullptr;
    }
    return RB_T::GetByOffset(frameId - StFrameId);
}

template <typename T, typename AllocatorType>
void FrameRingBuffer<T, AllocatorType>::Clear() {
    RB_T::Clear();
    StFrameId = EdFrameId = 0;
}

template <typename T, typename AllocatorType>
T* FrameRingBuffer<T, AllocatorType>::DryPut() {
    T* ret = RB_T::DryPut(); 
    EdFrameId++;
    return ret;
}
