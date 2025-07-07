#ifndef DEBUG_LOG_H_
#define DEBUG_LOG_H_ 1

#pragma once
#include <stdio.h>
#include <string>
#include <sstream>
#include "joltc_api.h"

extern "C"
{
    //Create a callback delegate
    typedef void(*FuncCallBack)(const char* message, int color, int size);
    static FuncCallBack callbackInstance = nullptr;
    JPH_CAPI void RegisterDebugCallback(FuncCallBack cb);
}

//Color Enum
enum class DColor { Red, Green, Blue, Black, White, Yellow, Orange };

class Debug
{
public:
    static void Log(const char* message, DColor color = DColor::Black);
    static void Log(const std::string message, DColor color = DColor::Black);
    static void Log(const int message, DColor color = DColor::Black);
    static void Log(const char message, DColor color = DColor::Black);
    static void Log(const float message, DColor color = DColor::Black);
    static void Log(const double message, DColor color = DColor::Black);
    static void Log(const bool message, DColor color = DColor::Black);

private:
    static void send_log(const std::stringstream &ss, const DColor&color);
};

#endif /* DEBUG_LOG_H_ */
