#include "../../include/state/Driver.h"

Driver *Driver::current_instance = nullptr;
std::mutex Driver::initialization_mutex;

Driver::Driver() {
    this->program_counter = 0UL;

    this->is_new_program_counter_set = true;
    this->is_nop_asserted = false;
    this->is_reset_flag_set = false;
    this->is_pause_flag_set = false;
    this->is_nop_flag_set = true;

    this->current_nop_set_operations = 0;

    this->instruction_memory = nullptr;
    this->if_id_stage_registers = nullptr;
    this->if_adder = nullptr;
    this->logger = nullptr;
    this->stage_synchronizer = nullptr;
}

void Driver::changeStageAndReset(PipelineType new_stage) {
    {  // Limit lock guard scope to avoid deadlock
        std::lock_guard<std::mutex> driver_lock_guard (this->getModuleMutex());

        this->log("PipelineType change." );
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
    this->is_nop_flag_set = true;
}

void Driver::pause() {
    this->log("Paused.");

    this->is_pause_flag_set = true;
}

void Driver::resume() {
    this->log("Resumed.");

    this->is_pause_flag_set = false;
    this->notifyModuleConditionVariable();
}

void Driver::setProgramCounter(unsigned long value) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    if (this->getPipelineType() == PipelineType::Five) {
        this->delayUpdateUntilNopFlagSet();
    }

    this->log("setProgramCounter waiting to acquire lock.");

    std::unique_lock<std::mutex> driver_lock (this->getModuleMutex());

    this->log("setProgramCounter acquired lock. Updating value.");

    if (!this->is_nop_asserted) {
        this->program_counter = value;
        this->log("setProgramCounter updated value.");
    } else {
        this->log("setProgramCounter update skipped. NOP asserted.");
    }

    this->is_new_program_counter_set = true;
    this->notifyModuleConditionVariable();
}

Driver *Driver::init() {
    std::lock_guard<std::mutex> driver_lock (Driver::initialization_mutex);

    if (Driver::current_instance == nullptr) {
        Driver::current_instance = new Driver();
    }

    return Driver::current_instance;
}

void Driver::initDependencies() {
    std::unique_lock<std::mutex> driver_lock (this->getModuleMutex());

    if (this->instruction_memory && this->if_id_stage_registers && this->if_adder && this->logger &&
        this->stage_synchronizer) {
        return;
    }

    this->instruction_memory = InstructionMemory::init();
    this->if_id_stage_registers = IFIDStageRegisters::init();
    this->if_adder = IFAdder::init();
    this->logger = Logger::init();
    this->stage_synchronizer = StageSynchronizer::init();
}

void Driver::run() {
    this->initDependencies();

    while (this->isAlive()) {
        this->log("Waiting to be woken up and acquire lock.");

        std::unique_lock<std::mutex> driver_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                driver_lock,
                [this] {
                    return (this->is_new_program_counter_set && this->is_nop_flag_set &&
                        !this->is_pause_flag_set) || this->is_reset_flag_set || this->isKilled();
                }
        );

        if (this->isKilled()) {
            this->log("Killed.");
            break;
        }

        if (this->is_reset_flag_set) {
            this->log("Resetting stage.");

            this->resetStage();
            this->is_reset_flag_set = false;

            this->log("Reset.");
            continue;
        }

        this->log("Woken up and acquired lock.");

        std::thread pass_program_counter_to_if_adder(&Driver::passProgramCounterToIFAdder, this, this->program_counter);
        std::thread pass_program_counter_to_instruction_memory(&Driver::passProgramCounterToInstructionMemory, this, this->program_counter);
        std::thread pass_program_counter_thread (&Driver::passProgramCounterToIFIDStageRegisters, this, this->program_counter);
        std::thread pass_nop_thread (&Driver::passNopToIFIDStageRegisters, this, this->is_nop_asserted);

        pass_program_counter_to_if_adder.join();
        pass_program_counter_to_instruction_memory.join();
        pass_program_counter_thread.join();
        pass_nop_thread.join();

        this->is_new_program_counter_set = false;
        this->is_nop_asserted = false;
        this->is_nop_flag_set = false;

        this->current_nop_set_operations = 0;

        this->stage_synchronizer->conditionalArriveSingleStage();
    }
}

void Driver::passProgramCounterToInstructionMemory(unsigned long pc) {
    this->log("Passing program counter to InstructionMemory.");
    this->instruction_memory->setProgramCounter(pc);
    this->log("Passed program counter to InstructionMemory.");
}

void Driver::passProgramCounterToIFAdder(unsigned long pc) {
    this->log("Passing program counter to IFAdder.");
    this->if_adder->setInput(IFAdderInputType::PCValue, pc);
    this->log("Passed program counter to IFAdder.");
}

void Driver::passProgramCounterToIFIDStageRegisters(unsigned long pc) {
    this->log("Passing program counter to IFIDStageRegisters.");
    this->if_id_stage_registers->setInput(pc);
    this->log("Passed program counter to IFIDStageRegisters.");
}

void Driver::setNop(bool is_asserted) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    this->log("setPassedNop waiting to acquire lock.");

    std::lock_guard<std::mutex> driver_lock (this->getModuleMutex());

    this->log("setPassedNop acquired lock.");

    this->is_nop_asserted |= is_asserted;

    if (++this->current_nop_set_operations == REQUIRED_NOP_FLAG_SET_OPERATIONS) {
        this->is_nop_flag_set = true;
        this->getModuleConditionVariable().notify_all();
    }
}

void Driver::passNopToIFIDStageRegisters(bool is_asserted) {
    this->log("Passing NOP to IFIDStageRegisters.");
    this->if_id_stage_registers->setPassedNop(is_asserted);
    this->log("Passed NOP to IFIDStageRegisters.");
}

void Driver::delayUpdateUntilNopFlagSet() {
    std::unique_lock<std::mutex> driver_lock (this->getModuleMutex());
    this->getModuleConditionVariable().wait(
            driver_lock,
            [this] {
                return this->is_nop_flag_set;
            }
    );
}

void Driver::assertSystemEnabledNop() {
    this->is_nop_asserted = true;
}

std::string Driver::getModuleTag() {
    return "Driver";
}

Stage Driver::getModuleStage() {
    return Stage::IF;
}