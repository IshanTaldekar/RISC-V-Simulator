#include "../../../include/common/logger/Logger.h"

void Logger::openFile() {
    try {
        this->log_file.open(this->log_file_path);
    } catch (const std::ios_base::failure &error) {
        std::cerr << "Failed to open log file (" << this->log_file_path << "): " << error.what() << std::endl;
    }
}