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

        virtual bool IsConsistent() {
            if (!(0 <= St && N > St)) {
                return false;
            }
            if (!(0 <= Ed && N >= Ed)) {
                return false;
            }
            if (!(0 <= Cnt && N >= Cnt)) {
                return false;
            }
            if (!(0 == dirtyPuttingCnt)) {
                return false;
            }
            return (isConsistent1() || isConsistent2());
        }

    protected:
        std::atomic<int> dirtyPuttingCnt;        // used for [Cnt-protection] in "DryPut()"

        virtual bool isConsistent1() {
            return (St + Cnt == Ed);
        }

        virtual bool isConsistent2() {
            return (St + Cnt >= N && St + Cnt == Ed + N);
        }

    public:
        std::string toSimpleStat() {
            std::ostringstream oss;
            oss << "St=" << St << ", Ed = " << Ed << ", Cnt / N = " << this->Cnt << "/" << this->N;
            return oss.str();
        }
}; 

#include "RingBufferMt.inl"

/*
The use of "RingBufferMt::DryPut() & RingBufferMt::Pop()/PopTail()" is analogous to "FixedSizeFreeList::ConstructObject & FixedSizeFreeList::DeconstructObject", however there're still a few noticeable differences.

- By the use of "atomic<int> RingBufferMt.dirtyPuttingCnt", I have a few exception handling for "St/Ed/Cnt.compare_exchange_weak" while no equivalent exception handling is found in "FixedSizeFreeList".  

- There's no concern about "FixedSizeFreeList::ConstructObject - FixedSizeFreeList::ConstructObject" intercepting execution, because in case of a conflict, some will fail [mFirstFreeObjectAndTag.compare_exchange_weak](https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Core/FixedSizeFreeList.inl#L95) and allocation would NOT be executed.

- There's some concern about "FixedSizeFreeList::DesstructObject - FixedSizeFreeList::ConstructObject" intercepting execution, because in case of a conflict, the failure of [FixedSizeFreeList::DesstructObject/mFirstFreeObjectAndTag.compare_exchange_weak](https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Core/FixedSizeFreeList.inl#L198) would NOT rollback previous deallocation or the assignment of [deallocatedObject.mNextFreeObject](https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Core/FixedSizeFreeList.inl#L192) -- what's worse, as only the destructor of Object is called (i.e. no "delete" called), the Object is NOT freed to be reused by [AlignedAllocate or "new"](https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Core/FixedSizeFreeList.inl#L74).
    - Jolt resolves the concern by [putting an infinite loop wrapping "mFirstFreeObjectAndTag.compare_exchange_weak" together with its preceding "original first free look up"](https://github.com/jrouwe/JoltPhysics/blob/v5.3.0/Jolt/Core/FixedSizeFreeList.inl#L185), which is valid but possibly inefficient. 

*/

#endif
