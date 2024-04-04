#include "../../../../include/combinational/ALU.h"

ALUInputForwardingMuxBase::ALUInputForwardingMuxBase() {
    this->id_ex_stage_registers_value = std::bitset<WORD_BIT_COUNT>(std::string(32, '0'));
    this->ex_mem_stage_registers_value = std::bitset<WORD_BIT_COUNT>(std::string(32, '0'));
    this->mem_wb_stage_registers_value = std::bitset<WORD_BIT_COUNT>(std::string(32, '0'));

    this->control_signal = ALUInputMuxControlSignals::IDEXStageRegisters;

    this->is_reset_flag_set = false;

    this->alu = nullptr;
    this->logger = nullptr;

    this->is_id_ex_stage_registers_value_set = false;
    this->is_ex_mem_stage_registers_value_set = false;
    this->is_mem_wb_stage_registers_value_set = false;
}

void ALUInputForwardingMuxBase::initDependencies() {
    std::lock_guard<std::mutex> mux_lock (this->getModuleMutex());

    if (this->alu && this->logger) {
        return;
    }

    this->alu = ALU::init();
    this->logger = Logger::init();
}

void ALUInputForwardingMuxBase::setInput(MuxInputType type, MuxInputDataType value) {
    if (!std::holds_alternative<ALUInputMuxInputTypes>(type) ||
            !std::holds_alternative<std::bitset<WORD_BIT_COUNT>>(value)) {
        throw std::runtime_error("ALUInputForwardingMuxBase::setInput: incompatible data types passed.");
    }

    this->log("setInput waiting to acquire lock.");

    std::lock_guard<std::mutex> mux_lock (this->getModuleMutex());

    if (std::get<ALUInputMuxInputTypes>(type) == ALUInputMuxInputTypes::IDEXStageRegisters) {
        this->id_ex_stage_registers_value = std::bitset<WORD_BIT_COUNT>(std::get<std::bitset<WORD_BIT_COUNT>>(value));
        this->is_id_ex_stage_registers_value_set = true;
    } else if (std::get<ALUInputMuxInputTypes>(type) == ALUInputMuxInputTypes::EXMEMStageRegisters) {
        this->ex_mem_stage_registers_value = std::bitset<WORD_BIT_COUNT>(std::get<std::bitset<WORD_BIT_COUNT>>(value));
        this->is_ex_mem_stage_registers_value_set = true;
    } else if (std::get<ALUInputMuxInputTypes>(type) == ALUInputMuxInputTypes::MEMWBStageRegisters) {
        this->mem_wb_stage_registers_value = std::bitset<WORD_BIT_COUNT>(std::get<std::bitset<WORD_BIT_COUNT>>(value));
        this->is_mem_wb_stage_registers_value_set = true;
    } else {
        throw std::runtime_error("ALUInputForwardingMuxBase::setInput parameters did not match any input type.");
    }

    this->log("setInput updated value.");
    this->notifyModuleConditionVariable();
}

void ALUInputForwardingMuxBase::setMuxControlSignal(ALUInputMuxControlSignals new_signal) {
    this->log("setControlSignal waiting to be woken up and acquire lock.");

    std::lock_guard<std::mutex> mux_lock (this->getModuleMutex());

    this->control_signal = new_signal;
    this->is_control_signal_set = true;

    this->log("setControlSignal control set.");
    this->notifyModuleConditionVariable();
}

void ALUInputForwardingMuxBase::resetState() {
    this->is_id_ex_stage_registers_value_set = false;
    this->is_ex_mem_stage_registers_value_set = false;
    this->is_mem_wb_stage_registers_value_set = false;

    this->id_ex_stage_registers_value = std::bitset<WORD_BIT_COUNT>(std::string(32, '0'));
    this->ex_mem_stage_registers_value = std::bitset<WORD_BIT_COUNT>(std::string(32, '0'));
    this->mem_wb_stage_registers_value = std::bitset<WORD_BIT_COUNT>(std::string(32, '0'));
}

void ALUInputForwardingMuxBase::reset() {
    std::lock_guard<std::mutex> alu_input_forwarding_mux_lock (this->getModuleMutex());

    this->is_reset_flag_set = true;
    this->notifyModuleConditionVariable();
}

void ALUInputForwardingMuxBase::run() {
    this->initDependencies();

    while (this->isAlive()) {
        this->log("Waiting to be woken up and acquire lock.");

        std::unique_lock<std::mutex> mux_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                mux_lock,
                [this] {
                    return this->is_id_ex_stage_registers_value_set && (this->getPipelineType() == PipelineType::Single ||
                            (this->is_ex_mem_stage_registers_value_set && this->is_mem_wb_stage_registers_value_set))
                            && this->is_control_signal_set || this->isKilled() || this->is_reset_flag_set;
                }
        );

        if (this->isKilled()) {
            this->log("Killed.");
            break;
        }

        if (this->is_reset_flag_set) {
            this->log("Resetting stage.");

            this->resetState();
            this->is_reset_flag_set = false;

            this->log("Reset.");
            continue;
        }

        this->log("Woken up and acquired lock.");

        this->passOutput();

        this->is_id_ex_stage_registers_value_set = false;
        this->is_ex_mem_stage_registers_value_set = false;
        this->is_mem_wb_stage_registers_value_set = false;
        this->is_control_signal_set = false;
    }
}

Stage ALUInputForwardingMuxBase::getModuleStage() {
    return Stage::EX;
}
