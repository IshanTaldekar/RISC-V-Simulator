#include "../../include/state/DataMemory.h"

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
}

DataMemory *DataMemory::init() {
    if (DataMemory::current_instance == nullptr) {
        DataMemory::current_instance = new DataMemory();
    }

    return DataMemory::current_instance;
}

void DataMemory::run() {

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
}

void DataMemory::setWriteData(unsigned long value) {
    std::lock_guard<std::mutex> data_memory_guard (this->getModuleMutex());

    this->write_data = value;
    this->is_write_data_set = true;
}

void DataMemory::setMemWrite(bool is_asserted) {
    std::lock_guard<std::mutex> data_memory_guard (this->getModuleMutex());

    this->is_mem_write_asserted = is_asserted;
    this->is_mem_write_flag_set = true;
}

void DataMemory::setMemRead(bool is_asserted) {
    std::lock_guard<std::mutex> data_memory_guard (this->getModuleMutex());

    this->is_mem_read_asserted = is_asserted;
    this->is_mem_read_flag_set = true;
}

void DataMemory::readDataMemoryFile() {
    std::lock_guard<std::mutex> data_memory_guard (this->getModuleMutex());

    std::ifstream data_memory_file (this->data_memory_file_path);
    std::string byte_instruction;

    this->data = std::vector<std::string> {};

    while (std::getline(data_memory_file, byte_instruction)) {
        this->data.push_back(byte_instruction);
    }

    this->is_input_file_read = true;
}

void DataMemory::passReadData() {
    // TODO
}