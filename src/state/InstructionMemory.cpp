#include "../../include/state/InstructionMemory.h"

InstructionMemory::InstructionMemory() {
    this->instruction_memory_file_path = "";
    this->is_instruction_file_read = false;

    this->if_id_stage_registers = IFIDStageRegisters::init();
    this->logger = IFLogger::init();

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
        this->logger->log("[InstructionMemory] run waiting to be woken up and acquire lock.");

        std::unique_lock<std::mutex> instruction_memory_lock(this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                instruction_memory_lock,
                [this] { return this->is_instruction_file_read && this->is_new_program_counter_set; }
        );

        this->logger->log("[InstructionMemory] run woken up and acquired lock.");

        this->fetchInstructionFromMemory();

        this->logger->log("[InstructionMemory] run passing instruction to IFIDStageRegisters.");
        this->passInstructionIntoIFIDStageRegisters();
        this->logger->log("[InstructionMemory] run successfully passed to IFIDStageRegisters.");

        this->is_new_program_counter_set = false;
    }
}

void InstructionMemory::setInstructionMemoryFilePath(const std::string &file_path) {
    if (!this->instruction_memory_file_path.empty()) {
        throw std::runtime_error("instruction_memory_file_path being reset in InstructionMemory");
    }

    this->logger->log("[InstructionMemory] waiting to set file path.");

    std::lock_guard<std::mutex> instruction_memory_lock(this->getModuleMutex());

    this->instruction_memory_file_path = file_path;
    this->readInstructionMemoryFile();

    this->logger->log("[InstructionMemory] file path set.");
}

void InstructionMemory::setProgramCounter(int value) {
    this->logger->log("[InstructionMemory] waiting to set program counter.");

    std::lock_guard<std::mutex> instruction_memory_lock(this->getModuleMutex());

    this->program_counter = value;
    this->is_new_program_counter_set = true;

    this->logger->log("[InstructionMemory] program counter set.");
}

void InstructionMemory::fetchInstructionFromMemory() {
    this->logger->log("[InstructionMemory] fetching instruction from data.");

    this->instruction = "";

    for (int i = this->program_counter; i < this->program_counter + 4; ++i) {
        if (i > this->data.size()) {
            throw std::runtime_error("Program counter to fetch index in InstructionMemory out of bounds");
        }

        this->instruction += this->data.at(i);
    }

    this->logger->log("[InstructionMemory] instruction fetched from data.");
}

void InstructionMemory::readInstructionMemoryFile() {
    if (this->is_instruction_file_read) {
        throw std::runtime_error("InstructionMemory file being read again");
    }

    this->logger->log("[InstructionMemory] waiting to read instruction file.");

    std::lock_guard<std::mutex> instruction_memory_lock (this->getModuleMutex());

    std::ifstream instruction_memory_file (this->instruction_memory_file_path);
    std::string byte_instruction;

    this->data = std::vector<std::string> {};

    while (std::getline(instruction_memory_file, byte_instruction)) {
        this->data.push_back(byte_instruction);
    }

    this->is_instruction_file_read = true;

    this->logger->log("[InstructionMemory] instruction file read.");
}

void InstructionMemory::passInstructionIntoIFIDStageRegisters() {
    this->if_id_stage_registers->setInput(this->instruction);
}

void InstructionMemory::notifyModuleConditionVariable() {
    this->getModuleConditionVariable().notify_one();
}