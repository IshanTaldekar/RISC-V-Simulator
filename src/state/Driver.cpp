#include "../../include/state/Driver.h"

Driver::Driver() {
    this->program_counter = -1;
    this->is_new_program_counter_set = false;

    this->instruction_memory = InstructionMemory::init();
    this->if_id_stage_registers = IFIDStageRegisters::init();
    this->if_adder = IFAdder::init();
    this->logger = IFLogger::init();
}

void Driver::setProgramCounter(int value) {
    this->logger->log("[Driver] setProgramCounter waiting to acquire lock.");

    std::unique_lock<std::mutex> driver_lock (this->getModuleMutex());

    this->program_counter = value;
    this->is_new_program_counter_set = true;

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
                [this] { return this->is_new_program_counter_set; }
        );

        this->logger->log("[Driver] run woken up and acquired lock.");

        this->passProgramCounterToAdder();
        this->passProgramCounterToInstructionMemory();

        this->is_new_program_counter_set = false;

        this->logger->log("[Driver] program counter passed to adder and instruction_bits data.");


        this->logger->log("[Driver] run waiting at barrier.");

        // TODO: Add barrier
        this->passProgramCounterToIFIDStageRegisters();
    }
}

void Driver::passProgramCounterToInstructionMemory() {
    this->logger->log("[Driver] Passing program counter to instruction_bits data.");

    this->instruction_memory->setProgramCounter(this->program_counter);
    this->instruction_memory->notifyModuleConditionVariable();


    this->logger->log("[Driver] program counter passed to instruction_bits data and instruction_bits data notified.");
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