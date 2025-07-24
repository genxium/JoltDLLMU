#pragma once

#ifndef USE_RBMT
#include "RingBuffer.h"
#define RB_T RingBuffer<T> 
#else
#include "RingBufferMt.h"
#define RB_T RingBufferMt<T> 
#endif

#include <string>

template <class T>
class FrameRingBuffer : public RB_T {
    public:
        int EdFrameId;
        int StFrameId;

    public: 
        FrameRingBuffer(int n);
        virtual ~FrameRingBuffer();
        T* Pop(); 
        T* PopTail();
        T* GetByFrameId(int frameId);

        void Clear();
        T* DryPut();

    public:
        std::string toSimpleStat() {
            std::string result = "StFrameId=" + std::to_string(StFrameId) + ", EdFrameId = " + std::to_string(EdFrameId) + ", Cnt / N = " + std::to_string(this->Cnt) + "/" + std::to_string(this->N);
            return result;
        }
}; 

#include "FrameRingBuffer.inl"
