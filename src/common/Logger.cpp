#include "../../include/common/Logger.h"

Logger *Logger::current_instance = nullptr;

Logger::Logger() {
    this->is_killed = false;

    this->if_stage_log_file.open(LOG_DIRECTORY_PATH + IF_STAGE_LOG_FILE_NAME);
    this->id_stage_log_file.open(LOG_DIRECTORY_PATH + ID_STAGE_LOG_FILE_NAME);
    this->ex_stage_log_file.open(LOG_DIRECTORY_PATH + EX_STAGE_LOG_FILE_NAME);
    this->mem_stage_log_file.open(LOG_DIRECTORY_PATH + MEM_STAGE_LOG_FILE_NAME);
    this->wb_stage_log_file.open(LOG_DIRECTORY_PATH + WB_STAGE_LOG_FILE_NAME);
}

Logger::~Logger() {
    this->if_stage_log_file.close();
    this->id_stage_log_file.close();
    this->ex_stage_log_file.close();
    this->mem_stage_log_file.close();
    this->wb_stage_log_file.close();
}

Logger *Logger::init() {
    if (Logger::current_instance == nullptr) {
        Logger::current_instance = new Logger();
    }

    return Logger::current_instance;
}

void Logger::run() {
    std::thread if_stage_log_writer (&Logger::writeIFStageMessagesToFile, this);
    std::thread id_stage_log_writer (&Logger::writeIDStageMessagesToFile, this);
    std::thread ex_stage_log_writer (&Logger::writeEXStageMessagesToFile, this);
    std::thread mem_stage_log_writer (&Logger::writeMEMStageMessagesToFile, this);
    std::thread wb_stage_log_writer (&Logger::writeWBStageMessagesToFile, this);

    if_stage_log_writer.detach();
    id_stage_log_writer.detach();
    ex_stage_log_writer.detach();
    mem_stage_log_writer.detach();
    wb_stage_log_writer.detach();

    while (!this->is_killed) {
        sleep(1);
    }
}

void Logger::log(Stage current_stage, const std::string &message) {
    switch (current_stage) {
        case Stage::IF:
            this->enqueueIFStageMessage(message);
            break;

        case Stage::ID:
            this->enqueueIDStageMessage(message);
            break;

        case Stage::EX:
            this->enqueueEXStageMessage(message);
            break;

        case Stage::MEM:
            this->enqueueMEMStageMessage(message);
            break;

        case Stage::WB:
            this->enqueueWBStageMessage(message);
            break;
    }
}

void Logger::kill() {
    this->is_killed = true;

    this->if_stage_condition_variable.notify_all();
    this->id_stage_condition_variable.notify_all();
    this->ex_stage_condition_variable.notify_all();
    this->mem_stage_condition_variable.notify_all();
    this->wb_stage_condition_variable.notify_all();
}

void Logger::enqueueIFStageMessage(const std::string &message) {
    std::unique_lock<std::mutex> if_stage_lock (this->if_stage_mutex);
    this->if_stage_condition_variable.wait(
            if_stage_lock,
            [this] {
                return this->if_stage_messages_queue.size() != MAX_MESSAGE_QUEUE_SIZE;
            }
    );

    this->if_stage_messages_queue.push(message);
    this->if_stage_condition_variable.notify_one();
}

void Logger::enqueueIDStageMessage(const std::string &message) {
    std::unique_lock<std::mutex> id_stage_lock (this->id_stage_mutex);
    this->id_stage_condition_variable.wait(
            id_stage_lock,
            [this] {
                return this->id_stage_messages_queue.size() != MAX_MESSAGE_QUEUE_SIZE;
            }
    );

    this->id_stage_messages_queue.push(message);
    this->id_stage_condition_variable.notify_one();
}

void Logger::enqueueEXStageMessage(const std::string &message) {
    std::unique_lock<std::mutex> ex_stage_lock (this->ex_stage_mutex);
    this->ex_stage_condition_variable.wait(
            ex_stage_lock,
            [this] {
                return this->ex_stage_messages_queue.size() != MAX_MESSAGE_QUEUE_SIZE;
            }
    );

    this->ex_stage_messages_queue.push(message);
    this->ex_stage_condition_variable.notify_one();
}

void Logger::enqueueMEMStageMessage(const std::string &message) {
    std::unique_lock<std::mutex> mem_stage_lock (this->mem_stage_mutex);
    this->mem_stage_condition_variable.wait(
            mem_stage_lock,
            [this] {
                return this->mem_stage_messages_queue.size() != MAX_MESSAGE_QUEUE_SIZE;
            }
    );

    this->mem_stage_messages_queue.push(message);
    this->mem_stage_condition_variable.notify_one();
}

void Logger::enqueueWBStageMessage(const std::string &message) {
    std::unique_lock<std::mutex> wb_stage_lock (this->wb_stage_mutex);
    this->wb_stage_condition_variable.wait(
            wb_stage_lock,
            [this] {
                return this->wb_stage_messages_queue.size() != MAX_MESSAGE_QUEUE_SIZE;
            }
    );

    this->wb_stage_messages_queue.push(message);
    this->wb_stage_condition_variable.notify_one();
}

void Logger::writeIFStageMessagesToFile() {
    while (!this->is_killed) {
        std::unique_lock<std::mutex> if_stage_lock(this->if_stage_mutex);
        this->if_stage_condition_variable.wait(
                if_stage_lock,
                [this] {
                    return !this->if_stage_messages_queue.empty() || this->is_killed;
                }
        );

        while (!this->if_stage_messages_queue.empty()) {
            this->if_stage_log_file << this->if_stage_messages_queue.front() << std::endl;
            this->if_stage_messages_queue.pop();
        }

        this->if_stage_condition_variable.notify_one();
    }
}

void Logger::writeIDStageMessagesToFile() {
    while (!this->is_killed) {
        std::unique_lock<std::mutex> id_stage_lock(this->id_stage_mutex);
        this->id_stage_condition_variable.wait(
                id_stage_lock,
                [this] {
                    return !this->id_stage_messages_queue.empty() || this->is_killed;
                }
        );

        while (!this->id_stage_messages_queue.empty()) {
            this->id_stage_log_file << this->id_stage_messages_queue.front() << std::endl;
            this->id_stage_messages_queue.pop();
        }

        this->id_stage_condition_variable.notify_one();
    }
}

void Logger::writeEXStageMessagesToFile() {
    while (!this->is_killed) {
        std::unique_lock<std::mutex> ex_stage_lock(this->ex_stage_mutex);
        this->ex_stage_condition_variable.wait(
                ex_stage_lock,
                [this] {
                    return !this->ex_stage_messages_queue.empty() || this->is_killed;
                }
        );

        while (!this->ex_stage_messages_queue.empty()) {
            this->ex_stage_log_file << this->ex_stage_messages_queue.front() << std::endl;
            this->ex_stage_messages_queue.pop();
        }

        this->ex_stage_condition_variable.notify_one();
    }
}

void Logger::writeMEMStageMessagesToFile() {
    while (!this->is_killed) {
        std::unique_lock<std::mutex> mem_stage_lock(this->mem_stage_mutex);
        this->mem_stage_condition_variable.wait(
                mem_stage_lock,
                [this] {
                    return !this->mem_stage_messages_queue.empty() || this->is_killed;
                }
        );

        while (!this->mem_stage_messages_queue.empty()) {
            this->mem_stage_log_file << this->mem_stage_messages_queue.front() << std::endl;
            this->mem_stage_messages_queue.pop();
        }

        this->mem_stage_condition_variable.notify_one();
    }
}

void Logger::writeWBStageMessagesToFile() {
    while (!this->is_killed) {
        std::unique_lock<std::mutex> wb_stage_lock(this->wb_stage_mutex);
        this->wb_stage_condition_variable.wait(
                wb_stage_lock,
                [this] {
                    return !this->wb_stage_messages_queue.empty() || this->is_killed;
                }
        );

        while (!this->wb_stage_messages_queue.empty()) {
            this->wb_stage_log_file << this->wb_stage_messages_queue.front() << std::endl;
            this->wb_stage_messages_queue.pop();
        }

        this->wb_stage_condition_variable.notify_one();
    }
}