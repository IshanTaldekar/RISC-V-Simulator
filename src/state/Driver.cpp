#include "../../include/state/Driver.h"

Driver *Driver::current_instance = nullptr;

Driver::Driver() {
    this->program_counter = 0UL;

    this->is_new_program_counter_set = true;
    this->is_nop_asserted = false;
    this->is_reset_flag_set = false;
    this->is_pause_flag_set = false;

    this->instruction_memory = InstructionMemory::init();
    this->if_id_stage_registers = IFIDStageRegisters::init();
    this->if_adder = IFAdder::init();
    this->logger = Logger::init();
    this->stage_synchronizer = StageSynchronizer::init();
}

void Driver::changeStageAndReset(PipelineType new_stage) {
    {  // Limit lock guard scope to avoid deadlock
        std::lock_guard<std::mutex> driver_lock_guard (this->getModuleMutex());

        this->logger->log(Stage::IF, "[Driver] PipelineType change." );
        this->setPipelineType(new_stage);
    }

    this->reset();
}

void Driver::reset() {
    std::lock_guard<std::mutex> driver_lock_guard (this->getModuleMutex());

    this->is_reset_flag_set = true;
    this->notifyModuleConditionVariable();
}

void Driver::resetStage() {
    this->program_counter = 0UL;

    this->is_new_program_counter_set = true;
    this->is_nop_asserted = false;
}

void Driver::pause() {
    this->logger->log(Stage::IF, "[Driver] Paused.");

    this->is_pause_flag_set = true;
}

void Driver::resume() {
    this->logger->log(Stage::IF, "[Driver] Resumed.");

    this->is_pause_flag_set = false;
    this->notifyModuleConditionVariable();
}

void Driver::setProgramCounter(unsigned long value) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    this->logger->log(Stage::IF, "[Driver] setProgramCounter waiting to acquire lock.");

    std::unique_lock<std::mutex> driver_lock (this->getModuleMutex());

    this->logger->log(Stage::IF, "[Driver] setProgramCounter acquired lock. Updating value.");

    if (!this->is_nop_asserted) {
        this->program_counter = value;
        this->logger->log(Stage::IF, "[Driver] setProgramCounter updated value.");
    } else {
        this->is_nop_asserted = false;
        this->logger->log(Stage::IF, "[Driver] setProgramCounter update skipped. NOP asserted.");
    }

    this->is_new_program_counter_set = true;
    this->notifyModuleConditionVariable();
}

Driver *Driver::init() {
    if (Driver::current_instance == nullptr) {
        Driver::current_instance = new Driver();
    }

    return Driver::current_instance;
}

void Driver::run() {
    while (this->isAlive()) {
        this->logger->log(Stage::IF, "[Driver] Waiting to be woken up and acquire lock.");

        std::unique_lock<std::mutex> driver_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                driver_lock,
                [this] {
                    return (this->is_new_program_counter_set && !this->is_pause_flag_set) || this->is_reset_flag_set;
                }
        );

        if (this->isKilled()) {
            this->logger->log(Stage::IF, "[Driver] Killed.");
            break;
        }

        if (this->is_reset_flag_set) {
            this->logger->log(Stage::IF, "[Driver] Resetting stage.");

            this->resetStage();
            this->is_reset_flag_set = false;

            this->logger->log(Stage::IF, "[Driver] Reset.");
            continue;
        }

        this->logger->log(Stage::IF, "[Driver] Woken up and acquired lock.");

        this->passProgramCounterToIFAdder();
        this->passProgramCounterToInstructionMemory();

        std::thread pass_program_counter_thread (&Driver::passProgramCounterToIFIDStageRegisters, this);

        this->is_new_program_counter_set = false;

        this->stage_synchronizer->conditionalArriveSingleStage();
    }
}

void Driver::passProgramCounterToInstructionMemory() {
    this->logger->log(Stage::IF, "[Driver] Passing program counter to InstructionMemory.");

    this->instruction_memory->setProgramCounter(this->program_counter);
    this->instruction_memory->notifyModuleConditionVariable();

    this->logger->log(Stage::IF, "[Driver] Passed program counter to InstructionMemory.");
}

void Driver::passProgramCounterToIFAdder() {
    this->logger->log(Stage::IF, "[Driver] Passing program counter to IFAdder.");

    this->if_adder->setInput(IFAdderInputType::PCValue, this->program_counter);
    this->if_adder->notifyModuleConditionVariable();

    this->logger->log(Stage::IF, "[Driver] Passed program counter to IFAdder.");
}

void Driver::passProgramCounterToIFIDStageRegisters() {
    this->logger->log(Stage::IF, "[Driver] Passing program counter to IFIDStageRegisters.");

    this->if_id_stage_registers->setInput(this->program_counter);
    this->if_id_stage_registers->notifyModuleConditionVariable();

    this->logger->log(Stage::IF, "[Driver] Passed program counter to IFIDStageRegisters.");
}

void Driver::assertNop() {
    this->is_nop_asserted = true;
}