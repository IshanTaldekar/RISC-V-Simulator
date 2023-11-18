#include "../../../include/common/logger/EXLogger.h"

EXLogger *EXLogger::current_instance = nullptr;

EXLogger::EXLogger() {
    this->log_file_path = EX_STAGE_LOG_FILE_PATH;
}

EXLogger *EXLogger::init() {
    if (EXLogger::current_instance == nullptr) {
        EXLogger::current_instance = new EXLogger();
    }

    return EXLogger::current_instance;
}

void EXLogger::log(const std::string &text) {
    std::unique_lock<std::mutex> logger_lock (this->log_file_mutex);

    this->log_file << text << std::endl;
}