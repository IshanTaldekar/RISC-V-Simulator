#include "../../../include/combinational/adder/EXAdder.h"

EXAdder::EXAdder() {
    this->program_counter = -1;
    this->immediate = -1;

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
}

void EXAdder::notifyModuleConditionVariable() {
    this->getModuleConditionVariable().notify_one();
}

void EXAdder::passBranchAddressToEXMEMStageRegisters() {

}