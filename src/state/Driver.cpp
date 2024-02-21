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
    this->logger = IFLogger::init();
    this->stage_synchronizer = StageSynchronizer::init();
}

void Driver::changeStageAndReset(Stage new_stage) {
    this->logger->log("[Driver] Stage change." );

    this->setStage(new_stage);
    this->reset();
}

void Driver::reset() {
    this->logger->log("[Driver] Reset flag set.");

    this->is_reset_flag_set = true;
    this->notifyModuleConditionVariable();
}

void Driver::resetStage() {
    this->logger->log("[Driver] Reset stage: program counter is 0.");

    this->program_counter = 0UL;

    this->is_new_program_counter_set = true;
    this->is_nop_asserted = false;
}

void Driver::pause() {
    this->logger->log("[Driver] Pausing execution.");

    this->is_pause_flag_set = true;
    this->notifyModuleConditionVariable();
}

void Driver::resume() {
    this->logger->log("[Driver] Resuming execution.");

    this->is_pause_flag_set = false;
    this->notifyModuleConditionVariable();
}

void Driver::setProgramCounter(unsigned long value) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    this->logger->log("[Driver] setProgramCounter waiting to acquire lock.");

    std::unique_lock<std::mutex> driver_lock (this->getModuleMutex());

    if (!this->is_nop_asserted) {
        this->program_counter = value;
    } else {
        this->is_nop_asserted = false;
    }

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
        this->logger->log("[Driver] Waiting to be woken up and acquire lock.");

        std::unique_lock<std::mutex> driver_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                driver_lock,
                [this] {
                    return this->is_new_program_counter_set || !this->is_pause_flag_set || this->is_reset_flag_set;
                }
        );

        if (this->isKilled()) {
            break;
        }

        if (this->is_reset_flag_set) {
            this->resetStage();
            this->is_reset_flag_set = false;

            continue;
        }

        if (this->is_pause_flag_set) {
            continue;
        }

        this->logger->log("[Driver] Woken up and acquired lock. Passing values to Adder and InstructionMemory.");

        this->passProgramCounterToAdder();
        this->passProgramCounterToInstructionMemory();

        this->logger->log("[Driver] Passing values to the IFStageRegister.");

        std::thread pass_program_counter_thread (&Driver::passProgramCounterToIFIDStageRegisters, this);

        this->is_new_program_counter_set = false;

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
    std::lock_guard<std::mutex> driver_lock (this->getModuleMutex());
    this->is_nop_asserted = true;
}