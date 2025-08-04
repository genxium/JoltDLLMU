#include "RingBufferMt.h"
#include <thread>
#include <functional>
#include <algorithm>
#include <random>
#include <iostream>
#include <cassert>
#include <sstream>

#define RB_T RingBufferMt<int>

struct ThreadData {
    int value;
    RB_T* rb;
    std::atomic<int>* popSuccessCnt;
    std::atomic<int>* popFailureCnt;
    std::atomic<int>* popTailSuccessCnt;
    std::atomic<int>* popTailFailureCnt;
    std::atomic<int>* dryPutSuccessCnt;
    std::atomic<int>* dryPutFailureCnt;
};

void PopTask(const ThreadData* data) {
    RB_T* rb = data->rb;
    int* ret = rb->Pop();
    if (nullptr == ret) {
        ++(*(data->popFailureCnt));
    } else {
        ++(*(data->popSuccessCnt));
    }
}

void PopTailTask(const ThreadData* data) {
    RB_T* rb = data->rb;
    int* ret = rb->PopTail();
    if (nullptr == ret) {
        ++(*(data->popTailFailureCnt));
    } else {
        ++(*(data->popTailSuccessCnt));
    }
}

void DryPutTask(const ThreadData* data) {
    RB_T* rb = data->rb;
    int* ret = rb->DryPut();
    if (nullptr == ret) {
        ++(*(data->dryPutFailureCnt));
    } else {
        ++(*(data->dryPutSuccessCnt));
    }
}

bool runTestCase1() {
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<int> thDataValueDistribution(1, 10000);

    int N = 8;
    int popTaskCnt = 128, popTailTaskCnt = 64, dryPutTaskCnt = 1024; 

    RB_T rbmt(8);
    const int numThreads = (popTaskCnt + popTailTaskCnt + dryPutTaskCnt);
    std::vector<int> thTypes;
    for (int i = 0; i < popTaskCnt; ++i) {
        thTypes.emplace_back(1); 
    }
    for (int i = 0; i < popTailTaskCnt; ++i) {
        thTypes.emplace_back(2); 
    }
    for (int i = 0; i < dryPutTaskCnt; ++i) {
        thTypes.emplace_back(3); 
    }
    auto rng = std::default_random_engine {};
    std::shuffle(std::begin(thTypes), std::end(thTypes), rng);

    std::vector<std::thread> ths;
    std::vector<ThreadData> thData(numThreads);

    std::atomic<int> popSuccessCnt;
    std::atomic<int> popFailureCnt;
    std::atomic<int> popTailSuccessCnt;
    std::atomic<int> popTailFailureCnt;
    std::atomic<int> dryPutSuccessCnt;
    std::atomic<int> dryPutFailureCnt;

    int j = 0;
    while (!thTypes.empty()) {
        thData[j].value = thDataValueDistribution(rng);
        thData[j].rb    = &rbmt;
        thData[j].popSuccessCnt     = &popSuccessCnt;
        thData[j].popFailureCnt     = &popFailureCnt;
        thData[j].popTailSuccessCnt = &popTailSuccessCnt;
        thData[j].popTailFailureCnt = &popTailFailureCnt;
        thData[j].dryPutSuccessCnt  = &dryPutSuccessCnt;
        thData[j].dryPutFailureCnt  = &dryPutFailureCnt;

        int thType = thTypes.back();
        thTypes.pop_back();
        switch (thType) {
        case 1:
            ths.emplace_back(std::thread(&PopTask, &thData[j]));
        break;
        case 2:
            ths.emplace_back(std::thread(&PopTailTask, &thData[j]));
        break;
        case 3:
            ths.emplace_back(std::thread(&DryPutTask, &thData[j]));
        break;
        }
        ++j;
    }

    // Join all ths
    for (std::thread& th : ths) {
        if (th.joinable()) {
            th.join();
        }
    }

    std::ostringstream oss;
    oss << "popSuccessCnt=" << popSuccessCnt << "\npopFailureCnt=" << popFailureCnt << "\npopTailSuccessCnt=" << popTailSuccessCnt << "\npopTailFailureCnt=" << popTailFailureCnt << "\ndryPutSuccessCnt=" << dryPutSuccessCnt << "\ndryPutFailureCnt=" << dryPutFailureCnt << "\nrbmt.stat=" << rbmt.toSimpleStat();
    std::cout << oss.str() << std::endl;
    assert(0 <= rbmt.St && rbmt.N > rbmt.St);
    assert(0 <= rbmt.Ed && rbmt.N >= rbmt.Ed);
    assert(0 <= rbmt.Cnt && rbmt.N >= rbmt.Cnt);
    bool stCntEdConsistent1 = (rbmt.St + rbmt.Cnt == rbmt.Ed); 
    bool stCntEdConsistent2 = (rbmt.St + rbmt.Cnt >= rbmt.N && rbmt.St + rbmt.Cnt == rbmt.Ed + N); 
    assert(stCntEdConsistent1 || stCntEdConsistent2);
    std::cout << "Passed TestCase1" << std::endl;
    return true;
}

// Program entry point
int main(int argc, char** argv)
{
    runTestCase1();
	return 0;
}

#ifdef _WIN32
#include <windows.h>

int APIENTRY WinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow)
{
    return main(__argc, __argv);
}
#endif
