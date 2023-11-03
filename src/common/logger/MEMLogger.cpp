#include "../../../include/common/logger/MEMLogger.h"

MEMLogger::MEMLogger() {
    this->log_file_path = MEM_STAGE_LOG_FILE_PATH;
}

MEMLogger *MEMLogger::init() {
    if (MEMLogger::current_instance == nullptr) {
        MEMLogger::current_instance = new MEMLogger();
    }

    return MEMLogger::current_instance;
}

void MEMLogger::log(const std::string &text) {
    std::unique_lock<std::mutex> logger_lock (this->log_file_mutex);

    this->log_file << text << std::endl;
}