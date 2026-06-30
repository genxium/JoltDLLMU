#ifndef DEBUG_LOG_H_
#define DEBUG_LOG_H_ 1

#include <string>
#include "joltc_api.h"

extern "C"
{
    //Create a callback delegate
    typedef void(*FuncCallBack)(const char* message, int color, int size);
    static FuncCallBack callbackInstance = nullptr;
    JPH_CAPI void RegisterDebugCallback(FuncCallBack cb);
}

//Color Enum
enum class DColor { Red, Yellow, Orange, Green, Blue, Black, White };

class Debug
{
public:
    static void Log(const char* message, DColor color = DColor::White);
    static void Log(const std::string message, DColor color = DColor::White);
};

#endif /* DEBUG_LOG_H_ */
