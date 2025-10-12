#include <log_library/sink.h>
#include <log_library/file_sink_config.h>
#include "file_rotation.cpp"
#include <windows.h>
#include <cstring>
#include <stdexcept>
#include <filesystem>

namespace log_library {

class WindowsFileSink : public Sink {
public:
    explicit WindowsFileSink(const FileSinkConfig& config = {}) 
        : config_(config), file_handle_(INVALID_HANDLE_VALUE), current_offset_(0) {
        initialize();
    }
    
    ~WindowsFileSink() override {
        cleanup();
    }
    
    void write(const std::string& message, LogLevel level) override {
        if (file_handle_ == INVALID_HANDLE_VALUE) {
            return;
        }
        
        if (current_offset_ + message.size() > config_.max_file_size) {
            if (!rotate_file()) {
                return;
            }
        }
        
        DWORD bytes_written;
        if (WriteFile(file_handle_, message.data(), static_cast<DWORD>(message.size()), 
                     &bytes_written, nullptr)) {
            current_offset_ += bytes_written;
            
            if (config_.fsync_on_error && level >= LOG_LEVEL_ERROR) {
                FlushFileBuffers(file_handle_);
            }
        }
    }
    
    void flush() override {
        if (file_handle_ != INVALID_HANDLE_VALUE) {
            FlushFileBuffers(file_handle_);
        }
    }
    
private:
    FileSinkConfig config_;
    HANDLE file_handle_;
    size_t current_offset_;
    
    void initialize() {
        if (!FileRotationUtils::ensure_log_directory(config_)) {
            throw std::runtime_error("Failed to create log directory");
        }
        
        if (!create_file()) {
            throw std::runtime_error("Failed to create log file");
        }
    }
    
    bool create_file() {
        std::string file_path = FileRotationUtils::get_current_log_path(config_);
        
        std::wstring wide_path(file_path.begin(), file_path.end());
        
        file_handle_ = CreateFileW(
            wide_path.c_str(),
            GENERIC_WRITE,
            FILE_SHARE_READ,
            nullptr,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );
        
        if (file_handle_ == INVALID_HANDLE_VALUE) {
            return false;
        }
        
        current_offset_ = 0;
        return true;
    }
    
    bool rotate_file() {
        if (file_handle_ != INVALID_HANDLE_VALUE) {
            CloseHandle(file_handle_);
            file_handle_ = INVALID_HANDLE_VALUE;
        }
        
        if (!FileRotationUtils::rotate_log_files(config_)) {
            return false;
        }
        
        FileRotationUtils::cleanup_old_files(config_);
        
        return create_file();
    }
    
    void cleanup() {
        if (file_handle_ != INVALID_HANDLE_VALUE) {
            CloseHandle(file_handle_);
            file_handle_ = INVALID_HANDLE_VALUE;
        }
    }
};

} // namespace log_library
