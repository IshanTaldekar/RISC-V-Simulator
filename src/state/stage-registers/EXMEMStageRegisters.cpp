#include "../../../include/state/stage-registers/EXMEMStageRegisters.h"

EXMEMStageRegisters::EXMEMStageRegisters() {
    this->branch_program_counter = -1;
    this->alu_result = -1;
    this->read_data_2 = -1;
    this->register_destination = -1;

    this->is_alu_result_zero = false;

    this->is_branch_program_counter_set = false;
    this->is_alu_result_set = false;
    this->is_read_data_2_set = false;
    this->is_register_destination_set = false;
    this->is_alu_result_zero_flag_set = false;
    this->is_control_set = false;

    this->control = nullptr;
}

EXMEMStageRegisters *EXMEMStageRegisters::init() {
    if (EXMEMStageRegisters::current_instance == nullptr) {
        EXMEMStageRegisters::current_instance = new EXMEMStageRegisters();
    }

    return EXMEMStageRegisters::current_instance;
}

void EXMEMStageRegisters::run() {

}

void EXMEMStageRegisters::notifyModuleConditionVariable() {
    this->getModuleConditionVariable().notify_one();
}

void EXMEMStageRegisters::setBranchedProgramCounter(unsigned long value) {
    std::lock_guard<std::mutex> ex_mem_stage_registers_lock (this->getModuleMutex());

    this->branch_program_counter = value;
    this->is_branch_program_counter_set = true;
}

void EXMEMStageRegisters::setALUResult(unsigned long value) {
    std::lock_guard<std::mutex> ex_mem_stage_registers_lock (this->getModuleMutex());

    this->alu_result = value;
    this->is_alu_result_set = true;
}

void EXMEMStageRegisters::setIsResultZeroFlag(bool asserted) {
    std::lock_guard<std::mutex> ex_mem_stage_registers_lock (this->getModuleMutex());

    this->is_alu_result_zero = asserted;
    this->is_alu_result_zero_flag_set = true;
}

void EXMEMStageRegisters::setReadData2(unsigned long value) {
    std::lock_guard<std::mutex> ex_mem_stage_registers_lock (this->getModuleMutex());

    this->read_data_2 = value;
    this->is_read_data_2_set = true;
}

void EXMEMStageRegisters::setRegisterDestination(unsigned long value) {
    std::lock_guard<std::mutex> ex_mem_stage_registers_lock (this->getModuleMutex());

    this->register_destination = value;
    this->is_register_destination_set = true;
}

void EXMEMStageRegisters::setControl(Control *new_control) {
    std::lock_guard<std::mutex> ex_mem_stage_registers_lock (this->getModuleMutex());

    this->control = new_control;
    this->is_control_set = true;
}