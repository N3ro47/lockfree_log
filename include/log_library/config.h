#pragma once

#ifndef LOG_ACTIVE_LEVEL
#define LOG_ACTIVE_LEVEL LOG_LEVEL_DEBUG
#endif

enum LogLevel {
  LOG_LEVEL_DEBUG = 0,
  LOG_LEVEL_INFO = 1,
  LOG_LEVEL_WARN = 2,
  LOG_LEVEL_ERROR = 3,
  LOG_LEVEL_NONE = 4
};

constexpr const char* to_string(LogLevel level) {
  switch (level){
    case LOG_LEVEL_DEBUG: return "DEBUG";
    case LOG_LEVEL_INFO: return "INFO";
    case LOG_LEVEL_WARN: return "WARN";
    case LOG_LEVEL_ERROR: return "ERROR";
    case LOG_LEVEL_NONE: return "NONE";
    }
    return "????";
}
