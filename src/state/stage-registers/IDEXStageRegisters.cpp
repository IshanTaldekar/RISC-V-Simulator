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
    this->is_nop_flag_set = true;
    this->is_nop_passed_flag_set = false;
    this->is_nop_passed_flag_asserted = false;

    this->instruction = nullptr;
    this->control = nullptr;

    this->ex_mux_alu_input_1 = nullptr;
    this->ex_mux_alu_input_2 = nullptr;
    this->ex_adder = nullptr;
    this->forwarding_unit = nullptr;
    this->ex_mem_stage_register = nullptr;
    this->stage_synchronizer = nullptr;
    this->logger = nullptr;
    this->hazard_detection_unit = nullptr;

    this->current_nop_set_operations = 0;
}

void IDEXStageRegisters::changeStageAndReset(PipelineType new_pipeline_type) {
    {  // Limit lock guard scope to avoid deadlock
        std::lock_guard<std::mutex> if_id_stage_registers_lock(this->getModuleMutex());

        this->log("PipelineType change.");
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
        this->is_nop_flag_set = true;
        this->is_nop_passed_flag_set = false;
        this->is_nop_asserted = false;
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
        this->is_nop_passed_flag_set = true;
        this->is_nop_asserted = true;
    }

    this->is_nop_flag_set = true;
    this->instruction = new Instruction(std::string(32, '0'));
    this->control = new Control(this->instruction);

    this->read_data_1 = std::bitset<WORD_BIT_COUNT>(std::string(32, '0'));
    this->read_data_2 = std::bitset<WORD_BIT_COUNT>(std::string(32, '0'));
    this->immediate = std::bitset<WORD_BIT_COUNT>(std::string(32, '0'));

    this->program_counter = 0UL;
    this->register_destination = 0UL;
    this->register_source1 = 0UL;
    this->register_source2 = 0UL;

    this->is_reset_flag_set = false;
}

void IDEXStageRegisters::pause() {
    this->is_pause_flag_set = true;
}

void IDEXStageRegisters::resume() {
    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

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
    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleDependencyMutex());

    if (this->ex_mux_alu_input_1 && this->ex_mux_alu_input_2 && this->ex_adder && this->forwarding_unit &&
        this->ex_mem_stage_register && this->stage_synchronizer && this->logger && this->hazard_detection_unit) {
        return;
    }

    this->ex_mux_alu_input_1 = EXMuxALUInput1::init();
    this->ex_mux_alu_input_2 = EXMuxALUInput2::init();
    this->ex_adder = EXAdder::init();
    this->forwarding_unit = ForwardingUnit::init();
    this->ex_mem_stage_register = EXMEMStageRegisters::init();
    this->stage_synchronizer = StageSynchronizer::init();
    this->logger = Logger::init();
    this->hazard_detection_unit = HazardDetectionUnit::init();
}

void IDEXStageRegisters::run() {
    this->initDependencies();

    while (this->isAlive()) {
        this->log("Waiting to be woken up and acquire lock.");

        std::unique_lock<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                id_ex_stage_registers_lock,
                [this] {
                    return ((this->is_single_read_register_data_set || this->is_double_read_register_data_set) &&
                           this->is_program_counter_set && this->is_register_destination_set &&
                           this->is_immediate_set && this->is_control_set && this->is_register_source1_set &&
                           this->is_register_source2_set && this->is_instruction_set && !this->is_pause_flag_set &&
                           this->is_nop_flag_set && this->is_nop_passed_flag_set) || this->is_reset_flag_set ||
                           this->isKilled();
                }
        );

        if (this->isKilled()) {
            this->log("Killed.");
            break;
        }

        if (this->is_reset_flag_set) {
            this->log("Resetting.");

            this->resetStage();
            this->is_reset_flag_set = false;

            this->stage_synchronizer->arriveReset();

            this->log("Reset.");
            continue;
        }

        this->log("Woken up and acquired lock.");

        if (!this->control || !this->instruction) {
            this->instruction = new Instruction(std::string(32, '0'));
            this->control = new Control(this->instruction);
        }

        this->control->setNop(this->is_nop_asserted);
        this->control->toggleEXStageControlSignals();

        if (this->is_verbose_execution_flag_asserted) {
            this->printState();
        }

        std::thread pass_program_counter_ex_adder_thread (
                &IDEXStageRegisters::passProgramCounterToEXAdder,
                this,
                this->program_counter
        );

        std::thread pass_program_counter_ex_mux_alu_input_1_thread (
                &IDEXStageRegisters::passProgramCounterToEXMuxALUInput1,
                this,
                this->program_counter
        );

        std::thread pass_read_data_1_ex_mux_alu_input_1_thread (
                &IDEXStageRegisters::passReadData1ToExMuxALUInput1,
                this,
                this->read_data_1
        );

        std::thread pass_read_data_2_ex_mux_alu_input_2_thread (
                &IDEXStageRegisters::passReadData2ToExMuxALUInput2,
                this,
                this->read_data_2
        );

        std::thread pass_immediate_ex_mux_alu_input_2_thread (
                &IDEXStageRegisters::passImmediateToEXMuxALUInput2,
                this,
                this->immediate
        );

        std::thread pass_immediate_ex_adder_thread (
                &IDEXStageRegisters::passImmediateToEXAdder,
                this,
                this->immediate
        );

        std::thread pass_register_source_forwarding_unit_thread (
                &IDEXStageRegisters::passRegisterSourceToForwardingUnit,
                this,
                this->is_single_read_register_data_set,
                this->register_source1,
                this->register_source2
        );

        std::thread pass_mem_read_hazard_detection_unit_thread (
                &IDEXStageRegisters::passMemReadToHazardDetectionUnit,
                this,
                this->control->isMemReadAsserted()
        );

        std::thread pass_register_destination_hazard_detection_unit_thread (
                &IDEXStageRegisters::passRegisterDestinationToHazardDetectionUnit,
                this,
                this->register_destination
        );

        std::thread pass_register_destination_thread (
                &IDEXStageRegisters::passRegisterDestinationToEXMEMStageRegisters,
                this,
                this->register_destination
        );

        std::thread pass_read_data2_thread (
                &IDEXStageRegisters::passReadData2ToEXMEMStageRegisters,
                this,
                this->read_data_2
        );

        std::thread pass_control_thread (
                &IDEXStageRegisters::passControlToEXMEMStageRegisters,
                this,
                Control::deepCopy(this->control)
        );

        std::thread pass_nop_ex_mem_stage_registers_thread (
                &IDEXStageRegisters::passNopToEXMEMStageRegisters,
                this,
                this->is_nop_asserted
        );

        pass_program_counter_ex_adder_thread.detach();
        pass_program_counter_ex_mux_alu_input_1_thread.detach();
        pass_read_data_1_ex_mux_alu_input_1_thread.detach();
        pass_read_data_2_ex_mux_alu_input_2_thread.detach();
        pass_immediate_ex_mux_alu_input_2_thread.detach();
        pass_immediate_ex_adder_thread.detach();
        pass_register_source_forwarding_unit_thread.detach();
        pass_mem_read_hazard_detection_unit_thread.detach();
        pass_register_destination_hazard_detection_unit_thread.detach();
        pass_register_destination_thread.detach();
        pass_read_data2_thread.detach();
        pass_control_thread.detach();
        pass_nop_ex_mem_stage_registers_thread.detach();

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
        this->is_nop_passed_flag_set = false;
        this->is_nop_passed_flag_asserted = false;
        this->current_nop_set_operations = 0;

        this->stage_synchronizer->conditionalArriveSingleStage();
    }
}

void IDEXStageRegisters::setRegisterData(std::bitset<WORD_BIT_COUNT> reg_data1) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    if (this->getPipelineType() == PipelineType::Five) {
        this->delayUpdateUntilNopFlagSet();
    }

    this->log("setRegisterData waiting to acquire lock.");

    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    this->log("setRegisterData acquired lock. Updating value.");

    if (!this->is_nop_asserted) {
        this->read_data_1 = reg_data1;
        this->log("setRegisterData value updated.");
    } else {
        this->log("setRegisterData update skipped. NOP asserted.");
    }

    this->is_single_read_register_data_set = true;
    this->notifyModuleConditionVariable();
}

void IDEXStageRegisters::setRegisterData(std::bitset<WORD_BIT_COUNT> reg_data1, std::bitset<WORD_BIT_COUNT> reg_data2) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    if (this->getPipelineType() == PipelineType::Five) {
        this->delayUpdateUntilNopFlagSet();
    }

    this->log("setRegisterData waiting to acquire lock.");

    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    this->log("setRegisterData acquired lock. Updating value.");

    if (!this->is_nop_asserted) {
        this->read_data_1 = reg_data1;
        this->read_data_2 = reg_data2;
        this->log("setRegisterData value updated.");
    } else {
        this->log("setRegisterData update skipped. NOP asserted.");
    }

    this->is_double_read_register_data_set = true;
    this->notifyModuleConditionVariable();
}

void IDEXStageRegisters::setImmediate(std::bitset<WORD_BIT_COUNT> imm) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    if (this->getPipelineType() == PipelineType::Five) {
        this->delayUpdateUntilNopFlagSet();
    }

    this->log("setImmediate waiting to acquire lock.");

    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    this->log("setImmediate acquired lock. Updating value.");

    if (!this->is_nop_asserted) {
        this->immediate = imm;
        this->log("setImmediate value updated.");
    } else {
        this->log("setImmediate update skipped. NOP asserted.");
    }

    this->is_immediate_set = true;
    this->notifyModuleConditionVariable();
}

void IDEXStageRegisters::setRegisterDestination(unsigned long rd) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    if (this->getPipelineType() == PipelineType::Five) {
        this->delayUpdateUntilNopFlagSet();
    }

    this->log("setRegisterDestination waiting to acquire lock.");

    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    this->log("setRegisterDestination acquired lock. Updating value.");

    if (!this->is_nop_asserted) {
        this->register_destination = rd;
        this->log("setRegisterDestination value updated.");
    } else {
        this->log("setRegisterDestination update skipped. NOP asserted.");
    }

    this->is_register_destination_set = true;
    this->notifyModuleConditionVariable();
}

void IDEXStageRegisters::setProgramCounter(unsigned long pc) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    if (this->getPipelineType() == PipelineType::Five) {
        this->delayUpdateUntilNopFlagSet();
    }

    this->log("setProgramCounter waiting to acquire lock.");

    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    this->log("setProgramCounter acquired lock. Updating value.");

    if (!this->is_nop_asserted) {
        this->program_counter = pc;
        this->log("setProgramCounter value updated.");
    } else {
        this->log("setProgramCounter update skipped. NOP asserted.");
    }

    this->is_program_counter_set = true;
    this->notifyModuleConditionVariable();
}

void IDEXStageRegisters::setControlModule(Control *new_control) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    if (this->getPipelineType() == PipelineType::Five) {
        this->delayUpdateUntilNopFlagSet();
    }

    this->log("setControlModule waiting to acquire lock.");

    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    this->log("setControlModule acquired lock. Updating value.");

    if (this->is_nop_asserted) {
        this->log("setControlModule update skipped. NOP asserted.");
    } else {
        this->control = new_control;
        this->log("setControlModule value updated.");
    }

    this->is_control_set = true;
    this->notifyModuleConditionVariable();
}

void IDEXStageRegisters::setInstruction(Instruction *current_instruction) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    if (this->getPipelineType() == PipelineType::Five) {
        this->delayUpdateUntilNopFlagSet();
    }

    this->log("setInstruction waiting to acquire lock.");

    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    this->log("setInstruction acquired lock. Updating value.");

    if (!this->is_nop_asserted) {
        this->instruction = current_instruction;
        this->log("setInstruction acquired lock. Updating value.");
    } else {
        this->log("setInstruction update skipped. NOP asserted.");
    }

    this->is_instruction_set = true;
    this->notifyModuleConditionVariable();
}

void IDEXStageRegisters::setRegisterSource1(unsigned long rs1) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    if (this->getPipelineType() == PipelineType::Five) {
        this->delayUpdateUntilNopFlagSet();
    }

    this->log("setRegisterSource1 waiting to acquire lock.");

    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    this->log("setRegisterSource1 acquired lock. Updating value.");

    if (!this->is_nop_asserted) {
        this->register_source1 = rs1;
        this->log("setRegisterSource1 value updated.");
    } else {
        this->log("setRegisterSource1 update skipped. NOP asserted.");
    }

    this->is_register_source1_set = true;
    this->notifyModuleConditionVariable();
}

void IDEXStageRegisters::setPassedNop(bool is_asserted) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    if (this->getPipelineType() == PipelineType::Five) {
        this->delayUpdateUntilNopFlagSet();
    }

    this->log("setPassedNop waiting to acquire lock.");

    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    this->log("setPassedNop acquired lock. Updating value.");

    this->is_nop_passed_flag_asserted = is_asserted;
    this->is_nop_passed_flag_set = true;

    this->log("setPassedNop updated value.");
    this->notifyModuleConditionVariable();
}

void IDEXStageRegisters::setNop(bool is_asserted) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    this->log("setPassedNop waiting to acquire lock.");

    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    this->log("setPassedNop acquired lock.");

    this->is_nop_asserted |= is_asserted;

    if (++this->current_nop_set_operations == REQUIRED_NOP_FLAG_SET_OPERATIONS) {
        this->is_nop_flag_set = true;
        this->getModuleConditionVariable().notify_all();
    }
}

void IDEXStageRegisters::setRegisterSource2(unsigned long rs2) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    if (this->getPipelineType() == PipelineType::Five) {
        this->delayUpdateUntilNopFlagSet();
    }

    this->log("setRegisterSource2 waiting to acquire lock.");

    std::lock_guard<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());

    this->log("setRegisterSource2 acquired lock. Updating value.");

    if (!this->is_nop_asserted) {
        this->register_source2 = rs2;
        this->log("setRegisterSource2 value updated.");
    } else {
        this->log("setRegisterSource2 update skipped. NOP asserted.");
    }

    this->is_register_source2_set = true;
    this->notifyModuleConditionVariable();
}

void IDEXStageRegisters::passProgramCounterToEXAdder(unsigned long pc) {
    this->log("Passing program counter to EXAdder.");
    this->ex_adder->setInput(EXAdderInputType::PCValue, pc);
    this->log("Passed program counter to EXAdder.");
}

void IDEXStageRegisters::passProgramCounterToEXMuxALUInput1(unsigned long pc) {
    this->log("Passing program counter to EXMuxALUInput1.");
    this->ex_mux_alu_input_1->setInput(EXStageMuxALUInput1InputType::ProgramCounter, pc);
    this->log("Passed program counter to EXMuxALUInput1.");
}

void IDEXStageRegisters::passReadData1ToExMuxALUInput1(std::bitset<WORD_BIT_COUNT> data) {
    this->log("Passing read data 1 to EXMuxALUInput1.");
    this->ex_mux_alu_input_1->setInput(EXStageMuxALUInput1InputType::ReadData1, data);
    this->log("Passed read data 1 to EXMuxALUInput1.");
}

void IDEXStageRegisters::passReadData2ToExMuxALUInput2(std::bitset<WORD_BIT_COUNT> data) {
    this->log("Passing read data 2 to EXMuxALUInput2.");
    this->ex_mux_alu_input_2->setInput(EXStageMuxALUInput2InputType::ReadData2, data);
    this->log("Passed read data 2 to EXMuxALUInput2.");
}

void IDEXStageRegisters::passImmediateToEXMuxALUInput2(std::bitset<WORD_BIT_COUNT> imm) {
    this->log("Passing immediate to EXMuxALUInput2.");
    this->ex_mux_alu_input_2->setInput(EXStageMuxALUInput2InputType::ImmediateValue, imm);
    this->log("Passed immediate to EXMuxALUInput2.");
}

void IDEXStageRegisters::passImmediateToEXAdder(std::bitset<WORD_BIT_COUNT> imm) {
    this->log("Passing immediate to EXAdder.");
    this->ex_adder->setInput(EXAdderInputType::ImmediateValue, imm);
    this->log("Passed immediate to EXAdder.");
}

void IDEXStageRegisters::passRegisterDestinationToEXMEMStageRegisters(unsigned long rd) {
    this->log("Passing register destination to EXMEMStageRegisters.");
    this->ex_mem_stage_register->setRegisterDestination(rd);
    this->log("Passed register destination to EXMEMStageRegisters.");
}

void IDEXStageRegisters::passReadData2ToEXMEMStageRegisters(std::bitset<WORD_BIT_COUNT> data) {
    this->log("Passing read data 2 to EXMEMStageRegisters.");
    this->ex_mem_stage_register->setReadData2(data);
    this->log("Passed read data 2 to EXMEMStageRegisters.");
}

void IDEXStageRegisters::passControlToEXMEMStageRegisters(Control *current_control) {
    this->log("Passing control to EXMEMStageRegisters.");
    this->ex_mem_stage_register->setControl(current_control);
    this->log("Passed control to EXMEMStageRegisters.");
}

void IDEXStageRegisters::assertSystemEnabledNop() {
    this->is_nop_asserted = true;
}

void IDEXStageRegisters::passRegisterSourceToForwardingUnit(bool is_single_register_used,
                                                            unsigned long rs1,
                                                            unsigned long rs2) {
    this->log("Passing register source to Forwarding Unit.");

    if (is_single_register_used) {
        this->forwarding_unit->setSingleRegisterSource(rs1);
    } else {
        this->forwarding_unit->setDoubleRegisterSource(rs1, rs2);
    }

    this->log("Passed register source to Forwarding Unit.");
}

void IDEXStageRegisters::passNopToEXMEMStageRegisters(bool is_signal_asserted) {
    this->log("Passing NOP to EXMEMStageRegisters.");
    this->ex_mem_stage_register->setPassedNop(is_signal_asserted);
    this->log("Passed NOP to EXMEMStageRegisters.");
}

void IDEXStageRegisters::passMemReadToHazardDetectionUnit(bool is_signal_asserted) {
    this->log("Passing MemRead to HazardDetectionUnit.");
    this->hazard_detection_unit->setIDEXMemRead(is_signal_asserted);
    this->log("Passed MemRead to HazardDetectionUnit.");
}

void IDEXStageRegisters::passRegisterDestinationToHazardDetectionUnit(unsigned long rd) {
    this->log("Passing register destination to HazardDetectionUnit.");
    this->hazard_detection_unit->setIDEXRegisterDestination(rd);
    this->log("Passed register destination to HazardDetectionUnit.");
}

void IDEXStageRegisters::delayUpdateUntilNopFlagSet() {
    std::unique_lock<std::mutex> id_ex_stage_registers_lock (this->getModuleMutex());
    this->getModuleConditionVariable().wait(
            id_ex_stage_registers_lock,
            [this] {
                return this->is_nop_flag_set;
            }
    );
}

std::string IDEXStageRegisters::getModuleTag() {
    return "IDEXStageRegisters";
}

Stage IDEXStageRegisters::getModuleStage() {
    return Stage::ID;
}

void IDEXStageRegisters::printState() {
    std::cout << std::string(20, '.') << std::endl;
    std::cout << "IDEXStageRegisters" << std::endl;
    std::cout << std::string(20, '.') << std::endl;

    std::cout << "read_data_1: " << this->read_data_1.to_ulong() << std::endl;
    std::cout << "read_data_2: " << this->read_data_2.to_ulong() << std::endl;
    std::cout << "immediate: " << this->immediate.to_ulong() << std::endl;
    std::cout << "register_source1: " << this->register_source1 << std::endl;
    std::cout << "register_source2: " << this->register_source2 << std::endl;
    std::cout << "register_destination: " << this->register_destination << std::endl;
    std::cout << "program_counter: " << this->program_counter << std::endl;
    std::cout << "is_single_read_register_data_set: " << this->is_single_read_register_data_set << std::endl;
    std::cout << "is_double_read_register_data_set: " << this->is_double_read_register_data_set << std::endl;
    std::cout << "is_immediate_set: " << this->is_immediate_set << std::endl;
    std::cout << "is_register_destination_set: " << this->is_register_destination_set << std::endl;
    std::cout << "is_register_source1_set: " << this->is_register_source1_set << std::endl;
    std::cout << "is_register_source2_set: " << this->is_register_source2_set << std::endl;
    std::cout << "is_program_counter_set: " << this->is_program_counter_set << std::endl;
    std::cout << "is_control_set: " << this->is_control_set << std::endl;
    std::cout << "is_instruction_set: " << this->is_instruction_set << std::endl;
    std::cout << "is_nop_passed_flag_set: " << this->is_nop_passed_flag_set << std::endl;
    std::cout << "is_nop_asserted: " << this->is_nop_asserted << std::endl;
    std::cout << "is_reset_flag_set: " << this->is_reset_flag_set << std::endl;
    std::cout << "is_pause_flag_set: " << this->is_pause_flag_set << std::endl;
    std::cout << "is_nop_passed_flag_asserted: " << this->is_nop_passed_flag_asserted << std::endl;

    this->control->printState();
}

void IDEXStageRegisters::assertVerboseExecutionFlag() {
    this->is_verbose_execution_flag_asserted = true;
}