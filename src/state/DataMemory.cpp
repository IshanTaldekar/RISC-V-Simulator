#include "../../include/state/DataMemory.h"

DataMemory *DataMemory::current_instance = nullptr;

DataMemory::DataMemory() {
    this->address = 0UL;
    this->write_data = 0UL;
    this->read_data = 0UL;

    this->is_mem_write_asserted = false;
    this->is_mem_read_asserted = false;

    this->is_address_set = false;
    this->is_write_data_set = false;
    this->is_read_data_set = false;
    this->is_mem_write_flag_set = false;
    this->is_mem_read_flag_set = false;
    this->is_input_file_read = false;

    this->mem_wb_stage_registers = MEMWBStageRegisters::init();
}

DataMemory *DataMemory::init() {
    if (DataMemory::current_instance == nullptr) {
        DataMemory::current_instance = new DataMemory();
    }

    return DataMemory::current_instance;
}

void DataMemory::run() {
    std::unique_lock<std::mutex> data_memory_lock (this->getModuleMutex());
    this->getModuleConditionVariable().wait(
            data_memory_lock,
            [this] { return !this->data_memory_file_path.empty(); }
    );

    this->readDataMemoryFile();

    while (this->isAlive()) {
        this->getModuleConditionVariable().wait(
                data_memory_lock,
                [this] {
                    return this->is_address_set && this->is_write_data_set && this->is_read_data_set &&
                            this->is_mem_write_flag_set && this->is_mem_read_flag_set && this->is_input_file_read;
                }
        );

        this->readData();
        this->writeData();
        this->passReadData();

        this->is_address_set = false;
        this->is_write_data_set = false;
        this->is_read_data_set = false;
        this->is_mem_write_flag_set = false;
        this->is_mem_read_flag_set =  false;
    }
}

void DataMemory::notifyModuleConditionVariable() {
    this->getModuleConditionVariable().notify_one();
}

void DataMemory::setDataMemoryInputFilePath(const std::string &file_path) {
    if (this->is_input_file_read) {
        std::cerr << "[DataMemory] input file set again." << std::endl;
    }

    std::lock_guard<std::mutex> data_memory_lock (this->getModuleMutex());

    this->data_memory_file_path = file_path;
}

void DataMemory::setAddress(unsigned long value) {
    std::lock_guard<std::mutex> data_memory_guard (this->getModuleMutex());

    this->address = value;
    this->is_address_set = true;

    this->notifyModuleConditionVariable();
}

void DataMemory::setWriteData(unsigned long value) {
    std::lock_guard<std::mutex> data_memory_guard (this->getModuleMutex());

    this->write_data = value;
    this->is_write_data_set = true;

    this->notifyModuleConditionVariable();
}

void DataMemory::setMemWrite(bool is_asserted) {
    std::lock_guard<std::mutex> data_memory_guard (this->getModuleMutex());

    this->is_mem_write_asserted = is_asserted;
    this->is_mem_write_flag_set = true;

    this->notifyModuleConditionVariable();
}

void DataMemory::setMemRead(bool is_asserted) {
    std::lock_guard<std::mutex> data_memory_guard (this->getModuleMutex());

    this->is_mem_read_asserted = is_asserted;
    this->is_mem_read_flag_set = true;

    this->notifyModuleConditionVariable();
}

void DataMemory::readDataMemoryFile() {
    std::lock_guard<std::mutex> data_memory_guard (this->getModuleMutex());

    std::ifstream data_memory_file (this->data_memory_file_path);
    std::string byte_instruction;

    this->data_memory = std::vector<std::string> {};

    while (std::getline(data_memory_file, byte_instruction)) {
        this->data_memory.push_back(byte_instruction);
    }

    this->is_input_file_read = true;
}

void DataMemory::passReadData() {
    this->mem_wb_stage_registers->setReadData(this->read_data);
}

void DataMemory::readData() {
    this->read_data = 0UL;

    if (this->is_mem_read_asserted) {
        std::string word;

        for (unsigned long i = this->address; i < this->address + 4; ++i) {
            if (this->data_memory.size() > i) {
                word += this->data_memory.at(i);
            } else {
                std::cerr << "[DataMemory] Read address out of bounds." << std::endl;
            }
        }

        this->read_data = std::bitset<WORD_BIT_COUNT>(word).to_ulong();
    }
}

void DataMemory::writeData() {
    if (this->is_write_data_set && this->is_mem_write_asserted) {
        std::string word = std::bitset<WORD_BIT_COUNT>(this->write_data).to_string();

        for (int i = 0; i < 4; ++i) {
            this->data_memory.at(this->address + i) = word.substr(i * 8, 8);
        }
    }
}