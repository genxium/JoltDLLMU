#pragma once

#include <vector>
#include <atomic>
#include "joltc_export.h"

/*
[WARNING]

This class is MOSTLY NOT thread-safe (though some efforts are made). 

This class DOESN'T support resizing. 

This class implicitly deallocates pointer-type element memory in destructor.
*/
template <class T>
class JOLTC_EXPORT RingBufferMt {
    public: 
        int CONSECUTIVE_SET = 0;
        int NON_CONSECUTIVE_SET = 1;
        int FAILED_TO_SET = 2;
        std::atomic<int> Ed;        // write index, open index
        std::atomic<int> St;        // read index, closed index
        std::atomic<int> Cnt;       // the count of valid elements in the buffer, used mainly to distinguish what "St == Ed" means for "Pop" and "Get" methods
        int N;
        std::vector<T*> Eles;
    public:
        RingBufferMt(int n);
        virtual ~RingBufferMt();
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

#include "RingBufferMt.inl"
