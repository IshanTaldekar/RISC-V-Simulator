#include "../../../include/combinational/mux/EXMuxALUInput2.h"

EXMuxALUInput2 *EXMuxALUInput2::current_instance = nullptr;

EXMuxALUInput2::EXMuxALUInput2() {
    this->immediate = 0UL;
    this->read_data_2 = 0UL;

    this->is_alu_src_asserted = false;
    this->is_pass_four_flag_asserted = false;

    this->is_immediate_set = false;
    this->is_read_data_2_set = false;
    this->is_pass_four_flag_set = false;

    this->logger = Logger::init();
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
        this->logger->log(Stage::EX, "[EXMuxALUInput2] Waiting to be woken up and acquire lock.");

        std::unique_lock<std::mutex> ex_mux_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                ex_mux_lock,
                [this] {
                    return (this->is_immediate_set && this->is_read_data_2_set && this->is_control_signal_set
                        && this->is_pass_four_flag_set) || this->isKilled();
                }
        );

        if (this->isKilled()) {
            this->logger->log(Stage::EX, "[EXMuxALUInput2] Killed.");
            break;
        }

        this->logger->log(Stage::EX, "[EXMuxALUInput2] Woken up and acquired lock.");

        this->passOutput();

        this->is_read_data_2_set = false;
        this->is_immediate_set = false;
        this->is_control_signal_set = false;
        this->is_pass_four_flag_set = false;
    }
}

void EXMuxALUInput2::setInput(MuxInputType type, unsigned long value) {
    if (!std::holds_alternative<EXStageMuxALUInput2InputType>(type)) {
        throw std::runtime_error("Incorrect MuxInputType passed to EXMuxALUInput2");
    }

    this->logger->log(Stage::EX, "[EXMuxALUInput2] setInput waiting to be woken up and acquire lock.");

    std::lock_guard<std::mutex> ex_mux_lock (this->getModuleMutex());

    this->logger->log(Stage::EX, "[EXMuxALUInput2] setInput Woken up and acquired lock. Updating value.");

    if (std::get<EXStageMuxALUInput2InputType>(type) == EXStageMuxALUInput2InputType::ReadData2) {
        this->read_data_2 = value;
        this->is_read_data_2_set = true;
    } else if (std::get<EXStageMuxALUInput2InputType>(type) == EXStageMuxALUInput2InputType::ImmediateValue) {
        this->immediate = value;
        this->is_immediate_set = true;
    }

    this->logger->log(Stage::EX, "[EXMuxALUInput2] setInput value updated.");
    this->notifyModuleConditionVariable();
}

void EXMuxALUInput2::assertControlSignal(bool is_asserted) {
    this->logger->log(Stage::EX, "[EXMuxALUInput2] assertControlSignal waiting to be woken up and acquire lock.");

    std::lock_guard<std::mutex> ex_mux_lock (this->getModuleMutex());

    this->logger->log(Stage::EX, "[EXMuxALUInput2] assertControlSignal woken up and acquired lock. Updating control signal.");

    this->is_alu_src_asserted = is_asserted;
    this->is_control_signal_set = true;

    this->logger->log(Stage::EX, "[EXMuxALUInput2] assertControlSignal updated control signal.");
    this->notifyModuleConditionVariable();
}

void EXMuxALUInput2::passOutput() {
    this->logger->log(Stage::EX, "[EXMuxALUInput2] Passing value to ALU input 2.");

    if (this->is_pass_four_flag_asserted) {
        this->alu_input_2_forwarding_mux->setInput(ALUInputMuxInputTypes::IDEXStageRegisters, 4);
    } else if (this->is_alu_src_asserted) {
        this->alu_input_2_forwarding_mux->setInput(ALUInputMuxInputTypes::IDEXStageRegisters, this->immediate);
    } else {
        this->alu_input_2_forwarding_mux->setInput(ALUInputMuxInputTypes::IDEXStageRegisters, this->read_data_2);
    }

    this->logger->log(Stage::EX, "[EXMuxALUInput2] Passed value to ALU input 2.");
}

void EXMuxALUInput2::assertJALCustomControlSignal(bool is_asserted) {
    this->logger->log(Stage::EX, "[EXMuxALUInput2] assertJALCustomControlSignal waiting to be woken "
                                 "up and acquire lock.");

    std::lock_guard<std::mutex> ex_mux_lock (this->getModuleMutex());

    this->logger->log(Stage::EX, "[EXMuxALUInput2] assertJALCustomControlSignal woken up and acquired"
                                 " lock. Updating values.");

    this->is_pass_four_flag_asserted = is_asserted;
    this->is_pass_four_flag_set = true;

    this->logger->log(Stage::EX, "[EXMuxALUInput2] assertJALCustomControlSignal updated control.");
    this->notifyModuleConditionVariable();
}