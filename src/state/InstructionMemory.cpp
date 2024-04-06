#include "../../include/state/InstructionMemory.h"

InstructionMemory *InstructionMemory::current_instance = nullptr;
std::mutex InstructionMemory::initialization_mutex;

InstructionMemory::InstructionMemory() {
    this->instruction_memory_file_path = "";
    this->is_instruction_file_read = false;

    this->program_counter = 0UL;
    this->is_new_program_counter_set = false;

    this->if_id_stage_registers = nullptr;
    this->driver = nullptr;
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
    std::lock_guard<std::mutex> instruction_memory_lock (this->getModuleDependencyMutex());

    if (this->if_id_stage_registers && this->logger && this->driver) {
        return;
    }

    this->if_id_stage_registers = IFIDStageRegisters::init();
    this->logger = Logger::init();
    this->driver = Driver::init();
}

void InstructionMemory::run() {
    this->initDependencies();

    std::unique_lock<std::mutex> instruction_memory_lock(this->getModuleMutex());
    this->getModuleConditionVariable().wait(
            instruction_memory_lock,
            [this] { return !this->instruction_memory_file_path.empty(); }
    );

    while (this->isAlive()) {
        this->log("Waiting to be woken up and acquire lock.");

        this->getModuleConditionVariable().wait(
                instruction_memory_lock,
                [this] {
                    return (this->is_new_program_counter_set && this->is_instruction_file_read) || this->isKilled();
                }
        );

        if (this->isKilled()) {
            this->log("Killed.");
            break;
        }

        this->log("Woken up and acquired lock.");

        this->fetchInstructionFromMemory();

        std::thread pass_nop_thread (
                &InstructionMemory::passNopToDriver,
                this,
                this->getPipelineType() == PipelineType::Five && this->program_counter >= this->data.size()
        );
        std::thread pass_instruction_thread (&InstructionMemory::passInstructionIntoIFIDStageRegisters, this);

        pass_instruction_thread.join();
        pass_nop_thread.join();

        this->is_new_program_counter_set = false;
    }
}

void InstructionMemory::setInstructionMemoryInputFilePath(const std::string &file_path) {
    this->log("setInstructionMemoryInputFilePath waiting to "
                                 "acquire lock.");

    std::lock_guard<std::mutex> instruction_memory_lock(this->getModuleMutex());

    this->instruction_memory_file_path = file_path;
    this->readInstructionMemoryFile();

    this->log("setInstructionMemoryInputFilePath has finished.");
    this->notifyModuleConditionVariable();
}

void InstructionMemory::setProgramCounter(unsigned long value) {
    this->log("setProgramCounter waiting to acquire lock.");

    std::lock_guard<std::mutex> instruction_memory_lock(this->getModuleMutex());

    this->log("setProgramCounter acquired lock. Updating value.");

    this->program_counter = value;
    this->is_new_program_counter_set = true;

    this->log("setProgramCounter value updated.");
    this->notifyModuleConditionVariable();
}

void InstructionMemory::fetchInstructionFromMemory() {
    this->log("Fetching instruction from data_memory.");
    this->instruction.clear();

    for (unsigned long i = this->program_counter; i < this->program_counter + 4; ++i) {
        try {
            this->instruction += this->data.at(i);
        } catch (const std::out_of_range &e) {
            this->instruction = std::string(32, '0');
        }
    }

    this->log("Instruction fetched from data_memory.");
}

void InstructionMemory::readInstructionMemoryFile() {
    this->log("Reading instruction file.");

    std::ifstream instruction_memory_file (this->instruction_memory_file_path);
    std::string byte_instruction;

    this->data = std::vector<std::string> {};

    while (std::getline(instruction_memory_file, byte_instruction)) {
        this->data.push_back(byte_instruction);
    }

    this->is_instruction_file_read = true;

    this->log("Instruction file read.");
}

void InstructionMemory::passInstructionIntoIFIDStageRegisters() {
    this->log("Passing instruction to IFIDStageRegisters.");

    this->if_id_stage_registers->setInput(this->instruction);
    this->log("Passed instruction to IFIDStageRegisters.");
}

void InstructionMemory::passNopToDriver(bool is_asserted) {
    this->driver->setNop(is_asserted);
}

std::string InstructionMemory::getModuleTag() {
    return "InstructionMemory";
}

Stage InstructionMemory::getModuleStage() {
    return Stage::IF;
}