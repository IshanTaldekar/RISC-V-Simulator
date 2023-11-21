#include "../../../include/combinational/mux/EXMux.h"

EXMux *EXMux::current_instance = nullptr;

EXMux::EXMux() {
    this->immediate = -1;
    this->read_data_2 = -1;

    this->is_alu_src_asserted = false;

    this->alu = ALU::init();

    this->is_immediate_set = false;
    this->is_read_data_2_set = false;
}

EXMux *EXMux::init() {
    if (EXMux::current_instance == nullptr) {
        EXMux::current_instance = new EXMux();
    }

    return EXMux::current_instance;
}

void EXMux::run() {
    while (this->isAlive()) {
        std::unique_lock<std::mutex> ex_mux_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                ex_mux_lock,
                [this] {
                    return this->is_immediate_set && this->is_read_data_2_set && this->is_control_signal_set;
                }
        );

        if (this->isKilled()) {
            break;
        }

        this->passOutput();

        this->is_read_data_2_set = false;
        this->is_immediate_set = false;
        this->is_control_signal_set = false;
    }
}

void EXMux::notifyModuleConditionVariable() {
    this->getModuleConditionVariable().notify_one();
}

void EXMux::setInput(StageMuxInputType type, unsigned long value) {
    if (!std::holds_alternative<EXStageMuxInputType>(type)) {
        std::cerr << "Incorrect StageMuxInputType passed to EXMux" << std::endl;
    }

    std::lock_guard<std::mutex> ex_mux_lock (this->getModuleMutex());

    if (std::get<EXStageMuxInputType>(type) == EXStageMuxInputType::ReadData2) {
        this->read_data_2 = value;
        this->is_read_data_2_set = true;
    } else if (std::get<EXStageMuxInputType>(type) == EXStageMuxInputType::ImmediateValue) {
        this->immediate = value;
        this->is_immediate_set = true;
    }

    this->notifyModuleConditionVariable();
}

void EXMux::assertControlSignal(bool is_asserted) {
    std::lock_guard<std::mutex> ex_mux_lock (this->getModuleMutex());

    this->is_alu_src_asserted = is_asserted;
    this->is_control_signal_set = true;

    this->notifyModuleConditionVariable();
}

void EXMux::passOutput() {
    if (this->is_alu_src_asserted) {
        this->alu->setInput2(this->immediate);
    } else {
        this->alu->setInput2(this->read_data_2);
    }
}