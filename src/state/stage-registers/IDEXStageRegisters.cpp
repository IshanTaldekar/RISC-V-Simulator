#include "../../../include/state/stage-registers/IDEXStageRegisters.h"

IDEXStageRegisters::IDEXStageRegisters() {
    this->instruction = nullptr;
    this->control = nullptr;

    this->register_destination = -1;
    this->program_counter = -1;

    this->is_single_read_register_data_set = false;
    this->is_double_read_register_data_set = false;
    this->is_immediate_set = false;
    this->is_register_destination_set = false;
    this->is_program_counter_set = false;
    this->is_control_set = false;
}

IDEXStageRegisters *IDEXStageRegisters::init() {
    if (IDEXStageRegisters::current_instance == nullptr) {
        IDEXStageRegisters::current_instance = new IDEXStageRegisters();
    }

    return IDEXStageRegisters::current_instance;
}

void IDEXStageRegisters::run() {
    while (this->isAlive()) {
        std::unique_lock<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                id_ex_stage_registers_lock,
                [this] {
                    return (this->is_single_read_register_data_set || this->is_double_read_register_data_set) &&
                           this->is_program_counter_set && this->is_register_destination_set &&
                           this->is_immediate_set && this->is_control_set;
                }
        );

        this->control->setEXStageControlSignals();

        // TODO: load data to next stage

        this->is_single_read_register_data_set = false;
        this->is_double_read_register_data_set = false;
        this->is_immediate_set = false;
        this->is_register_destination_set = false;
        this->is_program_counter_set = false;
        this->is_control_set = false;
    }
}

void IDEXStageRegisters::notifyModuleConditionVariable() {
    this->getModuleConditionVariable().notify_one();
}

void IDEXStageRegisters::setRegisterData(const std::bitset<WORD_BIT_COUNT> &rd1) {
    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    this->read_data_1 = rd1;
    this->is_single_read_register_data_set = true;
}

void IDEXStageRegisters::setRegisterData(const std::bitset<WORD_BIT_COUNT> &rd1, const std::bitset<WORD_BIT_COUNT> &rd2) {
    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    this->read_data_1 = rd1;
    this->read_data_2 = rd2;

    this->is_double_read_register_data_set = true;
}

void IDEXStageRegisters::setImmediate(const std::bitset<WORD_BIT_COUNT> &imm) {
    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    this->immediate = imm;
    this->is_immediate_set = true;
}

void IDEXStageRegisters::setRegisterDestination(unsigned long rd) {
    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    this->register_destination = rd;
    this->is_register_destination_set = true;
}

void IDEXStageRegisters::setProgramCounter(int pc) {
    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    this->program_counter = pc;
    this->is_program_counter_set = true;
}

void IDEXStageRegisters::setControlModule(Control *new_control) {
    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    this->control = new_control;
    this->is_control_set = true;
}