#pragma once

#include <string>
#include <log_library/config.h>

namespace log_library {


class Sink {
public:
    virtual ~Sink() = default;
    
    virtual void write(const std::string& message, LogLevel level) = 0;
    
    virtual void flush() = 0;
};

} // namespace log_library
