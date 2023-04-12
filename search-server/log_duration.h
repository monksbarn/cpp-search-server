#pragma once
#include <string>
#include <chrono>
#include <iostream>

#define PROFILE_CONCAT_INTERNAL(X, Y) X ## Y
#define PROFILE_CONCAT(X, Y) PROFILE_CONCAT_INTERNAL(X, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT(profileGuard, __LINE__)
#define LOG_DURATION(x) LogDuration UNIQUE_VAR_NAME_PROFILE(x)
#define LOG_DURATION_STREAM(x,y) LogDuration UNIQUE_VAR_NAME_PROFILE(x,y)

class LogDuration {
public:
    // заменим имя типа std::chrono::steady_clock
    // с помощью using для удобства
    using Clock = std::chrono::steady_clock;
    LogDuration() = default;
    LogDuration(const std::string& msg);
    LogDuration(const std::string& msg, std::ostream& out);

    ~LogDuration();

private:
    const Clock::time_point start_time_ = Clock::now();
    std::string msg_;
};