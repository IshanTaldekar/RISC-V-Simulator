#include "../../../include/combinational/mux/EXMuxALUInput2.h"

EXMuxALUInput2 *EXMuxALUInput2::current_instance = nullptr;
std::mutex EXMuxALUInput2::initialization_mutex;

EXMuxALUInput2::EXMuxALUInput2() {
    this->immediate = std::bitset<WORD_BIT_COUNT>(std::string(32, '0'));
    this->read_data_2 = std::bitset<WORD_BIT_COUNT>(std::string(32, '0'));

    this->is_alu_src_asserted = false;
    this->is_pass_four_flag_asserted = false;

    this->is_immediate_set = false;
    this->is_read_data_2_set = false;
    this->is_pass_four_flag_set = false;

    this->is_reset_flag_set = false;
    this->is_control_signal_set = false;

    this->logger = nullptr;
    this->alu_input_2_forwarding_mux = nullptr;
}

EXMuxALUInput2 *EXMuxALUInput2::init() {
    std::lock_guard<std::mutex> ex_mux_alu_input_2 (EXMuxALUInput2::initialization_mutex);

    if (EXMuxALUInput2::current_instance == nullptr) {
        EXMuxALUInput2::current_instance = new EXMuxALUInput2();
    }

    return EXMuxALUInput2::current_instance;
}

void EXMuxALUInput2::initDependencies() {
    this->logger = Logger::init();
    this->alu_input_2_forwarding_mux = ALUInput2ForwardingMux::init();
}

void EXMuxALUInput2::run() {
    this->initDependencies();

    while (this->isAlive()) {
        this->logger->log(Stage::EX, "[EXMuxALUInput2] Waiting to be woken up and acquire lock.");

        std::unique_lock<std::mutex> ex_mux_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                ex_mux_lock,
                [this] {
                    return (this->is_immediate_set && this->is_read_data_2_set && this->is_control_signal_set
                        && this->is_pass_four_flag_set) || this->isKilled() || this->is_reset_flag_set;
                }
        );

        if (this->isKilled()) {
            this->logger->log(Stage::EX, "[EXMuxALUInput2] Killed.");
            break;
        }

        if (this->is_reset_flag_set) {
            this->logger->log(Stage::EX, "[EXMuxALUInput2] Resetting stage.");

            this->resetState();
            this->is_reset_flag_set = false;

            this->logger->log(Stage::EX, "[EXMuxALUInput2] Reset.");
            continue;
        }

        this->logger->log(Stage::EX, "[EXMuxALUInput2] Woken up and acquired lock.");

        this->passOutput();

        this->is_read_data_2_set = false;
        this->is_immediate_set = false;
        this->is_control_signal_set = false;
        this->is_pass_four_flag_set = false;
    }
}

void EXMuxALUInput2::setInput(MuxInputType type, MuxInputDataType value) {
    if (!std::holds_alternative<EXStageMuxALUInput2InputType>(type) ||
            !std::holds_alternative<std::bitset<WORD_BIT_COUNT>>(value)) {
        throw std::runtime_error("EXMuxALUInput2::setInput: incompatible data types passed.");
    }

    this->logger->log(Stage::EX, "[EXMuxALUInput2] setInput waiting to be woken up and acquire lock.");

    std::lock_guard<std::mutex> ex_mux_lock (this->getModuleMutex());

    this->logger->log(Stage::EX, "[EXMuxALUInput2] setInput Woken up and acquired lock. Updating value.");

    if (std::get<EXStageMuxALUInput2InputType>(type) == EXStageMuxALUInput2InputType::ReadData2) {
        this->read_data_2 = std::bitset<WORD_BIT_COUNT>(std::get<std::bitset<WORD_BIT_COUNT>>(value));
        this->is_read_data_2_set = true;
    } else if (std::get<EXStageMuxALUInput2InputType>(type) == EXStageMuxALUInput2InputType::ImmediateValue) {
        this->immediate = std::bitset<WORD_BIT_COUNT>(std::get<std::bitset<WORD_BIT_COUNT>>(value));
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

void EXMuxALUInput2::reset() {
    std::lock_guard<std::mutex> ex_mux_alu_input_2_lock (this->getModuleMutex());

    this->is_reset_flag_set = true;
    this->notifyModuleConditionVariable();
}

void EXMuxALUInput2::resetState() {
    this->is_pass_four_flag_asserted = false;
    this->is_alu_src_asserted = false;
    this->immediate = std::bitset<WORD_BIT_COUNT>(std::string(32, '0'));
    this->read_data_2 = std::bitset<WORD_BIT_COUNT>(std::string(32, '0'));

    this->is_immediate_set = false;
    this->is_read_data_2_set = false;
    this->is_pass_four_flag_set = false;
    this->is_control_signal_set = false;
}
