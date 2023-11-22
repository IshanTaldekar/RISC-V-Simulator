#include "../../../include/state/stage-registers/IDEXStageRegisters.h"

IDEXStageRegisters *IDEXStageRegisters::current_instance = nullptr;

IDEXStageRegisters::IDEXStageRegisters() {
    this->control = new Control(new Instruction(std::string(32, '0')));

    this->register_destination = 0UL;
    this->program_counter = 0UL;

    this->is_single_read_register_data_set = false;
    this->is_double_read_register_data_set = false;
    this->is_immediate_set = false;
    this->is_register_destination_set = false;
    this->is_program_counter_set = false;
    this->is_control_set = false;
    this->is_nop_asserted = false;
    this->is_reset_flag_set = false;
    this->is_pause_flag_set = false;

    this->ex_mux = EXMux::init();
    this->ex_adder = EXAdder::init();
    this->alu = ALU::init();
    this->ex_mem_stage_register = EXMEMStageRegisters::init();
    this->stage_synchronizer = StageSynchronizer::init();
}

void IDEXStageRegisters::reset() {
    this->is_reset_flag_set = true;
    this->notifyModuleConditionVariable();
}

void IDEXStageRegisters::resetStage() {
    if (this->getStage() == Stage::Single) {
        this->is_single_read_register_data_set = false;
        this->is_double_read_register_data_set = false;
        this->is_immediate_set = false;
        this->is_register_destination_set = false;
        this->is_program_counter_set = false;
        this->is_control_set = false;
    } else {
        this->is_single_read_register_data_set = true;
        this->is_double_read_register_data_set = true;
        this->is_immediate_set = true;
        this->is_register_destination_set = true;
        this->is_program_counter_set = true;
        this->is_control_set = true;
    }

    this->program_counter = 0UL;
    this->register_destination = 0UL;

    this->is_nop_asserted = false;
    this->is_reset_flag_set = false;

    this->control = new Control(new Instruction(std::string(32, '0')));
}

void IDEXStageRegisters::pauseStage() {
    this->is_single_read_register_data_set = false;
    this->is_double_read_register_data_set = false;
    this->is_immediate_set = false;
    this->is_register_destination_set = false;
    this->is_program_counter_set = false;
    this->is_control_set = false;
}

void IDEXStageRegisters::pause() {
    this->is_pause_flag_set = true;
    this->notifyModuleConditionVariable();
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
                    return ((this->is_single_read_register_data_set || this->is_double_read_register_data_set) &&
                           this->is_program_counter_set && this->is_register_destination_set &&
                           this->is_immediate_set && this->is_control_set) || this->is_reset_flag_set ||
                           this->is_pause_flag_set;
                }
        );

        if (this->isKilled()) {
            break;
        }

        if (this->is_pause_flag_set) {
            this->pauseStage();
            this->is_pause_flag_set = false;

            continue;
        }

        if (this->is_reset_flag_set) {
            this->resetStage();
            this->is_reset_flag_set = false;

            continue;
        }

        this->control->toggleEXStageControlSignals();

        this->passProgramCounterToEXAdder();
        this->passReadData1ToALU();
        this->passReadData2ToExMux();
        this->passImmediateToEXMux();
        this->passImmediateToEXAdder();

        std::thread pass_register_destination_thread (&IDEXStageRegisters::passRegisterDestinationToEXMEMStageRegisters, this);
        std::thread pass_read_data2_thread (&IDEXStageRegisters::passReadData2ToEXMEMStageRegisters, this);
        std::thread pass_control_thread (&IDEXStageRegisters::passControlToEXMEMStageRegisters, this);

        this->is_single_read_register_data_set = false;
        this->is_double_read_register_data_set = false;
        this->is_immediate_set = false;
        this->is_register_destination_set = false;
        this->is_program_counter_set = false;
        this->is_control_set = false;
        this->is_nop_asserted = false;

        this->stage_synchronizer->conditionalArriveSingleStage();
    }
}

void IDEXStageRegisters::notifyModuleConditionVariable() {
    this->getModuleConditionVariable().notify_one();
}

void IDEXStageRegisters::setRegisterData(const std::bitset<WORD_BIT_COUNT> &rd1) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    this->read_data_1 = rd1;
    this->is_single_read_register_data_set = true;

    this->notifyModuleConditionVariable();
}

void IDEXStageRegisters::setRegisterData(const std::bitset<WORD_BIT_COUNT> &rd1, const std::bitset<WORD_BIT_COUNT> &rd2) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    this->read_data_1 = rd1;
    this->read_data_2 = rd2;

    this->is_double_read_register_data_set = true;

    this->notifyModuleConditionVariable();
}

void IDEXStageRegisters::setImmediate(const std::bitset<WORD_BIT_COUNT> &imm) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    this->immediate = imm;
    this->is_immediate_set = true;

    this->notifyModuleConditionVariable();
}

void IDEXStageRegisters::setRegisterDestination(unsigned long rd) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    this->register_destination = rd;
    this->is_register_destination_set = true;

    this->notifyModuleConditionVariable();
}

void IDEXStageRegisters::setProgramCounter(int pc) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    this->program_counter = pc;
    this->is_program_counter_set = true;

    this->notifyModuleConditionVariable();
}

void IDEXStageRegisters::setControlModule(Control *new_control) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    if (this->is_nop_asserted) {
        this->control = new Control(new Instruction(std::string(32, '0')));
    } else {
        this->control = new_control;
    }

    this->is_control_set = true;

    this->notifyModuleConditionVariable();
}

void IDEXStageRegisters::passProgramCounterToEXAdder() {
    this->ex_adder->setInput(EXAdderInputType::PCValue, this->program_counter);
}

void IDEXStageRegisters::passReadData1ToALU() {
    this->alu->setInput1(this->read_data_1.to_ulong());
}

void IDEXStageRegisters::passReadData2ToExMux() {
    this->ex_mux->setInput(EXStageMuxInputType::ReadData2, this->read_data_2.to_ulong());
}

void IDEXStageRegisters::passImmediateToEXMux() {
    this->ex_mux->setInput(EXStageMuxInputType::ImmediateValue, this->immediate.to_ulong());
}

void IDEXStageRegisters::passImmediateToEXAdder() {
    this->ex_adder->setInput(EXAdderInputType::ImmediateValue, this->immediate.to_ulong());
}

void IDEXStageRegisters::passRegisterDestinationToEXMEMStageRegisters() {
    this->ex_mem_stage_register->setRegisterDestination(this->register_destination);
}

void IDEXStageRegisters::passReadData2ToEXMEMStageRegisters() {
    this->ex_mem_stage_register->setReadData2(this->read_data_2.to_ulong());
}

void IDEXStageRegisters::passControlToEXMEMStageRegisters() {
    this->ex_mem_stage_register->setControl(this->control);
}

void IDEXStageRegisters::setNop() {
    this->is_nop_asserted = true;
}