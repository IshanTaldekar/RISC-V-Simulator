#include "../../../include/state/stage-registers/IDEXStageRegisters.h"

IDEXStageRegisters *IDEXStageRegisters::current_instance = nullptr;

IDEXStageRegisters::IDEXStageRegisters() {
    this->instruction = new Instruction(std::string(32, '0'));
    this->control = new Control(this->instruction);

    this->register_source1 = 0UL;
    this->register_source2 = 0UL;
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
    this->is_register_source1_set = false;
    this->is_register_source2_set = false;

    this->ex_mux_alu_input_1 = EXMuxALUInput1::init();
    this->ex_mux_alu_input_2 = EXMuxALUInput2::init();
    this->ex_adder = EXAdder::init();
    this->forwarding_unit = ForwardingUnit::init();
    this->ex_mem_stage_register = EXMEMStageRegisters::init();
    this->stage_synchronizer = StageSynchronizer::init();
}

void IDEXStageRegisters::reset() {
    this->is_reset_flag_set = true;
    this->notifyModuleConditionVariable();
}

void IDEXStageRegisters::resetStage() {
    if (this->getStage() == PipelineType::Single) {
        this->is_single_read_register_data_set = false;
        this->is_double_read_register_data_set = false;
        this->is_immediate_set = false;
        this->is_register_destination_set = false;
        this->is_program_counter_set = false;
        this->is_control_set = false;
        this->is_register_source1_set = false;
        this->is_register_source2_set = false;
    } else {
        this->is_single_read_register_data_set = true;
        this->is_double_read_register_data_set = true;
        this->is_immediate_set = true;
        this->is_register_destination_set = true;
        this->is_program_counter_set = true;
        this->is_control_set = true;
        this->is_register_source1_set = true;
        this->is_register_source2_set = true;
    }

    this->program_counter = 0UL;
    this->register_destination = 0UL;
    this->register_source1 = 0UL;
    this->register_source2 = 0UL;

    this->is_nop_asserted = false;
    this->is_reset_flag_set = false;

    this->instruction = new Instruction(std::string(32, '0'));
    this->control = new Control(this->instruction);
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
                           this->is_immediate_set && this->is_control_set && this->is_register_source1_set &&
                           this->is_register_source2_set) || this->is_reset_flag_set || this->is_pause_flag_set;
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
        this->passProgramCounterToEXMuxALUInput1();
        this->passReadData1ToExMuxALUInput1();
        this->passReadData2ToExMuxALUInput2();
        this->passImmediateToEXMuxALUInput2();
        this->passImmediateToEXAdder();

        std::thread pass_register_destination_thread (&IDEXStageRegisters::passRegisterDestinationToEXMEMStageRegisters, this);
        std::thread pass_read_data2_thread (&IDEXStageRegisters::passReadData2ToEXMEMStageRegisters, this);
        std::thread pass_control_thread (&IDEXStageRegisters::passControlToEXMEMStageRegisters, this);
        std::thread pass_register_sources_thread (&IDEXStageRegisters::passRegisterSourceToForwardingUnit, this);

        pass_register_destination_thread.join();
        pass_read_data2_thread.join();
        pass_control_thread.join();
        pass_register_sources_thread.join();

        this->is_single_read_register_data_set = false;
        this->is_double_read_register_data_set = false;
        this->is_immediate_set = false;
        this->is_register_destination_set = false;
        this->is_program_counter_set = false;
        this->is_control_set = false;
        this->is_nop_asserted = false;
        this->is_register_source1_set = false;
        this->is_register_source2_set = false;

        this->stage_synchronizer->conditionalArriveSingleStage();
    }
}

void IDEXStageRegisters::notifyModuleConditionVariable() {
    this->getModuleConditionVariable().notify_one();
}

void IDEXStageRegisters::setRegisterData(const std::bitset<WORD_BIT_COUNT> &rd1) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    if (!this->is_nop_asserted) {
        this->read_data_1 = rd1;
    }

    this->is_single_read_register_data_set = true;
    this->notifyModuleConditionVariable();
}

void IDEXStageRegisters::setRegisterData(const std::bitset<WORD_BIT_COUNT> &rd1, const std::bitset<WORD_BIT_COUNT> &rd2) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    if (!this->is_nop_asserted) {
        this->read_data_1 = rd1;
        this->read_data_2 = rd2;
    }

    this->is_double_read_register_data_set = true;
    this->notifyModuleConditionVariable();
}

void IDEXStageRegisters::setImmediate(const std::bitset<WORD_BIT_COUNT> &imm) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    if (!this->is_nop_asserted) {
        this->immediate = imm;
    }

    this->is_immediate_set = true;
    this->notifyModuleConditionVariable();
}

void IDEXStageRegisters::setRegisterDestination(unsigned long rd) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    if (!this->is_nop_asserted) {
        this->register_destination = rd;
    }

    this->is_register_destination_set = true;
    this->notifyModuleConditionVariable();
}

void IDEXStageRegisters::setProgramCounter(unsigned long pc) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    if (!this->is_nop_asserted) {
        this->program_counter = pc;
    }

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

void IDEXStageRegisters::setInstruction(Instruction *current_instruction) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    if (!this->is_nop_asserted) {
        this->instruction = current_instruction;
    }
}

void IDEXStageRegisters::passProgramCounterToEXAdder() {
    this->ex_adder->setInput(EXAdderInputType::PCValue, this->program_counter);
}

void IDEXStageRegisters::passProgramCounterToEXMuxALUInput1() {
    this->ex_mux_alu_input_1->setInput(EXStageMuxALUInput1InputType::ProgramCounter, this->program_counter);
}

void IDEXStageRegisters::passReadData1ToExMuxALUInput1() {
    this->ex_mux_alu_input_1->setInput(EXStageMuxALUInput1InputType::ReadData1, this->read_data_1.to_ulong());
}

void IDEXStageRegisters::passReadData2ToExMuxALUInput2() {
    this->ex_mux_alu_input_2->setInput(EXStageMuxALUInput2InputType::ReadData2, this->read_data_2.to_ulong());
}

void IDEXStageRegisters::passImmediateToEXMuxALUInput2() {
    this->ex_mux_alu_input_2->setInput(EXStageMuxALUInput2InputType::ImmediateValue, this->immediate.to_ulong());
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

void IDEXStageRegisters::assertNop() {
    this->is_nop_asserted = true;
}

void IDEXStageRegisters::setRegisterSource1(unsigned long rs1) {
    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    this->register_source1 = rs1;
    this->is_register_source1_set = true;
}

void IDEXStageRegisters::setRegisterSource2(unsigned long rs2) {
    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    this->register_source2 = rs2;
    this->is_register_source2_set = true;
}

void IDEXStageRegisters::passRegisterSourceToForwardingUnit() {
    if (this->is_single_read_register_data_set) {
        this->forwarding_unit->setSingleRegisterSource(this->register_source1);
    } else {
        this->forwarding_unit->setDoubleRegisterSource(this->register_source1, this->register_source2);
    }
}