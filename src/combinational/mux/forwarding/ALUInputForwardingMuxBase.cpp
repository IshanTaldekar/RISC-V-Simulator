#include "../../../../include/combinational/ALU.h"

ALUInputForwardingMuxBase::ALUInputForwardingMuxBase() {
    this->id_ex_stage_registers_value = 0UL;
    this->ex_mem_stage_registers_value = 0UL;
    this->mem_wb_stage_registers_value = 0UL;

    this->control_signal = ALUInputMuxControlSignals::IDEXStageRegisters;

    this->is_id_ex_stage_registers_value_set = false;
    this->is_ex_mem_stage_registers_value_set = false;
    this->is_mem_wb_stage_registers_value_set = false;

    this->alu = ALU::init();
    this->logger = EXLogger::init();
}

void ALUInputForwardingMuxBase::setInput(MuxInputType type, unsigned long value) {
    if (!std::holds_alternative<ALUInputMuxInputTypes>(type)) {
        throw std::runtime_error("MuxInputType passed to ALUInputForwardingMuxBase not compatible with ALUInputMuxInputTypes.");
    }

    this->logger->log("[" + this->getModuleTag() + "] setInput waiting to acquire lock");

    std::lock_guard<std::mutex> mux_lock (this->getModuleMutex());

    if (std::get<ALUInputMuxInputTypes>(type) == ALUInputMuxInputTypes::IDEXStageRegisters) {
        this->id_ex_stage_registers_value = value;
        this->is_id_ex_stage_registers_value_set = true;
    } else if (std::get<ALUInputMuxInputTypes>(type) == ALUInputMuxInputTypes::EXMEMStageRegisters) {
        this->ex_mem_stage_registers_value = value;
        this->is_ex_mem_stage_registers_value_set = true;
    } else if (std::get<ALUInputMuxInputTypes>(type) == ALUInputMuxInputTypes::MEMWBStageRegisters) {
        this->mem_wb_stage_registers_value = value;
        this->is_mem_wb_stage_registers_value_set = true;
    } else {
        throw std::runtime_error("ALUInputForwardingMuxBase::setInput parameters did not match any input type.");
    }

    this->notifyModuleConditionVariable();
}

void ALUInputForwardingMuxBase::setMuxControlSignal(ALUInputMuxControlSignals new_signal) {
    std::lock_guard<std::mutex> mux_lock (this->getModuleMutex());

    this->control_signal = new_signal;
    this->is_control_signal_set = true;
}

void ALUInputForwardingMuxBase::run() {
    while (this->isAlive()) {
        this->logger->log("[" + this->getModuleTag() + "] Waiting to wakeup and acquire lock.");

        std::unique_lock<std::mutex> mux_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                mux_lock,
                [this] {
                    return this->is_id_ex_stage_registers_value_set && this->is_ex_mem_stage_registers_value_set
                           && this->is_mem_wb_stage_registers_value_set && this->is_control_signal_set;
                }
        );

        if (this->isKilled()) {
            break;
        }

        this->logger->log("[" + this->getModuleTag() + "] Woken up and acquired lock. Passing output.");

        this->passOutput();

        this->is_id_ex_stage_registers_value_set = false;
        this->is_ex_mem_stage_registers_value_set = false;
        this->is_mem_wb_stage_registers_value_set = false;
        this->is_control_signal_set = false;

        this->logger->log("[" + this->getModuleTag() + "] Passed output.");
    }
}

void ALUInputForwardingMuxBase::notifyModuleConditionVariable() {
    this->getModuleConditionVariable().notify_one();
}
