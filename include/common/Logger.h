#ifndef RISC_V_SIMULATOR_LOGGER_H
#define RISC_V_SIMULATOR_LOGGER_H

#include <mutex>
#include <condition_variable>
#include <string>
#include <fstream>
#include <queue>

#include "Config.h"

class Logger {
    std::mutex if_stage_mutex;
    std::mutex id_stage_mutex;
    std::mutex ex_stage_mutex;
    std::mutex mem_stage_mutex;
    std::mutex wb_stage_mutex;

    std::condition_variable if_stage_condition_variable;
    std::condition_variable id_stage_condition_variable;
    std::condition_variable ex_stage_condition_variable;
    std::condition_variable mem_stage_condition_variable;
    std::condition_variable wb_stage_condition_variable;

    std::queue<std::string> if_stage_messages_queue;
    std::queue<std::string> id_stage_messages_queue;
    std::queue<std::string> ex_stage_messages_queue;
    std::queue<std::string> mem_stage_messages_queue;
    std::queue<std::string> wb_stage_messages_queue;

    const std::string LOG_DIRECTORY_PATH = "../logs/";

    const std::string IF_STAGE_LOG_FILE_NAME = "IFStage.log";
    const std::string ID_STAGE_LOG_FILE_NAME = "IDStage.log";
    const std::string EX_STAGE_LOG_FILE_NAME = "EXStage.log";
    const std::string MEM_STAGE_LOG_FILE_NAME = "MEMStage.log";
    const std::string WB_STAGE_LOG_FILE_NAME = "WBStage.log";

    const int MAX_MESSAGE_QUEUE_SIZE = 20;

    bool is_killed;

    std::ofstream if_stage_log_file;
    std::ofstream id_stage_log_file;
    std::ofstream ex_stage_log_file;
    std::ofstream mem_stage_log_file;
    std::ofstream wb_stage_log_file;

    static Logger *current_instance;
    static std::mutex initialization_mutex;

public:
    Logger();
    ~Logger();

    static Logger *init();

    void log(Stage current_stage, const std::string &message);
    void kill();

private:
    void enqueueIFStageMessage(const std::string &message);
    void enqueueIDStageMessage(const std::string &message);
    void enqueueEXStageMessage(const std::string &message);
    void enqueueMEMStageMessage(const std::string &message);
    void enqueueWBStageMessage(const std::string &message);

    void writeIFStageMessagesToFile();
    void writeIDStageMessagesToFile();
    void writeEXStageMessagesToFile();
    void writeMEMStageMessagesToFile();
    void writeWBStageMessagesToFile();
};

#endif //RISC_V_SIMULATOR_LOGGER_H
