#include "../../../include/common/logger/IDLogger.h"

IDLogger *IDLogger::current_instance = nullptr;

IDLogger::IDLogger() {
    this->log_file_path = ID_STAGE_LOG_FILE_PATH;
}

IDLogger *IDLogger::init() {
    if (IDLogger::current_instance == nullptr) {
        IDLogger::current_instance = new IDLogger();
    }

    return IDLogger::current_instance;
}

void IDLogger::log(const std::string &text) {
    std::unique_lock<std::mutex> logger_lock (this->log_file_mutex);

    this->log_file << text << std::endl;
}