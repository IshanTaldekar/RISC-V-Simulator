#include "../../../include/combinational/mux/EXMuxALUInput1.h"

EXMuxALUInput1 *EXMuxALUInput1::current_instance = nullptr;
std::mutex EXMuxALUInput1::initialization_mutex;

EXMuxALUInput1::EXMuxALUInput1() {
    this->program_counter = std::bitset<WORD_BIT_COUNT>(std::string(32, '0'));
    this->read_data_1 = std::bitset<WORD_BIT_COUNT>(std::string(32, '0'));

    this->is_program_counter_set = false;
    this->is_read_data_1_set = false;

    this->is_reset_flag_set = false;

    this->is_pass_program_counter_flag_asserted = false;
    this->is_pass_program_counter_flag_set = false;

    this->alu_input_1_forwarding_mux = nullptr;
    this->logger = nullptr;
}

EXMuxALUInput1 *EXMuxALUInput1::init() {
    std::lock_guard<std::mutex> ex_mux_alu_input_1 (EXMuxALUInput1::initialization_mutex);

    if (EXMuxALUInput1::current_instance == nullptr) {
        EXMuxALUInput1::current_instance = new EXMuxALUInput1();
    }

    return EXMuxALUInput1::current_instance;
}

void EXMuxALUInput1::initDependencies() {
    this->alu_input_1_forwarding_mux = ALUInput1ForwardingMux::init();
    this->logger = Logger::init();
}

void EXMuxALUInput1::run() {
    this->initDependencies();

    while (this->isAlive()) {
        this->logger->log(Stage::EX, "[EXMuxALUInput1] Waiting to be woken up and acquire lock.");

        std::unique_lock<std::mutex> ex_mux_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                ex_mux_lock,
                [this] {
                    return (this->is_program_counter_set && this->is_read_data_1_set &&
                        this->is_pass_program_counter_flag_set) || this->is_reset_flag_set || this->isKilled();
                }
        );

        if  (this->isKilled()) {
            this->logger->log(Stage::EX, "[EXMuxALUInput1] Killed.");
            break;
        }

        if (this->is_reset_flag_set) {
            this->logger->log(Stage::EX, "[EXMuxALUInput1] Resetting stage.");

            this->resetState();
            this->is_reset_flag_set = false;

            this->logger->log(Stage::EX, "[EXMuxALUInput1] Reset.");
            continue;
        }

        this->logger->log(Stage::EX, "[EXMuxALUInput1] Woken up and acquired lock.");

        this->passOutput();

        this->is_program_counter_set = false;
        this->is_read_data_1_set = false;
        this->is_pass_program_counter_flag_asserted = false;
    }
}

void EXMuxALUInput1::setInput(MuxInputType type, MuxInputDataType value) {
    if (!std::holds_alternative<EXStageMuxALUInput1InputType>(type)) {
        throw std::runtime_error("EXMuxALUInput1::setInput: incompatible data types passed.");
    }

    this->logger->log(Stage::EX, "[EXMuxALUInput1] setInput waiting to be woken up and acquire lock.");

    std::lock_guard<std::mutex> ex_mux_lock (this->getModuleMutex());

    this->logger->log(Stage::EX, "[EXMuxALUInput1] setInput Woken up and acquired lock. Updating value.");

    if (std::get<EXStageMuxALUInput1InputType>(type) == EXStageMuxALUInput1InputType::ProgramCounter) {
        this->program_counter = std::bitset<WORD_BIT_COUNT>(std::get<unsigned long>(value));
        this->is_program_counter_set = true;
    } else if (std::get<EXStageMuxALUInput1InputType>(type) == EXStageMuxALUInput1InputType::ReadData1) {
        this->read_data_1 = std::bitset<WORD_BIT_COUNT>(std::get<std::bitset<WORD_BIT_COUNT>>(value));
        this->is_read_data_1_set = true;
    }

    this->logger->log(Stage::EX, "[EXMuxALUInput1] setInput value updated.");
    this->notifyModuleConditionVariable();
}

void EXMuxALUInput1::assertControlSignal(bool is_asserted) {
    this->is_control_signal_set = true;
}

void EXMuxALUInput1::passOutput() {
    this->logger->log(Stage::EX, "[EXMuxALUInput1] Passing value to ALU input 1.");

    if (this->is_pass_program_counter_flag_asserted) {
        this->alu_input_1_forwarding_mux->setInput(ALUInputMuxInputTypes::IDEXStageRegisters, this->program_counter);
    } else {
        this->alu_input_1_forwarding_mux->setInput(ALUInputMuxInputTypes::IDEXStageRegisters, this->read_data_1);
    }

    this->logger->log(Stage::EX, "[EXMuxALUInput1] Passed value to ALU input 1.");
}

void EXMuxALUInput1::assertJALCustomControlSignal(bool is_asserted) {
    this->logger->log(Stage::EX, "[EXMuxALUInput1] assertJALCustomControlSignal waiting to be woken"
                                 " up and acquire lock.");

    std::lock_guard<std::mutex> ex_mux_lock (this->getModuleMutex());

    this->logger->log(Stage::EX, "[EXMuxALUInput1] assertJALCustomControlSignal woken up and "
                                 "acquired lock. Updating values.");

    this->is_pass_program_counter_flag_asserted = is_asserted;
    this->is_pass_program_counter_flag_set = true;

    this->logger->log(Stage::EX, "[EXMuxALUInput1] assertJALCustomControlSignal value updated.");
    this->notifyModuleConditionVariable();
}

void EXMuxALUInput1::reset() {
    std::lock_guard<std::mutex> ex_mux_alu_input_1_lock (this->getModuleMutex());

    this->is_reset_flag_set = true;
    this->notifyModuleConditionVariable();
}

void EXMuxALUInput1::resetState() {
    this->program_counter = std::bitset<WORD_BIT_COUNT>(std::string(32, '0'));
    this->read_data_1 = std::bitset<WORD_BIT_COUNT>(std::string(32, '0'));
    this->is_pass_program_counter_flag_asserted = false;
    this->is_program_counter_set = false;
    this->is_read_data_1_set = false;
    this->is_pass_program_counter_flag_set = false;
}