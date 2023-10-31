#include "../../include/combinational/adder/IFAdder.h"

IFAdder::IFAdder() {
    program_counter = 0;
    program_counter_set = false;

    if_mux = IFMux::init();
}

IFAdder *IFAdder::init() {
    if (IFAdder::current_instance == nullptr) {
        IFAdder::current_instance = new IFAdder();
    }

    return IFAdder::current_instance;
}

void IFAdder::run() {
    while (this->isAlive()) {
        std::unique_lock<std::mutex> adder_lock(this->getMutex());
        this->getConditionVariable().wait(
                adder_lock,
                [this] { return this->program_counter_set; }
        );

        this->loadProgramCounterToIFMux();
        this->if_mux->notifyConditionVariable();

        this->program_counter_set = false;
    }
}

void IFAdder::setInput(AdderInputType type, int value) {
    if (!std::holds_alternative<IFAdderInputType>(type)) {
        throw std::runtime_error("AdderInputType passed in IFAdder not compatible with IFAdderInputTypes");
    }

    std::unique_lock<std::mutex> adder_lock(this->getMutex());

    if (std::get<IFAdderInputType>(type) == IFAdderInputType::PCValue) {
        this->program_counter = value;
        this->program_counter_set = true;
    }
}

void IFAdder::loadProgramCounterToIFMux() {
    this->if_mux->setInput(IFStageMuxInputType::IncrementedPc, this->program_counter);
}

void IFAdder::notifyConditionVariable() {
    this->getConditionVariable().notify_one();
}