#include "DebugLog.h"

//-------------------------------------------------------------------
void  Debug::Log(const char* message, DColor color) {
    if (callbackInstance != nullptr)
        callbackInstance(message, (int)color, (int)strlen(message));
}

void  Debug::Log(const std::string message, DColor color) {
    const char* tmsg = message.c_str();
    if (callbackInstance != nullptr)
        callbackInstance(tmsg, (int)color, (int)strlen(tmsg));
}

void  Debug::Log(const int message, DColor color) {
    std::stringstream ss;
    ss << message;
    send_log(ss, color);
}

void  Debug::Log(const char message, DColor color) {
    std::stringstream ss;
    ss << message;
    send_log(ss, color);
}

void  Debug::Log(const float message, DColor color) {
    std::stringstream ss;
    ss << message;
    send_log(ss, color);
}

void  Debug::Log(const double message, DColor color) {
    std::stringstream ss;
    ss << message;
    send_log(ss, color);
}

void Debug::Log(const bool message, DColor color) {
    std::stringstream ss;
    if (message)
        ss << "true";
    else
        ss << "false";

    send_log(ss, color);
}

void Debug::send_log(const std::stringstream &ss, const DColor &color) {
    const std::string tmp = ss.str();
    const char* tmsg = tmp.c_str();
    if (callbackInstance != nullptr)
        callbackInstance(tmsg, (int)color, (int)strlen(tmsg));
}
//-------------------------------------------------------------------

//Create a callback delegate
void RegisterDebugCallback(FuncCallBack cb) {
    callbackInstance = cb;
}
