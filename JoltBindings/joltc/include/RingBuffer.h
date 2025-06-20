#pragma once

#include <vector>
#include "BattleConsts.h"
#include "joltc_export.h"

template <class T>
class JOLTC_EXPORT RingBuffer {
    public: 
        int CONSECUTIVE_SET = 0;
        int NON_CONSECUTIVE_SET = 1;
        int FAILED_TO_SET = 2;
        int Ed;        // write index, open index
        int St;        // read index, closed index
        int N;
        int Cnt;       // the count of valid elements in the buffer, used mainly to distinguish what "st == ed" means for "Pop" and "Get" methods
        std::vector<T> Eles;
    public:
        RingBuffer(int n);
        virtual ~RingBuffer();
        bool Put(T item);
        T* GetFirst();
        T* GetLast();
        T* Pop();
        T* PopTail();
        int GetArrIdxByOffset(int offsetFromSt); 
        T* GetByOffset(int offsetFromSt);
        void Clear();
}; 

#include "RingBuffer.inl"
