#include "../../../include/common/logger/IFLogger.h"

IFLogger::IFLogger() {
    this->log_file_path = IF_STAGE_LOG_FILE_PATH;
}

IFLogger *IFLogger::init() {
    if (IFLogger::current_instance == nullptr) {
        IFLogger::current_instance = new IFLogger();
    }

    return IFLogger::current_instance;
}

void IFLogger::log(const std::string &text) {
    std::unique_lock<std::mutex> logger_lock (this->log_file_mutex);

    this->log_file << text << std::endl;
}