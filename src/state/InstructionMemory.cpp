#include "../../include/state/InstructionMemory.h"

InstructionMemory *InstructionMemory::current_instance = nullptr;

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
    std::unique_lock<std::mutex> instruction_memory_lock(this->getModuleMutex());
    this->getModuleConditionVariable().wait(
            instruction_memory_lock,
            [this] { return !this->instruction_memory_file_path.empty(); }
    );

    while (this->isAlive()) {
        this->logger->log("[InstructionMemory] Waiting to be woken up and acquire lock.");
        this->getModuleConditionVariable().wait(
                instruction_memory_lock,
                [this] { return this->is_instruction_file_read && this->is_new_program_counter_set; }
        );

        if (this->isKilled()) {
            break;
        }

        this->logger->log("[InstructionMemory] Woken up and acquired lock.");

        this->fetchInstructionFromMemory();

        this->logger->log("[InstructionMemory] Passing instruction to IFIDStageRegisters.");
        std::thread pass_instruction_thread (&InstructionMemory::passInstructionIntoIFIDStageRegisters, this);
        this->logger->log("[InstructionMemory] Successfully passed to IFIDStageRegisters.");

        this->is_new_program_counter_set = false;
    }
}

void InstructionMemory::loadInstructionMemoryFromFilePath(const std::string &file_path) {
    if (this->is_instruction_file_read) {
        throw std::runtime_error("instruction_memory_file_path being reset in InstructionMemory");
    }

    this->logger->log("[InstructionMemory] Waiting to set file path.");

    std::lock_guard<std::mutex> instruction_memory_lock(this->getModuleMutex());

    this->instruction_memory_file_path = file_path;
    this->readInstructionMemoryFile();

    this->logger->log("[InstructionMemory] File path set.");
}

void InstructionMemory::setProgramCounter(unsigned long value) {
    this->logger->log("[InstructionMemory] Waiting to set program counter.");

    std::lock_guard<std::mutex> instruction_memory_lock(this->getModuleMutex());

    this->program_counter = value;
    this->is_new_program_counter_set = true;

    this->notifyModuleConditionVariable();

    this->logger->log("[InstructionMemory] Program counter set.");
}

void InstructionMemory::fetchInstructionFromMemory() {
    this->logger->log("[InstructionMemory] Fetching instruction from data_memory.");

    this->instruction = "";

    // TODO: Determine big/little endian
    for (unsigned long i = this->program_counter; i < this->program_counter + 4; ++i) {
        if (i > this->data.size()) {
            throw std::runtime_error("Program counter to fetch index in InstructionMemory out of bounds");
        }

        this->instruction += this->data.at(i);
    }

    this->logger->log("[InstructionMemory] Instruction fetched from data_memory.");
}

void InstructionMemory::readInstructionMemoryFile() {
    this->logger->log("[InstructionMemory] Reading instruction file.");

    std::ifstream instruction_memory_file (this->instruction_memory_file_path);
    std::string byte_instruction;

    this->data = std::vector<std::string> {};

    while (std::getline(instruction_memory_file, byte_instruction)) {
        this->data.push_back(byte_instruction);
    }

    this->is_instruction_file_read = true;

    this->logger->log("[InstructionMemory] Instruction file read.");
}

void InstructionMemory::passInstructionIntoIFIDStageRegisters() {
    std::lock_guard<std::mutex> instruction_memory_lock (this->getModuleMutex());
    this->logger->log("[InstructionMemory] Passing instruction to IFIDStageRegisters.");

    this->if_id_stage_registers->setInput(this->instruction);
}

void InstructionMemory::notifyModuleConditionVariable() {
    this->getModuleConditionVariable().notify_one();
}