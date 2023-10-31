#include "../../include/state/InstructionMemory.h"

InstructionMemory::InstructionMemory() {
    this->instruction_memory_file_path = "";
    this->is_instruction_file_read = false;

    this->if_id_stage_registers = IFIDStageRegisters::init();

    this->program_counter = -1;
    this->is_new_program_counter_set = false;
}

InstructionMemory *InstructionMemory::init() {
    if (InstructionMemory::current_instance == nullptr) {
        InstructionMemory::current_instance = new InstructionMemory();
    }

    return InstructionMemory::current_instance;
}

void InstructionMemory::run() {
    while (this->isAlive()) {
        std::unique_lock<std::mutex> instruction_memory_lock(this->getMutex());
        this->getConditionVariable().wait(
                instruction_memory_lock,
                [this] { return this->is_instruction_file_read && this->is_new_program_counter_set; }
        );

        this->fetchInstructionFromMemory();
        this->loadInstructionIntoIFIDStageRegisters();

        this->is_new_program_counter_set = false;
    }
}

void InstructionMemory::setInstructionMemoryFilePath(const std::string &file_path) {
    if (!this->instruction_memory_file_path.empty()) {
        throw std::runtime_error("instruction_memory_file_path being reset in InstructionMemory");
    }

    std::unique_lock<std::mutex> instruction_memory_lock(this->getMutex());

    this->instruction_memory_file_path = file_path;
    this->readInstructionMemoryFile();
}

void InstructionMemory::setProgramCounter(int value) {
    std::unique_lock<std::mutex> instruction_memory_lock(this->getMutex());

    this->program_counter = value;
    this->is_new_program_counter_set = true;
}

void InstructionMemory::fetchInstructionFromMemory() {
    this->instruction = "";

    for (int i = this->program_counter; i < this->program_counter + 4; ++i) {
        if (i > this->data.size()) {
            throw std::runtime_error("Program counter to fetch index in InstructionMemory out of bounds");
        }

        this->instruction += this->data.at(i);
    }
}

void InstructionMemory::readInstructionMemoryFile() {
    if (this->is_instruction_file_read) {
        throw std::runtime_error("InstructionMemory file being read twice");
    }

    std::ifstream instruction_memory_file (this->instruction_memory_file_path);
    std::string byte_instruction;

    while (std::getline(instruction_memory_file, byte_instruction)) {
        this->data.push_back(byte_instruction);
    }

    this->is_instruction_file_read = true;
}

void InstructionMemory::loadInstructionIntoIFIDStageRegisters() {
    this->if_id_stage_registers->setInput(this->instruction);
}

void InstructionMemory::notifyConditionVariable() {
    this->getConditionVariable().notify_one();
}