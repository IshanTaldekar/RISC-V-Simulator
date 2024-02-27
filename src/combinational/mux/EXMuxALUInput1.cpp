#include "../../../include/combinational/mux/EXMuxALUInput1.h"

EXMuxALUInput1 *EXMuxALUInput1::current_instance = nullptr;

EXMuxALUInput1::EXMuxALUInput1() {
    this->program_counter = 0UL;
    this->read_data_1 = 0UL;

    this->is_program_counter_set = false;
    this->is_read_data_1_set = false;

    this->is_pass_program_counter_flag_asserted = false;

    this->alu_input_1_forwarding_mux = ALUInput1ForwardingMux::init();
}

EXMuxALUInput1 *EXMuxALUInput1::init() {
    if (EXMuxALUInput1::current_instance == nullptr) {
        EXMuxALUInput1::current_instance = new EXMuxALUInput1();
    }

    return EXMuxALUInput1::current_instance;
}

void EXMuxALUInput1::run() {
    while (this->isAlive()) {
        std::unique_lock<std::mutex> ex_mux_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                ex_mux_lock,
                [this] {
                    return this->is_program_counter_set && this->is_read_data_1_set &&
                        this->is_pass_program_counter_flag_asserted;
                }
        );

        if  (this->isKilled()) {
            break;
        }

        this->passOutput();

        this->is_program_counter_set = false;
        this->is_read_data_1_set = false;
        this->is_pass_program_counter_flag_asserted = false;
    }
}

void EXMuxALUInput1::notifyModuleConditionVariable() {
    this->getModuleConditionVariable().notify_one();
}

void EXMuxALUInput1::setInput(MuxInputType type, unsigned long value) {
    if (!std::holds_alternative<EXStageMuxALUInput1InputType>(type)) {
        std::cerr << "Incorrect MuxInputType passed to EXMuxALUInput2" << std::endl;
    }

    std::lock_guard<std::mutex> ex_mux_lock (this->getModuleMutex());

    if (std::get<EXStageMuxALUInput1InputType>(type) == EXStageMuxALUInput1InputType::ProgramCounter) {
        this->program_counter = value;
        this->is_program_counter_set = true;
    } else if (std::get<EXStageMuxALUInput1InputType>(type) == EXStageMuxALUInput1InputType::ReadData1) {
        this->read_data_1 = value;
        this->is_read_data_1_set = true;
    }

    this->notifyModuleConditionVariable();
}

void EXMuxALUInput1::assertControlSignal(bool is_asserted) {
    std::lock_guard<std::mutex> ex_mux_lock (this->getModuleMutex());

    this->is_pass_program_counter_flag_asserted = is_asserted;
    this->is_control_signal_set = true;

    this->notifyModuleConditionVariable();
}

void EXMuxALUInput1::passOutput() {
    if (this->is_pass_program_counter_flag_asserted) {
        this->alu_input_1_forwarding_mux->setInput(ALUInputMuxInputTypes::IDEXStageRegisters, this->program_counter);
    } else {
        this->alu_input_1_forwarding_mux->setInput(ALUInputMuxInputTypes::IDEXStageRegisters, this->read_data_1);
    }
}