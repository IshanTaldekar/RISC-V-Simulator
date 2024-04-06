#include "../../include/state/DataMemory.h"

DataMemory *DataMemory::current_instance = nullptr;
std::mutex DataMemory::initialization_mutex;

DataMemory::DataMemory() {
    this->address = 0UL;
    this->write_data = std::bitset<WORD_BIT_COUNT>(std::string(32, '0'));
    this->read_data = std::bitset<WORD_BIT_COUNT>(std::string(32, '0'));

    this->is_mem_write_asserted = false;
    this->is_mem_read_asserted = false;

    this->is_address_set = false;
    this->is_write_data_set = false;
    this->is_mem_write_flag_set = false;
    this->is_mem_read_flag_set = false;
    this->is_input_file_read = false;
    this->is_reset_flag_set = false;

    this->mem_wb_stage_registers = nullptr;
    this->logger = nullptr;
    this->stage_synchronizer = nullptr;

    this->output_file_path = "../output/DataMemory";
    std::ofstream output_file (this->output_file_path + "-SS.log", std::ios::out);
    output_file.close();
}

DataMemory *DataMemory::init() {
    std::lock_guard<std::mutex> data_memory_lock (DataMemory::initialization_mutex);

    if (DataMemory::current_instance == nullptr) {
        DataMemory::current_instance = new DataMemory();
    }

    return DataMemory::current_instance;
}

void DataMemory::initDependencies() {
    std::unique_lock<std::mutex> data_memory_lock (this->getModuleDependencyMutex());

    if (this->mem_wb_stage_registers && this->logger) {
        return;
    }

    this->mem_wb_stage_registers = MEMWBStageRegisters::init();
    this->logger = Logger::init();
    this->stage_synchronizer = StageSynchronizer::init();
}

void DataMemory::run() {
    this->initDependencies();

    this->log("Waiting for data memory file to be set and acquire lock.");
    {
        std::unique_lock<std::mutex> data_memory_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                data_memory_lock,
                [this] { return !this->data_memory_file_path.empty(); }
        );
    }

    while (this->isAlive()) {
        this->log("Waiting to be woken up and acquire lock.");

        std::unique_lock<std::mutex> data_memory_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                data_memory_lock,
                [this] {
                    return (this->is_address_set && this->is_write_data_set &&
                            this->is_mem_write_flag_set && this->is_mem_read_flag_set && this->is_input_file_read) ||
                            this->is_reset_flag_set || this->isKilled();
                }
        );

        if (this->is_reset_flag_set) {
            this->log("Resetting stage.");

            this->resetState();
            this->is_reset_flag_set = false;

            this->stage_synchronizer->arriveReset();

            this->log("Reset.");
            continue;
        }

        if (this->isKilled()) {
            this->log("Killed.");
            break;
        }

        this->log("Woken up and acquired lock.");

        this->writeData();
        this->readData();

        std::thread pass_read_data_thread (&DataMemory::passReadData, this, this->read_data);
        pass_read_data_thread.detach();

        this->is_mem_write_asserted = false;
        this->is_mem_read_asserted = false;
        this->is_address_set = false;
        this->is_write_data_set = false;
        this->is_mem_write_flag_set = false;
        this->is_mem_read_flag_set =  false;
    }
}

void DataMemory::setDataMemoryInputFilePath(const std::string &file_path) {
    this->log("setDataMemoryInputFilePath waiting to acquire lock.");

    std::lock_guard<std::mutex> data_memory_lock (this->getModuleMutex());

    this->log("setDataMemoryInputFilePath acquired lock. Updating value.");

    this->data_memory_file_path = file_path;
    this->readDataMemoryFile();

    this->log("setDataMemoryInputFilePath updated value.");
    this->notifyModuleConditionVariable();
}

void DataMemory::setAddress(unsigned long value) {
    this->log("setAddress waiting to acquire lock.");

    std::lock_guard<std::mutex> data_memory_guard (this->getModuleMutex());

    this->log("setAddress acquired lock. Updating value.");

    this->address = value;
    this->is_address_set = true;

    this->log("setAddress updated value.");
    this->notifyModuleConditionVariable();
}

void DataMemory::setWriteData(std::bitset<WORD_BIT_COUNT> value) {
    this->log("setWriteData waiting to acquire lock.");

    std::lock_guard<std::mutex> data_memory_guard (this->getModuleMutex());

    this->log("setWriteData acquired lock. Updating value.");

    this->write_data = value;
    this->is_write_data_set = true;

    this->log("setWriteData updated value.");
    this->notifyModuleConditionVariable();
}

void DataMemory::setMemWrite(bool is_asserted) {
    this->log("setMemWrite waiting to acquire lock.");

    std::lock_guard<std::mutex> data_memory_guard (this->getModuleMutex());

    this->log("setMemWrite acquired lock. Updating value.");

    this->is_mem_write_asserted = is_asserted;
    this->is_mem_write_flag_set = true;

    this->log("setMemWrite updated value.");
    this->notifyModuleConditionVariable();
}

void DataMemory::setMemRead(bool is_asserted) {
    this->log("setMemRead waiting to acquire lock.");

    std::lock_guard<std::mutex> data_memory_guard (this->getModuleMutex());

    this->log("setMemRead acquired lock. Updating value.");

    this->is_mem_read_asserted = is_asserted;
    this->is_mem_read_flag_set = true;

    this->log("setMemRead updated value.");
    this->notifyModuleConditionVariable();
}

void DataMemory::readDataMemoryFile() {
    std::ifstream data_memory_file (this->data_memory_file_path);
    std::string byte_instruction;

    this->data_memory.clear();

    while (std::getline(data_memory_file, byte_instruction)) {
        this->data_memory.push_back(byte_instruction);
    }

    int data_memory_size = static_cast<int>(this->data_memory.size());

    for (int i = 0; i < 1000 - data_memory_size; ++i) {
        this->data_memory.emplace_back(8, '0');
    }

    this->is_input_file_read = true;
    data_memory_file.close();

    this->log("Data memory file read.");
}

void DataMemory::passReadData(std::bitset<WORD_BIT_COUNT> data) {
    this->log("Passing read data to MEMWBStageRegisters.");
    this->mem_wb_stage_registers->setReadData(data);
    this->log("Passing read data to MEMWBStageRegisters.");
}

void DataMemory::readData() {
    this->read_data = std::bitset<WORD_BIT_COUNT>(std::string(32, '0'));

    if (this->is_mem_read_asserted) {
        std::string word;

        for (unsigned long i = this->address; i < this->address + 4; ++i) {
            if (this->data_memory.size() > i) {
                word += this->data_memory.at(i);
            } else {
                std::cerr << "[DataMemory] Read address out of bounds." << std::endl;
            }
        }

        this->read_data = std::bitset<WORD_BIT_COUNT>(word);
        this->log("Data memory read.");
    }
}

void DataMemory::writeData() {
    if (this->is_write_data_set && this->is_mem_write_asserted) {
        std::string word = std::bitset<WORD_BIT_COUNT>(this->write_data).to_string();

        for (int i = 0; i < 4; ++i) {
            this->data_memory.at(this->address + i) = word.substr(i * 8, 8);
        }

        this->log("Data memory written.");
    }
}

void DataMemory::reset() {
    std::lock_guard<std::mutex> data_memory_lock (this->getModuleMutex());
    this->is_reset_flag_set = true;
    this->notifyModuleConditionVariable();
}

void DataMemory::resetState() {
    this->readDataMemoryFile();

    this->address = 0UL;
    this->write_data = std::bitset<WORD_BIT_COUNT>(std::string(32, '0'));
    this->read_data = std::bitset<WORD_BIT_COUNT>(std::string(32, '0'));

    this->is_address_set = false;
    this->is_write_data_set = false;
    this->is_mem_write_flag_set = false;
    this->is_mem_read_flag_set = false;
    this->is_mem_write_asserted = false;
    this->is_mem_read_asserted = false;
    this->is_input_file_read = true;
}

void DataMemory::writeDataMemoryContentsToOutput() {
    std::ofstream output_file (
            this->output_file_path + (this->getPipelineType() == PipelineType::Single ?  "-SS.log" : "-FS.log"),
            std::ios::app
    );

    for (const std::string &data: this->data_memory) {
        output_file << data << std::endl;
    }

    output_file.close();
}

std::string DataMemory::getModuleTag() {
    return "DataMemory";
}

Stage DataMemory::getModuleStage() {
    return Stage::MEM;
}

void DataMemory::clearDataMemoryOutputFile() {
    std::ofstream output_file (
            this->output_file_path + (this->getPipelineType() == PipelineType::Single ?  "-SS.log" : "-FS.log"),
            std::ios::out
    );
    output_file.close();
}