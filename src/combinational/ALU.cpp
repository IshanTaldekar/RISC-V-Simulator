#include "../../include/combinational/ALU.h"

ALU *ALU::current_instance = nullptr;

ALU::ALU() {
    this->input1 = -1UL;
    this->input2 = -1UL;
    this->result = -1UL;

    this->ex_mem_stage_registers = EXMEMStageRegisters::init();

    this->is_result_zero = false;
    this->is_input1_set = false;
    this->is_input2_set = false;
    this->is_alu_op_set = false;
}

void ALU::setInput1(unsigned long value) {
    std::lock_guard<std::mutex> alu_lock (this->getModuleMutex());

    this->input1 = value;
    this->is_input1_set = true;

    this->notifyModuleConditionVariable();
}

void ALU::setInput2(unsigned long value) {
    std::lock_guard<std::mutex> alu_lock (this->getModuleMutex());

    this->input2 = value;
    this->is_input2_set = true;

    this->notifyModuleConditionVariable();
}

void ALU::setALUOp(const std::bitset<ALU_OP_BIT_COUNT> &value) {
    std::lock_guard<std::mutex> alu_lock (this->getModuleMutex());

    this->alu_op = value;
    this->is_alu_op_set = true;

    this->notifyModuleConditionVariable();
}

void ALU::notifyModuleConditionVariable() {
    this->getModuleConditionVariable().notify_one();
}

void ALU::run() {
    while (this->isAlive()) {
        std::unique_lock<std::mutex> alu_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                alu_lock,
                [this] {
                    return this->is_input1_set && this->is_input2_set && this->is_alu_op_set;
                }
        );

        if (this->isKilled()) {
            break;
        }

        this->computeResult();

        std::thread pass_result_thread(&ALU::passResultToEXMEMStageRegisters, this);
        std::thread pass_zero_flag_thread (&ALU::passZeroFlagToEXMEMStageRegisters, this);

        this->is_alu_op_set = false;
        this->is_input1_set = false;
        this->is_input2_set = false;
        this->is_result_zero = false;
    }
}

ALU *ALU::init() {
    if (ALU::current_instance == nullptr) {
        ALU::current_instance = new ALU();
    }

    return ALU::current_instance;
}

void ALU::computeResult() {
    if (this->alu_op == std::bitset<ALU_OP_BIT_COUNT>("0000")) {  // Add
        this->result = this->input1 + this->input2;
    } else if (this->alu_op == std::bitset<ALU_OP_BIT_COUNT>("0001")) {  // subtract
        this->result = this->input1 - this->input2;
    } else if (this->alu_op == std::bitset<ALU_OP_BIT_COUNT>("0010")) {  // Xor
        this->result = this->input1 ^ this->input2;
    } else if (this->alu_op == std::bitset<ALU_OP_BIT_COUNT>("0011")) {  // Or
        this->result = this->input1 | this->input2;
    } else if (this->alu_op == std::bitset<ALU_OP_BIT_COUNT>("0100")) {  // And
        this->result = this->input1 & this->input2;
    }

    this->is_result_zero = this->result == 0;
}

void ALU::passZeroFlagToEXMEMStageRegisters() {
    this->ex_mem_stage_registers->setIsResultZeroFlag(this->is_result_zero);
}

void ALU::passResultToEXMEMStageRegisters() {
    this->ex_mem_stage_registers->setALUResult(this->result);
}
