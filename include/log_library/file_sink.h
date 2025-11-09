#pragma once

#include <log_library/file_sink_config.h>
#include <log_library/sink.h>
#include <memory>

namespace log_library {

std::unique_ptr<Sink> create_file_sink(const FileSinkConfig& config = {});

}  // namespace log_library