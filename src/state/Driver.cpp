#include "../../include/state/Driver.h"

Driver::Driver() {
    program_counter = -1;
    is_new_program_counter_set = false;

    instruction_memory = InstructionMemory::init();
    if_id_stage_registers = IFIDStageRegisters::init();
    if_adder = IFAdder::init();
}

void Driver::setProgramCounter(int value) {
    std::unique_lock<std::mutex> driver_lock (this->getMutex());

    this->program_counter = value;
    this->is_new_program_counter_set = true;
}

Driver *Driver::init() {
    if (Driver::current_instance == nullptr) {
        Driver::current_instance = new Driver();
    }

    return Driver::current_instance;
}

void Driver::run() {
    while (this->isAlive()) {
        std::unique_lock<std::mutex> driver_lock (this->getMutex());
        this->getConditionVariable().wait(
                driver_lock,
                [this] { return this->is_new_program_counter_set; }
        );

        this->passProgramCounterToAdder();
        this->passProgramCounterToInstructionMemory();

        this->is_new_program_counter_set = false;

        // TODO: Add barrier
        this->passProgramCounterToIFIDStageRegisters();
    }
}

void Driver::passProgramCounterToInstructionMemory() {
    this->instruction_memory->setProgramCounter(this->program_counter);
    this->instruction_memory->notifyConditionVariable();
}

void Driver::passProgramCounterToAdder() {
    this->if_adder->setInput(IFAdderInputType::PCValue, this->program_counter);
    this->if_adder->notifyConditionVariable();
}

void Driver::passProgramCounterToIFIDStageRegisters() {
    this->if_id_stage_registers->setInput(this->program_counter);
    this->if_id_stage_registers->notifyConditionVariable();
}

void Driver::notifyConditionVariable() {
    this->getConditionVariable().notify_one();
}