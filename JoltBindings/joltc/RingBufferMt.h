#ifndef RING_BUFFER_MT_H_
#define RING_BUFFER_MT_H_ 1

#include <vector>
#include <atomic>
#include <string>
#include <ostream>

#ifndef NDEBUG
#include "DebugLog.h"
#include <thread>
#include <sstream>
#endif

/*
[WARNING]

This class is MOSTLY NOT thread-safe (though some efforts are made). 

This class DOESN'T support resizing. 

This class implicitly deallocates pointer-type element memory in destructor.
*/
template <class T>
class RingBufferMt {
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
    protected:
        std::atomic<int> dirtyPuttingCnt;        // used for [Cnt-protection] in "DryPut()"

    public:
        std::string toSimpleStat() {
            std::ostringstream oss;
            oss << "St=" << St << ", Ed = " << Ed << ", Cnt / N = " << this->Cnt << "/" << this->N;
            return oss.str();
        }
        
        int GetDirtyPuttingCnt() {
            return dirtyPuttingCnt;
        }
}; 

#include "RingBufferMt.inl"

#endif
