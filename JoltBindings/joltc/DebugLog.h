#ifndef DEBUG_LOG_H_
#define DEBUG_LOG_H_ 1

#pragma once
#include <stdio.h>
#include <string>
#include <stdio.h>
#include <sstream>
#include "joltc_export.h"

extern "C"
{
    //Create a callback delegate
    typedef void(*FuncCallBack)(const char* message, int color, int size);
    static FuncCallBack callbackInstance = nullptr;
    JOLTC_EXPORT void RegisterDebugCallback(FuncCallBack cb);
}

//Color Enum
enum class Color { Red, Green, Blue, Black, White, Yellow, Orange };

class  Debug
{
public:
    static void Log(const char* message, Color color = Color::Black);
    static void Log(const std::string message, Color color = Color::Black);
    static void Log(const int message, Color color = Color::Black);
    static void Log(const char message, Color color = Color::Black);
    static void Log(const float message, Color color = Color::Black);
    static void Log(const double message, Color color = Color::Black);
    static void Log(const bool message, Color color = Color::Black);

private:
    static void send_log(const std::stringstream &ss, const Color &color);
};

#endif /* DEBUG_LOG_H_ */
