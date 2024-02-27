#include "../../../include/combinational/mux/EXMuxALUInput2.h"

EXMuxALUInput2 *EXMuxALUInput2::current_instance = nullptr;

EXMuxALUInput2::EXMuxALUInput2() {
    this->immediate = -1;
    this->read_data_2 = -1;

    this->is_alu_src_asserted = false;

    this->is_immediate_set = false;
    this->is_read_data_2_set = false;

    this->alu_input_2_forwarding_mux = ALUInput2ForwardingMux::init();
}

EXMuxALUInput2 *EXMuxALUInput2::init() {
    if (EXMuxALUInput2::current_instance == nullptr) {
        EXMuxALUInput2::current_instance = new EXMuxALUInput2();
    }

    return EXMuxALUInput2::current_instance;
}

void EXMuxALUInput2::run() {
    while (this->isAlive()) {
        std::unique_lock<std::mutex> ex_mux_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                ex_mux_lock,
                [this] {
                    return this->is_immediate_set && this->is_read_data_2_set && this->is_control_signal_set
                        && this->is_pass_four_flag_set;
                }
        );

        if (this->isKilled()) {
            break;
        }

        this->passOutput();

        this->is_read_data_2_set = false;
        this->is_immediate_set = false;
        this->is_control_signal_set = false;
        this->is_pass_four_flag_set = false;
    }
}

void EXMuxALUInput2::notifyModuleConditionVariable() {
    this->getModuleConditionVariable().notify_one();
}

void EXMuxALUInput2::setInput(MuxInputType type, unsigned long value) {
    if (!std::holds_alternative<EXStageMuxALUInput2InputType>(type)) {
        std::cerr << "Incorrect MuxInputType passed to EXMuxALUInput2" << std::endl;
    }

    std::lock_guard<std::mutex> ex_mux_lock (this->getModuleMutex());

    if (std::get<EXStageMuxALUInput2InputType>(type) == EXStageMuxALUInput2InputType::ReadData2) {
        this->read_data_2 = value;
        this->is_read_data_2_set = true;
    } else if (std::get<EXStageMuxALUInput2InputType>(type) == EXStageMuxALUInput2InputType::ImmediateValue) {
        this->immediate = value;
        this->is_immediate_set = true;
    }

    this->notifyModuleConditionVariable();
}

void EXMuxALUInput2::assertControlSignal(bool is_asserted) {
    std::lock_guard<std::mutex> ex_mux_lock (this->getModuleMutex());

    this->is_alu_src_asserted = is_asserted;
    this->is_control_signal_set = true;

    this->notifyModuleConditionVariable();
}

void EXMuxALUInput2::passOutput() {
    if (this->is_alu_src_asserted) {
        this->alu_input_2_forwarding_mux->setInput(ALUInputMuxInputTypes::IDEXStageRegisters, this->immediate);
    } else {
        this->alu_input_2_forwarding_mux->setInput(ALUInputMuxInputTypes::IDEXStageRegisters, this->read_data_2);
    }
}

void EXMuxALUInput2::assertJALCustomControlSignal(bool is_asserted) {
    std::lock_guard<std::mutex> ex_mux_lock (this->getModuleMutex());

    this->is_pass_four_flag_asserted = is_asserted;
    this->is_pass_four_flag_set = true;

    this->notifyModuleConditionVariable();

}