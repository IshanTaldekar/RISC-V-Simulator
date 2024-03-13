#include "../../../include/state/stage-registers/IDEXStageRegisters.h"

IDEXStageRegisters *IDEXStageRegisters::current_instance = nullptr;
std::mutex IDEXStageRegisters::initialization_mutex;

IDEXStageRegisters::IDEXStageRegisters() {
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
    this->is_instruction_set = false;

    this->instruction = nullptr;
    this->control = nullptr;

    this->ex_mux_alu_input_1 = nullptr;
    this->ex_mux_alu_input_2 = nullptr;
    this->ex_adder = nullptr;
    this->forwarding_unit = nullptr;
    this->ex_mem_stage_register = nullptr;
    this->stage_synchronizer = nullptr;
    this->logger = nullptr;
}

void IDEXStageRegisters::changeStageAndReset(PipelineType new_pipeline_type) {
    {  // Limit lock guard scope to avoid deadlock
        std::lock_guard<std::mutex> if_id_stage_registers_lock(this->getModuleMutex());

        this->logger->log(Stage::ID, "[IDEXStageRegisters] PipelineType change.");
        this->setPipelineType(new_pipeline_type);
    }

    this->reset();
}

void IDEXStageRegisters::reset() {
    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    this->is_reset_flag_set = true;
    this->notifyModuleConditionVariable();
}

void IDEXStageRegisters::resetStage() {
    if (this->getPipelineType() == PipelineType::Single) {
        this->is_single_read_register_data_set = false;
        this->is_double_read_register_data_set = false;
        this->is_immediate_set = false;
        this->is_register_destination_set = false;
        this->is_program_counter_set = false;
        this->is_control_set = false;
        this->is_register_source1_set = false;
        this->is_register_source2_set = false;
        this->is_instruction_set = false;
    } else {
        this->is_single_read_register_data_set = true;
        this->is_double_read_register_data_set = true;
        this->is_immediate_set = true;
        this->is_register_destination_set = true;
        this->is_program_counter_set = true;
        this->is_control_set = true;
        this->is_register_source1_set = true;
        this->is_register_source2_set = true;
        this->is_instruction_set = true;
    }

    this->program_counter = 0UL;
    this->register_destination = 0UL;
    this->register_source1 = 0UL;
    this->register_source2 = 0UL;

    this->is_nop_asserted = false;
    this->is_reset_flag_set = false;
}

void IDEXStageRegisters::pause() {
    this->is_pause_flag_set = true;
}

void IDEXStageRegisters::resume() {
    this->is_pause_flag_set = false;
    this->notifyModuleConditionVariable();
}

IDEXStageRegisters *IDEXStageRegisters::init() {
    std::lock_guard<std::mutex> id_ex_stage_registers_lock (IDEXStageRegisters::initialization_mutex);

    if (IDEXStageRegisters::current_instance == nullptr) {
        IDEXStageRegisters::current_instance = new IDEXStageRegisters();
    }

    return IDEXStageRegisters::current_instance;
}

void IDEXStageRegisters::initDependencies() {
    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    this->ex_mux_alu_input_1 = EXMuxALUInput1::init();
    this->ex_mux_alu_input_2 = EXMuxALUInput2::init();
    this->ex_adder = EXAdder::init();
    this->forwarding_unit = ForwardingUnit::init();
    this->ex_mem_stage_register = EXMEMStageRegisters::init();
    this->stage_synchronizer = StageSynchronizer::init();
    this->logger = Logger::init();
}

void IDEXStageRegisters::run() {
    this->initDependencies();

    while (this->isAlive()) {
        this->logger->log(Stage::ID, "[IDEXStageRegisters] Waiting to be woken up and acquire lock.");

        std::unique_lock<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                id_ex_stage_registers_lock,
                [this] {
                    return ((this->is_single_read_register_data_set || this->is_double_read_register_data_set) &&
                           this->is_program_counter_set && this->is_register_destination_set &&
                           this->is_immediate_set && this->is_control_set && this->is_register_source1_set &&
                           this->is_register_source2_set && this->is_instruction_set && !this->is_pause_flag_set)
                           || this->is_reset_flag_set || this->isKilled();
                }
        );

        if (this->isKilled()) {
            this->logger->log(Stage::ID, "[IDEXStageRegisters] Killed.");
            break;
        }

        if (this->is_reset_flag_set) {
            this->logger->log(Stage::ID, "[IDEXStageRegisters] Resetting.");

            this->resetStage();
            this->is_reset_flag_set = false;

            this->logger->log(Stage::ID, "[IDEXStageRegisters] Reset.");
            continue;
        }

        this->logger->log(Stage::ID, "[IDEXStageRegisters] Woken up and acquired lock.");

        if (!this->control || !this->instruction) {
            this->instruction = new Instruction(std::string(32, '0'));
            this->control = new Control(this->instruction);
        }

        this->control->toggleEXStageControlSignals();

        this->passProgramCounterToEXAdder();
        this->passProgramCounterToEXMuxALUInput1();
        this->passReadData1ToExMuxALUInput1();
        this->passReadData2ToExMuxALUInput2();
        this->passImmediateToEXMuxALUInput2();
        this->passImmediateToEXAdder();
        this->passRegisterSourceToForwardingUnit();

        std::thread pass_register_destination_thread (&IDEXStageRegisters::passRegisterDestinationToEXMEMStageRegisters, this);
        std::thread pass_read_data2_thread (&IDEXStageRegisters::passReadData2ToEXMEMStageRegisters, this);
        std::thread pass_control_thread (&IDEXStageRegisters::passControlToEXMEMStageRegisters, this);

        pass_register_destination_thread.join();
        pass_read_data2_thread.join();
        pass_control_thread.join();

        this->is_single_read_register_data_set = false;
        this->is_double_read_register_data_set = false;
        this->is_immediate_set = false;
        this->is_register_destination_set = false;
        this->is_program_counter_set = false;
        this->is_control_set = false;
        this->is_nop_asserted = false;
        this->is_instruction_set = false;
        this->is_register_source1_set = false;
        this->is_register_source2_set = false;

        this->stage_synchronizer->conditionalArriveSingleStage();
    }
}

void IDEXStageRegisters::setRegisterData(const std::bitset<WORD_BIT_COUNT> &rd1) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    if (!this->logger) {
        this->initDependencies();
    }

    this->logger->log(Stage::ID, "[IDEXStageRegisters] setRegisterData waiting to acquire lock.");

    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    this->logger->log(Stage::ID, "[IDEXStageRegisters] setRegisterData acquired lock. Updating value.");

    if (!this->is_nop_asserted) {
        this->read_data_1 = rd1;
        this->logger->log(Stage::ID, "[IDEXStageRegisters] setRegisterData value updated.");
    } else {
        this->logger->log(Stage::ID, "[IDEXStageRegisters] setRegisterData update skipped. NOP asserted.");
    }

    this->is_single_read_register_data_set = true;
    this->notifyModuleConditionVariable();
}

void IDEXStageRegisters::setRegisterData(const std::bitset<WORD_BIT_COUNT> &rd1, const std::bitset<WORD_BIT_COUNT> &rd2) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    if (!this->logger) {
        this->initDependencies();
    }

    this->logger->log(Stage::ID, "[IDEXStageRegisters] setRegisterData waiting to acquire lock.");

    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    this->logger->log(Stage::ID, "[IDEXStageRegisters] setRegisterData acquired lock. Updating value.");

    if (!this->is_nop_asserted) {
        this->read_data_1 = rd1;
        this->read_data_2 = rd2;
        this->logger->log(Stage::ID, "[IDEXStageRegisters] setRegisterData value updated.");
    } else {
        this->logger->log(Stage::ID, "[IDEXStageRegisters] setRegisterData update skipped. NOP asserted.");
    }

    this->is_double_read_register_data_set = true;
    this->notifyModuleConditionVariable();
}

void IDEXStageRegisters::setImmediate(const std::bitset<WORD_BIT_COUNT> &imm) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    if (!this->logger) {
        this->initDependencies();
    }

    this->logger->log(Stage::ID, "[IDEXStageRegisters] setImmediate waiting to acquire lock.");

    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    this->logger->log(Stage::ID, "[IDEXStageRegisters] setImmediate acquired lock. Updating value.");

    if (!this->is_nop_asserted) {
        this->immediate = imm;
        this->logger->log(Stage::ID, "[IDEXStageRegisters] setImmediate value updated.");
    } else {
        this->logger->log(Stage::ID, "[IDEXStageRegisters] setImmediate update skipped. NOP asserted.");
    }

    this->is_immediate_set = true;
    this->notifyModuleConditionVariable();
}

void IDEXStageRegisters::setRegisterDestination(unsigned long rd) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    if (!this->logger) {
        this->initDependencies();
    }

    this->logger->log(Stage::ID, "[IDEXStageRegisters] setRegisterDestination waiting to acquire lock.");

    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    this->logger->log(Stage::ID, "[IDEXStageRegisters] setRegisterDestination acquired lock. Updating value.");

    if (!this->is_nop_asserted) {
        this->register_destination = rd;
        this->logger->log(Stage::ID, "[IDEXStageRegisters] setRegisterDestination value updated.");
    } else {
        this->logger->log(Stage::ID, "[IDEXStageRegisters] setRegisterDestination update skipped. NOP asserted.");
    }

    this->is_register_destination_set = true;
    this->notifyModuleConditionVariable();
}

void IDEXStageRegisters::setProgramCounter(unsigned long pc) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    if (!this->logger) {
        this->initDependencies();
    }

    this->logger->log(Stage::ID, "[IDEXStageRegisters] setProgramCounter waiting to acquire lock.");

    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    this->logger->log(Stage::ID, "[IDEXStageRegisters] setProgramCounter acquired lock. Updating value.");

    if (!this->is_nop_asserted) {
        this->program_counter = pc;
        this->logger->log(Stage::ID, "[IDEXStageRegisters] setProgramCounter value updated.");
    } else {
        this->logger->log(Stage::ID, "[IDEXStageRegisters] setProgramCounter update skipped. NOP asserted.");
    }

    this->is_program_counter_set = true;
    this->notifyModuleConditionVariable();
}

void IDEXStageRegisters::setControlModule(Control *new_control) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    if (!this->logger) {
        this->initDependencies();
    }

    this->logger->log(Stage::ID, "[IDEXStageRegisters] setControlModule waiting to acquire lock.");

    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    this->logger->log(Stage::ID, "[IDEXStageRegisters] setControlModule acquired lock. Updating value.");

    if (this->is_nop_asserted) {
        this->control = new Control(new Instruction(std::string(32, '0')));
        this->logger->log(Stage::ID, "[IDEXStageRegisters] setControlModule update skipped. NOP asserted.");
    } else {
        this->control = new_control;
        this->logger->log(Stage::ID, "[IDEXStageRegisters] setControlModule value updated.");
    }

    this->is_control_set = true;
    this->notifyModuleConditionVariable();
}

void IDEXStageRegisters::setInstruction(Instruction *current_instruction) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    if (!this->logger) {
        this->initDependencies();
    }

    this->logger->log(Stage::ID, "[IDEXStageRegisters] setInstruction waiting to acquire lock.");

    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    this->logger->log(Stage::ID, "[IDEXStageRegisters] setInstruction acquired lock. Updating value.");

    if (!this->is_nop_asserted) {
        this->instruction = current_instruction;
        this->logger->log(Stage::ID, "[IDEXStageRegisters] setInstruction acquired lock. Updating value.");
    } else {
        this->logger->log(Stage::ID, "[IDEXStageRegisters] setInstruction update skipped. NOP asserted.");
    }

    this->is_instruction_set = true;
    this->notifyModuleConditionVariable();
}

void IDEXStageRegisters::setRegisterSource1(unsigned long rs1) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    if (!this->logger) {
        this->initDependencies();
    }

    this->logger->log(Stage::ID, "[IDEXStageRegisters] setRegisterSource1 waiting to acquire lock.");

    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    this->logger->log(Stage::ID, "[IDEXStageRegisters] setRegisterSource1 acquired lock. Updating value.");

    if (!this->is_nop_asserted) {
        this->register_source1 = rs1;
        this->logger->log(Stage::ID, "[IDEXStageRegisters] setRegisterSource1 value updated.");
    } else {
        this->logger->log(Stage::ID, "[IDEXStageRegisters] setRegisterSource1 update skipped. NOP asserted.");
    }

    this->is_register_source1_set = true;
    this->notifyModuleConditionVariable();
}

void IDEXStageRegisters::setRegisterSource2(unsigned long rs2) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    if (!this->logger) {
        this->initDependencies();
    }

    this->logger->log(Stage::ID, "[IDEXStageRegisters] setRegisterSource2 waiting to acquire lock.");

    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    this->logger->log(Stage::ID, "[IDEXStageRegisters] setRegisterSource2 acquired lock. Updating value.");

    if (!this->is_nop_asserted) {
        this->register_source2 = rs2;
        this->logger->log(Stage::ID, "[IDEXStageRegisters] setRegisterSource2 value updated.");
    } else {
        this->logger->log(Stage::ID, "[IDEXStageRegisters] setRegisterSource2 update skipped. NOP asserted.");
    }

    this->is_register_source2_set = true;
    this->notifyModuleConditionVariable();
}

void IDEXStageRegisters::passProgramCounterToEXAdder() {
    this->logger->log(Stage::ID, "[IDEXStageRegisters] Passing program counter to EXAdder.");
    this->ex_adder->setInput(EXAdderInputType::PCValue, this->program_counter);
    this->logger->log(Stage::ID, "[IDEXStageRegisters] Passed program counter to EXAdder.");
}

void IDEXStageRegisters::passProgramCounterToEXMuxALUInput1() {
    this->logger->log(Stage::ID, "[IDEXStageRegisters] Passing program counter to EXMuxALUInput1.");
    this->ex_mux_alu_input_1->setInput(EXStageMuxALUInput1InputType::ProgramCounter, this->program_counter);
    this->logger->log(Stage::ID, "[IDEXStageRegisters] Passed program counter to EXMuxALUInput1.");
}

void IDEXStageRegisters::passReadData1ToExMuxALUInput1() {
    this->logger->log(Stage::ID, "[IDEXStageRegisters] Passing read data 1 to EXMuxALUInput1.");
    this->ex_mux_alu_input_1->setInput(EXStageMuxALUInput1InputType::ReadData1, this->read_data_1);
    this->logger->log(Stage::ID, "[IDEXStageRegisters] Passed read data 1 to EXMuxALUInput1.");
}

void IDEXStageRegisters::passReadData2ToExMuxALUInput2() {
    this->logger->log(Stage::ID, "[IDEXStageRegisters] Passing read data 2 to EXMuxALUInput2.");
    this->ex_mux_alu_input_2->setInput(EXStageMuxALUInput2InputType::ReadData2, this->read_data_2);
    this->logger->log(Stage::ID, "[IDEXStageRegisters] Passed read data 2 to EXMuxALUInput2.");
}

void IDEXStageRegisters::passImmediateToEXMuxALUInput2() {
    this->logger->log(Stage::ID, "[IDEXStageRegisters] Passing immediate to EXMuxALUInput2.");
    this->ex_mux_alu_input_2->setInput(EXStageMuxALUInput2InputType::ImmediateValue, this->immediate);
    this->logger->log(Stage::ID, "[IDEXStageRegisters] Passed immediate to EXMuxALUInput2.");
}

void IDEXStageRegisters::passImmediateToEXAdder() {
    this->logger->log(Stage::ID, "[IDEXStageRegisters] Passing immediate to EXAdder.");
    this->ex_adder->setInput(EXAdderInputType::ImmediateValue, this->immediate);
    this->logger->log(Stage::ID, "[IDEXStageRegisters] Passed immediate to EXAdder.");
}

void IDEXStageRegisters::passRegisterDestinationToEXMEMStageRegisters() {
    this->logger->log(Stage::ID, "[IDEXStageRegisters] Passing register destination to EXMEMStageRegisters.");
    this->ex_mem_stage_register->setRegisterDestination(this->register_destination);
    this->logger->log(Stage::ID, "[IDEXStageRegisters] Passed register destination to EXMEMStageRegisters.");
}

void IDEXStageRegisters::passReadData2ToEXMEMStageRegisters() {
    this->logger->log(Stage::ID, "[IDEXStageRegisters] Passing read data 2 to EXMEMStageRegisters.");
    this->ex_mem_stage_register->setReadData2(this->read_data_2);
    this->logger->log(Stage::ID, "[IDEXStageRegisters] Passed read data 2 to EXMEMStageRegisters.");
}

void IDEXStageRegisters::passControlToEXMEMStageRegisters() {
    this->logger->log(Stage::ID, "[IDEXStageRegisters] Passing control to EXMEMStageRegisters.");
    this->ex_mem_stage_register->setControl(this->control);
    this->logger->log(Stage::ID, "[IDEXStageRegisters] Passed control to EXMEMStageRegisters.");
}

void IDEXStageRegisters::assertNop() {
    this->logger->log(Stage::ID, "[IDEXStageRegisters] NOP asserted.");
    this->is_nop_asserted = true;
}

void IDEXStageRegisters::passRegisterSourceToForwardingUnit() {
    this->logger->log(Stage::ID, "[IDEXStageRegisters] Passing register source to Forwarding Unit.");

    if (this->is_single_read_register_data_set) {
        this->forwarding_unit->setSingleRegisterSource(this->register_source1);
    } else {
        this->forwarding_unit->setDoubleRegisterSource(this->register_source1, this->register_source2);
    }

    this->logger->log(Stage::ID, "[IDEXStageRegisters] Passed register source to Forwarding Unit.");
}