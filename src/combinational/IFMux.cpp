#include "../../include/combinational/mux/IFMux.h"

IFMux::IFMux() {
    this->driver = Driver::init();

    this->is_pc_src_signal_asserted = false;
    this->is_incremented_pc_set = false;
    this->is_branched_pc_set = false;

    this->incremented_pc = -1;
    this->branched_pc = -1;
}

IFMux *IFMux::init() {
    if (IFMux::current_instance == nullptr) {
        IFMux::current_instance = new IFMux();
    }

    return IFMux::current_instance;
}

void IFMux::run() {
    while (this->isAlive()) {
        std::unique_lock<std::mutex> mux_lock (this->getMutex());
        this->getConditionVariable().wait(
                mux_lock,
                [this] { return this->is_branched_pc_set && this->is_incremented_pc_set; }
        );

        this->loadOutput();

        this->is_pc_src_signal_asserted = false;
        this->is_branched_pc_set = false;
        this->is_incremented_pc_set = false;
    }
}

void IFMux::setInput(StageMuxInputType type, int value) {
    if (!std::holds_alternative<IFStageMuxInputType>(type)) {
        throw std::runtime_error("StageMuxInputType passed to IFMux not compatible with IFStageMuxInputTypes");
    }

    std::unique_lock<std::mutex> mux_lock (this->getMutex());

    if (std::get<IFStageMuxInputType>(type) == IFStageMuxInputType::IncrementedPc) {
        incremented_pc = value;
        is_incremented_pc_set = true;
    } else if (std::get<IFStageMuxInputType>(type) == IFStageMuxInputType::BranchedPc) {
        branched_pc = value;
        is_branched_pc_set = true;
    } else {
        std::cerr << "IFMux::setInput did not match any input type" << std::endl;
    }
}

void IFMux::assertControlSignal() {
    this->is_pc_src_signal_asserted = true;
}

void IFMux::loadOutput() {
    if (this->is_pc_src_signal_asserted) {
        this->driver->setProgramCounter(this->incremented_pc);
    } else {
        this->driver->setProgramCounter(this->branched_pc);
    }
}

void IFMux::notifyConditionVariable() {
    this->getConditionVariable().notify_one();
}