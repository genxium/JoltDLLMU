#pragma once

#include <vector>
#include "BattleConsts.h"
#include "joltc_export.h"

/*
[WARNING]

This class is NOT thread-safe. 

This class DOESN'T support resizing. 

This class implicitly deallocates pointer-type element memory in destructor.
*/
template <class T>
class JOLTC_EXPORT RingBuffer {
    public: 
        int CONSECUTIVE_SET = 0;
        int NON_CONSECUTIVE_SET = 1;
        int FAILED_TO_SET = 2;
        int Ed;        // write index, open index
        int St;        // read index, closed index
        int N;
        int Cnt;       // the count of valid elements in the buffer, used mainly to distinguish what "St == Ed" means for "Pop" and "Get" methods
        std::vector<T*> Eles;
    public:
        RingBuffer(int n);
        virtual ~RingBuffer();
        int GetArrIdxByOffset(int offsetFromSt); 
        T* GetByOffset(int offsetFromSt);
        T* GetFirst();
        T* GetLast();

        // [WARNING] Popping would NOT make any change to any "Eles[i]".
        virtual T* Pop(); 
        virtual T* PopTail(); 

        // [WARNING] Only changes indexes, no deallocation.
        virtual void Clear();

        // [WARNING] Always returns a non-null pointer to the slot for assignment -- when the candidate slot is nullptr, heap memory allocation will occur.
        virtual T* DryPut();   
}; 

#include "RingBuffer.inl"
