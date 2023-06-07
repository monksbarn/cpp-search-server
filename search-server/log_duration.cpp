#include "log_duration.h"

LogDuration::LogDuration(const std::string_view& msg) :msg_(msg) {
}
LogDuration::LogDuration(const std::string_view& msg, std::ostream& out) :msg_(msg) {
    out << std::endl;
}
LogDuration::~LogDuration() {
    using namespace std::chrono;
    using namespace std::literals;

    const auto end_time = Clock::now();
    const auto dur = end_time - start_time_;
    if (msg_ == "Long task") {
        std::cerr << "Operation time: "s << duration_cast<milliseconds>(dur).count() << " ms"s << std::endl;
    }
    else {
        std::cerr << msg_ << ": "s << duration_cast<milliseconds>(dur).count() << " ms"s << std::endl;
    }
}