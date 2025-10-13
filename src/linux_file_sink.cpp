#include <log_library/sink.h>
#include <log_library/file_sink_config.h>
#include "file_rotation.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <filesystem>

namespace log_library {

class LinuxFileSink : public Sink {
public:
    explicit LinuxFileSink(const FileSinkConfig& config = {}) 
        : config_(config), fd_(-1), mapped_memory_(nullptr), current_offset_(0) {
        initialize();
    }
    
    ~LinuxFileSink() override {
        cleanup();
    }
    
    void write(const std::string& message, LogLevel level) override {
        if (!mapped_memory_) {
            return;
        }
        
        if (current_offset_ + message.size() > config_.max_file_size) {
            if (!rotate_file()) {
                return;
            }
        }
        
        std::memcpy(static_cast<char*>(mapped_memory_) + current_offset_, 
                   message.data(), message.size());
        current_offset_ += message.size();
        
        if (config_.fsync_on_error && level >= LOG_LEVEL_ERROR) {
            sync_to_disk();
        }
    }
    
    void flush() override {
        sync_to_disk();
    }
    
private:
    FileSinkConfig config_;
    int fd_;
    void* mapped_memory_;
    size_t current_offset_;
    
    void initialize() {
        if (!FileRotationUtils::ensure_log_directory(config_)) {
            throw std::runtime_error("Failed to create log directory");
        }
        
        if (!create_and_map_file()) {
            throw std::runtime_error("Failed to create and map log file");
        }
    }
    
    bool create_and_map_file() {
        std::string file_path = FileRotationUtils::get_current_log_path(config_);
        
        fd_ = open(file_path.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0644);
        if (fd_ == -1) {
            return false;
        }
        
        if (posix_fallocate(fd_, 0, config_.max_file_size) != 0) {
            close(fd_);
            fd_ = -1;
            return false;
        }
        
        mapped_memory_ = mmap(nullptr, config_.max_file_size, 
                              PROT_WRITE, MAP_SHARED, fd_, 0);
        
        if (mapped_memory_ == MAP_FAILED) {
            mapped_memory_ = nullptr;
            close(fd_);
            fd_ = -1;
            return false;
        }
        
        current_offset_ = 0;
        return true;
    }
    
    bool rotate_file() {
        if (mapped_memory_) {
            munmap(mapped_memory_, config_.max_file_size);
            mapped_memory_ = nullptr;
        }
        
        if (fd_ != -1) {
            close(fd_);
            fd_ = -1;
        }
        
        if (!FileRotationUtils::rotate_log_files(config_)) {
            return false;
        }
        
        FileRotationUtils::cleanup_old_files(config_);
        
        return create_and_map_file();
    }
    
    void sync_to_disk() {
        if (!mapped_memory_) {
            return;
        }
        
        msync(mapped_memory_, current_offset_, MS_SYNC);
        
        if (fd_ != -1) {
            fsync(fd_);
        }
    }
    
    void cleanup() {
        if (mapped_memory_) {
            munmap(mapped_memory_, config_.max_file_size);
            mapped_memory_ = nullptr;
        }
        
        if (fd_ != -1) {
            close(fd_);
            fd_ = -1;
        }
    }
};

} // namespace log_library
