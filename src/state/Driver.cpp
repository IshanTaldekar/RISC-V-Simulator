#include "../../include/state/Driver.h"

Driver *Driver::current_instance = nullptr;

Driver::Driver() {
    this->program_counter = 0UL;

    this->is_new_program_counter_set = false;
    this->is_nop_asserted = false;
    this->is_reset_flag_set = false;
    this->is_pause_flag_set = false;

    this->instruction_memory = InstructionMemory::init();
    this->if_id_stage_registers = IFIDStageRegisters::init();
    this->if_adder = IFAdder::init();
    this->logger = IFLogger::init();
    this->stage_synchronizer = StageSynchronizer::init();
}

void Driver::reset() {
    this->is_reset_flag_set = true;
    this->notifyModuleConditionVariable();
}

void Driver::resetStage() {
    this->program_counter = 0UL;

    this->program_counter = true;
    this->is_nop_asserted = false;
}

void Driver::pauseStage() {
    this->is_new_program_counter_set = false;
}

void Driver::pause() {
    this->is_pause_flag_set = true;
    this->notifyModuleConditionVariable();
}

void Driver::setProgramCounter(int value) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    this->logger->log("[Driver] setProgramCounter waiting to acquire lock.");

    std::unique_lock<std::mutex> driver_lock (this->getModuleMutex());

    this->program_counter = value;
    this->is_new_program_counter_set = true;

    this->notifyModuleConditionVariable();

    this->logger->log("[Driver] PC set.");
}

Driver *Driver::init() {
    if (Driver::current_instance == nullptr) {
        Driver::current_instance = new Driver();
    }

    return Driver::current_instance;
}

void Driver::run() {
    while (this->isAlive()) {
        this->logger->log("[Driver] run waiting to be woken up and acquire lock.");

        std::unique_lock<std::mutex> driver_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                driver_lock,
                [this] {
                    return this->is_new_program_counter_set || this->is_pause_flag_set || this->is_reset_flag_set;
                }
        );

        this->logger->log("[Driver] run woken up and acquired lock.");

        if (this->isKilled()) {
            break;
        }

        if (this->is_pause_flag_set) {
            this->pauseStage();
            this->is_pause_flag_set = false;

            continue;
        }

        if (this->is_reset_flag_set) {
            this->resetStage();
            this->is_reset_flag_set = false;

            continue;
        }

        this->passProgramCounterToAdder();
        this->passProgramCounterToInstructionMemory();

        std::thread pass_program_counter_thread (&Driver::passProgramCounterToIFIDStageRegisters, this);

        this->is_new_program_counter_set = false;
        this->is_nop_asserted = false;

        this->stage_synchronizer->conditionalArriveSingleStage();
    }
}

void Driver::passProgramCounterToInstructionMemory() {
    this->logger->log("[Driver] Passing program counter to instruction_bits data_memory.");

    this->instruction_memory->setProgramCounter(this->program_counter);
    this->instruction_memory->notifyModuleConditionVariable();


    this->logger->log("[Driver] program counter passed to instruction_bits data_memory and instruction_bits data_memory notified.");
}

void Driver::passProgramCounterToAdder() {
    this->logger->log("[Driver] passing program counter to adder.");

    this->if_adder->setInput(IFAdderInputType::PCValue, this->program_counter);
    this->if_adder->notifyModuleConditionVariable();

    this->logger->log("[Driver] program counter passed to adder and adder notified.");
}

void Driver::passProgramCounterToIFIDStageRegisters() {
    this->logger->log("[Driver] passing program counter to IF/ID stage registers.");

    this->if_id_stage_registers->setInput(this->program_counter);
    this->if_id_stage_registers->notifyModuleConditionVariable();

    this->logger->log("[Driver] program counter passed to IF/ID stage registers.");
}

void Driver::notifyModuleConditionVariable() {
    this->getModuleConditionVariable().notify_one();
}

void Driver::setNop() {
    this->is_nop_asserted = true;
}