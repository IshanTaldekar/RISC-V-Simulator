#include "../../../../include/combinational/ALU.h"

ALUInputForwardingMuxBase::ALUInputForwardingMuxBase() {
    this->id_ex_stage_registers_value = 0UL;
    this->ex_mem_stage_registers_value = 0UL;
    this->mem_wb_stage_registers_value = 0UL;

    this->control_signal = ALUInputMuxControlSignals::IDEXStageRegisters;

    this->alu = nullptr;
    this->logger = nullptr;

    this->is_id_ex_stage_registers_value_set = false;
    this->is_ex_mem_stage_registers_value_set = false;
    this->is_mem_wb_stage_registers_value_set = false;
}

void ALUInputForwardingMuxBase::initDependencies() {
    this->alu = ALU::init();
    this->logger = Logger::init();
}

void ALUInputForwardingMuxBase::setInput(MuxInputType type, unsigned long value) {
    if (!std::holds_alternative<ALUInputMuxInputTypes>(type)) {
        throw std::runtime_error("MuxInputType passed to ALUInputForwardingMuxBase not compatible with ALUInputMuxInputTypes.");
    }

    this->logger->log(Stage::EX, "[" + this->getModuleTag() + "] setInput waiting to acquire lock.");

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

    this->logger->log(Stage::EX, "[" + this->getModuleTag() + "] setInput updated value.");
    this->notifyModuleConditionVariable();
}

void ALUInputForwardingMuxBase::setMuxControlSignal(ALUInputMuxControlSignals new_signal) {
    this->logger->log(Stage::EX, "[" + this->getModuleTag() + "] setControlSignal waiting to be "
                                                              "woken up and acquire lock.");

    std::lock_guard<std::mutex> mux_lock (this->getModuleMutex());

    this->control_signal = new_signal;
    this->is_control_signal_set = true;

    this->logger->log(Stage::EX, "[" + this->getModuleTag() + "] setControlSignal control set.");
    this->notifyModuleConditionVariable();
}

void ALUInputForwardingMuxBase::run() {
    this->initDependencies();

    while (this->isAlive()) {
        this->logger->log(Stage::EX, "[" + this->getModuleTag() + "] Waiting to be woken up and acquire lock.");

        std::unique_lock<std::mutex> mux_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                mux_lock,
                [this] {
                    return this->is_id_ex_stage_registers_value_set && (this->getPipelineType() == PipelineType::Single ||
                            (this->is_ex_mem_stage_registers_value_set && this->is_mem_wb_stage_registers_value_set))
                            && this->is_control_signal_set || this->isKilled();
                }
        );

        if (this->isKilled()) {
            this->logger->log(Stage::EX, "[" + this->getModuleTag() + "] Killed.");
            break;
        }

        this->logger->log(Stage::EX, "[" + this->getModuleTag() + "] Woken up and acquired lock.");

        this->passOutput();

        this->is_id_ex_stage_registers_value_set = false;
        this->is_ex_mem_stage_registers_value_set = false;
        this->is_mem_wb_stage_registers_value_set = false;
        this->is_control_signal_set = false;
    }
}
