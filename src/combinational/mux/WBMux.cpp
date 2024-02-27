#include "../../../include/combinational/mux/WBMux.h"

WBMux *WBMux::current_instance = nullptr;

WBMux::WBMux() {
    this->read_data = 0UL;
    this->alu_result = 0UL;

    this->is_mem_to_reg_asserted = false;

    this->is_read_data_set = false;
    this->is_alu_result_set = false;

    this->register_file = RegisterFile::init();
    this->alu_input_1_forwarding_mux = ALUInput1ForwardingMux::init();
    this->alu_input_2_forwarding_mux = ALUInput2ForwardingMux::init();

    this->logger = WBLogger::init();
}

WBMux *WBMux::init() {
    if (WBMux::current_instance == nullptr) {
        WBMux::current_instance = new WBMux();
    }

    return WBMux::current_instance;
}

void WBMux::run() {
    while (this->isAlive()) {
        std::unique_lock<std::mutex> wb_mux_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                wb_mux_lock,
                [this] {
                    return this->is_read_data_set && this->is_alu_result_set && this->is_control_signal_set;
                }
        );

        if (this->isKilled()) {
            break;
        }

        this->passOutput();
        this->passOutputToForwardingUnit();

        this->is_read_data_set = false;
        this->is_alu_result_set = false;
        this->is_control_signal_set = false;
    }
}

void WBMux::notifyModuleConditionVariable() {
    this->getModuleConditionVariable().notify_one();
}

void WBMux::setInput(MuxInputType type, unsigned long value) {
    if (!std::holds_alternative<WBStageMuxInputType>(type)) {
        std::cerr << "Incorrect MuxInputType passed to EXMuxALUInput2" << std::endl;
    }

    std::lock_guard<std::mutex> ex_mux_lock (this->getModuleMutex());

    if (std::get<WBStageMuxInputType>(type) == WBStageMuxInputType::ALUResult) {
        this->alu_result = value;
        this->is_alu_result_set = true;
    } else if (std::get<WBStageMuxInputType>(type) == WBStageMuxInputType::ReadData) {
        this->read_data = value;
        this->is_read_data_set = true;
    }

    this->notifyModuleConditionVariable();
}

void WBMux::assertControlSignal(bool is_asserted) {
    this->is_mem_to_reg_asserted = is_asserted;
    this->is_control_signal_set = true;
    this->notifyModuleConditionVariable();
}

void WBMux::passOutput() {
    if (this->is_mem_to_reg_asserted) {
        this->register_file->setWriteData(this->read_data);
    } else {
        this->register_file->setWriteData(this->alu_result);
    }
}

void WBMux::passOutputToForwardingUnit() {
    if (this->is_mem_to_reg_asserted) {
        this->alu_input_1_forwarding_mux->setInput(ALUInputMuxInputTypes::MEMWBStageRegisters, this->read_data);
        this->alu_input_2_forwarding_mux->setInput(ALUInputMuxInputTypes::MEMWBStageRegisters, this->read_data);
    } else {
        this->alu_input_1_forwarding_mux->setInput(ALUInputMuxInputTypes::MEMWBStageRegisters, this->alu_result);
        this->alu_input_2_forwarding_mux->setInput(ALUInputMuxInputTypes::MEMWBStageRegisters, this->alu_result);
    }
}