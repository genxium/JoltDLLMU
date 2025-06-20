#pragma once

#include "RingBuffer.h"
#include <tuple>
#include <string>

template <class T>
class JOLTC_EXPORT FrameRingBuffer : RingBuffer<T> {
    public:
        int EdFrameId;
        int StFrameId;

    public: 
        FrameRingBuffer(int n);
        virtual ~FrameRingBuffer();
        bool Put(T item);
        T* Pop(); 
        T* PopTail();
        void DryPut();
        T* GetByFrameId(int frameId);
        std::tuple<int, int, int> SetByFrameId(T item, int frameId); 
        void Clear();

    public:
        inline std::string toSimpleStat() {
            std::string result = "StFrameId=" + std::to_string(StFrameId) + ", EdFrameId = " + std::to_string(EdFrameId) + ", Cnt / N = " + std::to_string(Cnt) + "/" + std::to_string(N);
            return result;
        }
}; 

#include "FrameRingBuffer.inl"
