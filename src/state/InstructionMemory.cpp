#include "../../include/state/InstructionMemory.h"

InstructionMemory *InstructionMemory::current_instance = nullptr;
std::mutex InstructionMemory::initialization_mutex;

InstructionMemory::InstructionMemory() {
    this->instruction_memory_file_path = "";
    this->is_instruction_file_read = false;

    this->program_counter = 0UL;
    this->is_new_program_counter_set = false;

    this->if_id_stage_registers = nullptr;
    this->logger = nullptr;
}

InstructionMemory *InstructionMemory::init() {
    std::lock_guard<std::mutex> instruction_memory_lock (InstructionMemory::initialization_mutex);

    if (InstructionMemory::current_instance == nullptr) {
        InstructionMemory::current_instance = new InstructionMemory();
    }

    return InstructionMemory::current_instance;
}

void InstructionMemory::initDependencies() {
    this->if_id_stage_registers = IFIDStageRegisters::init();
    this->logger = Logger::init();
}

void InstructionMemory::run() {
    this->initDependencies();

    std::unique_lock<std::mutex> instruction_memory_lock(this->getModuleMutex());
    this->getModuleConditionVariable().wait(
            instruction_memory_lock,
            [this] { return !this->instruction_memory_file_path.empty(); }
    );

    while (this->isAlive()) {
        this->logger->log(Stage::IF, "[InstructionMemory] Waiting to be woken up and acquire lock.");

        this->getModuleConditionVariable().wait(
                instruction_memory_lock,
                [this] {
                    return (this->is_new_program_counter_set && this->is_instruction_file_read) || this->isKilled();
                }
        );

        if (this->isKilled()) {
            this->logger->log(Stage::IF, "[InstructionMemory] Killed.");
            break;
        }

        this->logger->log(Stage::IF, "[InstructionMemory] Woken up and acquired lock.");

        this->fetchInstructionFromMemory();

        std::thread pass_instruction_thread (&InstructionMemory::passInstructionIntoIFIDStageRegisters, this);
        pass_instruction_thread.join();

        this->is_new_program_counter_set = false;
    }
}

void InstructionMemory::setInstructionMemoryInputFilePath(const std::string &file_path) {
    if (!this->logger) {
        this->initDependencies();
    }

    this->logger->log(Stage::IF, "[InstructionMemory] setInstructionMemoryInputFilePath waiting to "
                                 "acquire lock.");

    std::lock_guard<std::mutex> instruction_memory_lock(this->getModuleMutex());

    this->instruction_memory_file_path = file_path;
    this->readInstructionMemoryFile();

    this->logger->log(Stage::IF, "[InstructionMemory] setInstructionMemoryInputFilePath has finished.");
}

void InstructionMemory::setProgramCounter(unsigned long value) {
    this->logger->log(Stage::IF, "[InstructionMemory] setProgramCounter waiting to acquire lock.");

    std::lock_guard<std::mutex> instruction_memory_lock(this->getModuleMutex());

    this->logger->log(Stage::IF, "[InstructionMemory] setProgramCounter acquired lock. Updating value.");

    this->program_counter = value;
    this->is_new_program_counter_set = true;

    this->logger->log(Stage::IF, "[InstructionMemory] setProgramCounter value updated.");
    this->notifyModuleConditionVariable();
}

void InstructionMemory::fetchInstructionFromMemory() {
    this->logger->log(Stage::IF, "[InstructionMemory] Fetching instruction from data_memory.");

    // TODO: Determine big/little endian
    for (unsigned long i = this->program_counter; i < this->program_counter + 4; ++i) {
        try {
            this->instruction += this->data.at(i);
        } catch (const std::out_of_range &e) {
            std::cerr << "Error: " << e.what() << std::endl;
            throw std::runtime_error("Program counter to fetch index in InstructionMemory out of bounds");
        }
    }

    this->logger->log(Stage::IF, "[InstructionMemory] Instruction fetched from data_memory.");
}

void InstructionMemory::readInstructionMemoryFile() {
    this->logger->log(Stage::IF, "[InstructionMemory] Reading instruction file.");

    std::ifstream instruction_memory_file (this->instruction_memory_file_path);
    std::string byte_instruction;

    this->data = std::vector<std::string> {};

    while (std::getline(instruction_memory_file, byte_instruction)) {
        this->data.push_back(byte_instruction);
    }

    this->is_instruction_file_read = true;

    this->logger->log(Stage::IF, "[InstructionMemory] Instruction file read.");
}

void InstructionMemory::passInstructionIntoIFIDStageRegisters() {
    this->logger->log(Stage::IF, "[InstructionMemory] Passing instruction to IFIDStageRegisters.");

    this->if_id_stage_registers->setInput(this->instruction);
    this->logger->log(Stage::IF, "[InstructionMemory] Passed instruction to IFIDStageRegisters.");
}
