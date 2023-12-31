#include "../../../include/combinational/adder/EXAdder.h"

EXAdder *EXAdder::current_instance = nullptr;

EXAdder::EXAdder() {
    this->program_counter = 0UL;
    this->immediate = 0UL;
    this->result = 0UL;

    this->ex_mem_stage_registers = EXMEMStageRegisters::init();

    this->is_program_counter_set = false;
    this->is_immediate_set = false;
}

EXAdder *EXAdder::init() {
    if (EXAdder::current_instance == nullptr) {
        EXAdder::current_instance = new EXAdder();
    }

    return EXAdder::current_instance;
}

void EXAdder::run() {
    while (this->isAlive()) {
        std::unique_lock<std::mutex> ex_adder_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                ex_adder_lock,
                [this] { return this->is_program_counter_set && this->is_immediate_set; }
        );

        if (this->isKilled()) {
            break;
        }

        this->computeResult();
        this->passBranchAddressToEXMEMStageRegisters();

        this->is_immediate_set = false;
        this->is_program_counter_set = false;
    }
}

void EXAdder::setInput(AdderInputType type, unsigned long value) {
    if (!std::holds_alternative<EXAdderInputType>(type)) {
        std::cerr << "Incorrect AdderInputType passed to EXAdder" << std::endl;
    }

    std::lock_guard<std::mutex> ex_adder_lock (this->getModuleMutex());

    if (std::get<EXAdderInputType>(type) == EXAdderInputType::PCValue) {
        this->program_counter = value;
        this->is_program_counter_set = true;
    } else if (std::get<EXAdderInputType>(type) == EXAdderInputType::ImmediateValue) {
        this->immediate = value;
        this->is_immediate_set = true;
    }

    this->notifyModuleConditionVariable();
}

void EXAdder::computeResult() {
    this->result = this->program_counter + this->immediate;
}

void EXAdder::notifyModuleConditionVariable() {
    this->getModuleConditionVariable().notify_one();
}

void EXAdder::passBranchAddressToEXMEMStageRegisters() {
    this->ex_mem_stage_registers->setBranchedProgramCounter(this->result);
}