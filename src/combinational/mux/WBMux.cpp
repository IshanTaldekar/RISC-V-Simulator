#include "../../../include/combinational/mux/WBMux.h"

WBMux *WBMux::current_instance = nullptr;
std::mutex WBMux::initialization_mutex;

WBMux::WBMux() {
    this->read_data = std::bitset<WORD_BIT_COUNT>(std::string(32, '0'));
    this->alu_result = std::bitset<WORD_BIT_COUNT>(std::string(32, '0'));

    this->is_mem_to_reg_asserted = false;

    this->is_read_data_set = false;
    this->is_alu_result_set = false;

    this->register_file = nullptr;
    this->alu_input_1_forwarding_mux = nullptr;
    this->alu_input_2_forwarding_mux = nullptr;
    this->logger = nullptr;
}

WBMux *WBMux::init() {
    std::lock_guard<std::mutex> wb_mux_lock (WBMux::initialization_mutex);

    if (WBMux::current_instance == nullptr) {
        WBMux::current_instance = new WBMux();
    }

    return WBMux::current_instance;
}

void WBMux::initDependencies() {
    this->register_file = RegisterFile::init();
    this->alu_input_1_forwarding_mux = ALUInput1ForwardingMux::init();
    this->alu_input_2_forwarding_mux = ALUInput2ForwardingMux::init();
    this->logger = Logger::init();
}

void WBMux::run() {
    this->initDependencies();

    while (this->isAlive()) {
        this->logger->log(Stage::WB, "[WBMux] Waiting to be woken up and acquire lock.");

        std::unique_lock<std::mutex> wb_mux_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                wb_mux_lock,
                [this] {
                    return (this->is_read_data_set && this->is_alu_result_set && this->is_control_signal_set) ||
                        this->isKilled();
                }
        );

        if (this->isKilled()) {
            this->logger->log(Stage::WB, "[WBMux] Killed.");
            break;
        }

        this->logger->log(Stage::WB, "[WBMux] Woken up and acquired lock.");

        std::thread pass_output_to_forwarding_unit (&WBMux::passOutputToForwardingMuxes, this);
        this->passOutput();

        pass_output_to_forwarding_unit.join();

        this->is_read_data_set = false;
        this->is_alu_result_set = false;
        this->is_control_signal_set = false;
    }
}

void WBMux::setInput(MuxInputType type, MuxInputDataType value) {
    if (!std::holds_alternative<WBStageMuxInputType>(type)) {
        throw std::runtime_error("Incorrect MuxInputType passed to EXMuxALUInput2");
    }

    this->logger->log(Stage::WB, "[WBMux] setInput waiting to acquire lock.");

    std::lock_guard<std::mutex> ex_mux_lock (this->getModuleMutex());

    if (std::get<WBStageMuxInputType>(type) == WBStageMuxInputType::ALUResult) {
        this->alu_result = std::bitset<WORD_BIT_COUNT>(std::get<std::bitset<WORD_BIT_COUNT>>(value));
        this->is_alu_result_set = true;
    } else if (std::get<WBStageMuxInputType>(type) == WBStageMuxInputType::ReadData) {
        this->read_data = std::bitset<WORD_BIT_COUNT>(std::get<std::bitset<WORD_BIT_COUNT>>(value));
        this->is_read_data_set = true;
    }

    this->logger->log(Stage::WB, "[WBMux] setInput value updated.");
    this->notifyModuleConditionVariable();
}

void WBMux::assertControlSignal(bool is_asserted) {
    if (!this->logger) {
        this->initDependencies();
    }

    this->logger->log(Stage::WB, "[WBMux] assertControlSignal waiting to acquire lock.");

    std::lock_guard<std::mutex> wb_mux_lock (this->getModuleMutex());

    this->is_mem_to_reg_asserted = is_asserted;
    this->is_control_signal_set = true;

    this->logger->log(Stage::WB, "[WBMux] assertControlSignal updated control.");
    this->notifyModuleConditionVariable();
}

void WBMux::passOutput() {
    this->logger->log(Stage::WB, "[WBMux] Passing output to register file.");

    if (this->is_mem_to_reg_asserted) {
        this->register_file->setWriteData(this->read_data);
    } else {
        this->register_file->setWriteData(this->alu_result);
    }

    this->logger->log(Stage::WB, "[WBMux] Passed output to register file.");
}

void WBMux::passOutputToForwardingMuxes() {
    this->logger->log(Stage::WB, "[WBMux] Passing values to forwarding muxes.");

    if (this->is_mem_to_reg_asserted) {
        this->alu_input_1_forwarding_mux->setInput(ALUInputMuxInputTypes::MEMWBStageRegisters, this->read_data);
        this->alu_input_2_forwarding_mux->setInput(ALUInputMuxInputTypes::MEMWBStageRegisters, this->read_data);
    } else {
        this->alu_input_1_forwarding_mux->setInput(ALUInputMuxInputTypes::MEMWBStageRegisters, this->alu_result);
        this->alu_input_2_forwarding_mux->setInput(ALUInputMuxInputTypes::MEMWBStageRegisters, this->alu_result);
    }

    this->logger->log(Stage::WB, "[WBMux] Passed values to forwarding muxes.");
}