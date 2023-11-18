#include "../../../include/common/logger/WBLogger.h"

WBLogger *WBLogger::current_instance = nullptr;

WBLogger::WBLogger() {
    this->log_file_path = WB_STAGE_LOG_FILE_PATH;
    this->openFile();
}

WBLogger *WBLogger::init() {
    if (WBLogger::current_instance == nullptr) {
        WBLogger::current_instance = new WBLogger();
    }

    return WBLogger::current_instance;
}

void WBLogger::log(const std::string &text) {
    std::unique_lock<std::mutex> logger_lock (this->log_file_mutex);

    this->log_file << text << std::endl;
}