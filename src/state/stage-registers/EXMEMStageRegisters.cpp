#include "../../../include/state/stage-registers/EXMEMStageRegisters.h"

EXMEMStageRegisters *EXMEMStageRegisters::current_instance = nullptr;
std::mutex EXMEMStageRegisters::initialization_mutex;

EXMEMStageRegisters::EXMEMStageRegisters() {
    this->branch_program_counter = 0UL;
    this->register_destination = 0UL;

    this->alu_result = std::bitset<WORD_BIT_COUNT>(std::string(32, '0'));
    this->read_data_2 = std::bitset<WORD_BIT_COUNT>(std::string(32, '0'));

    this->is_alu_result_zero = false;

    this->is_branch_program_counter_set = false;
    this->is_alu_result_set = false;
    this->is_read_data_2_set = false;
    this->is_register_destination_set = false;
    this->is_alu_result_zero_flag_set = false;
    this->is_control_set = false;
    this->is_nop_flag_asserted = false;
    this->is_reset_flag_set = false;
    this->is_pause_flag_set = false;
    this->is_nop_passed_flag_asserted = false;
    this->is_nop_passed_flag_set = false;
    this->is_verbose_execution_flag_asserted = false;

    this->control = nullptr;

    this->data_memory = nullptr;
    this->mem_wb_stage_registers = nullptr;
    this->if_mux = nullptr;
    this->stage_synchronizer = nullptr;
    this->alu_input_1_forwarding_mux = nullptr;
    this->alu_input_2_forwarding_mux = nullptr;
    this->forwarding_unit = nullptr;
    this->logger = nullptr;
}

void EXMEMStageRegisters::changeStageAndReset(PipelineType new_pipeline_type) {
    {  // Limit lock guard scope to avoid deadlock
        std::lock_guard<std::mutex> if_id_stage_registers_lock(this->getModuleMutex());

        this->log("PipelineType change.");
        this->setPipelineType(new_pipeline_type);
    }

    this->reset();
}

void EXMEMStageRegisters::reset() {
    std::lock_guard<std::mutex> if_id_stage_registers_lock(this->getModuleMutex());

    this->is_reset_flag_set = true;
    this->notifyModuleConditionVariable();
}

void EXMEMStageRegisters::resetStage() {
    if (this->getPipelineType() == PipelineType::Single) {
        this->is_branch_program_counter_set = false;
        this->is_alu_result_set = false;
        this->is_read_data_2_set = false;
        this->is_register_destination_set = false;
        this->is_alu_result_zero_flag_set = false;
        this->is_control_set = false;
        this->is_nop_passed_flag_set = false;
    } else {
        this->is_branch_program_counter_set = true;
        this->is_alu_result_set = true;
        this->is_read_data_2_set = true;
        this->is_register_destination_set = true;
        this->is_alu_result_zero_flag_set = true;
        this->is_control_set = true;
        this->is_nop_passed_flag_asserted = true;
        this->is_nop_passed_flag_set = true;
    }

    this->is_alu_result_zero = false;
    this->branch_program_counter = 0UL;
    this->register_destination = 0UL;

    this->alu_result = std::bitset<WORD_BIT_COUNT>(std::string(32, '0'));
    this->read_data_2 = std::bitset<WORD_BIT_COUNT>(std::string(32, '0'));

    this->is_nop_flag_asserted = false;
    this->is_reset_flag_set = false;

    this->control = new Control(new Instruction(std::string(32, '0')));
}

void EXMEMStageRegisters::pause() {
    this->log("Paused.");
    this->is_pause_flag_set = true;
}

void EXMEMStageRegisters::resume() {
    std::lock_guard<std::mutex> ex_mem_stage_registers_lock (this->getModuleMutex());

    this->log("Resumed.");
    this->is_pause_flag_set = false;
    this->notifyModuleConditionVariable();
}

EXMEMStageRegisters *EXMEMStageRegisters::init() {
    std::lock_guard<std::mutex> ex_mem_stage_registers_lock (EXMEMStageRegisters::initialization_mutex);

    if (EXMEMStageRegisters::current_instance == nullptr) {
        EXMEMStageRegisters::current_instance = new EXMEMStageRegisters();
    }

    return EXMEMStageRegisters::current_instance;
}

void EXMEMStageRegisters::initDependencies() {
    std::unique_lock<std::mutex> ex_mem_stage_registers_lock (this->getModuleDependencyMutex());

    if (this->control && this->data_memory && this->mem_wb_stage_registers && this->if_mux &&
        this->stage_synchronizer && this->alu_input_1_forwarding_mux && this->alu_input_2_forwarding_mux &&
        this->forwarding_unit && this->logger) {
        return;
    }

    this->control = new Control(new Instruction(std::string(32, '0')));

    this->data_memory = DataMemory::init();
    this->mem_wb_stage_registers = MEMWBStageRegisters::init();
    this->if_mux = IFMux::init();
    this->stage_synchronizer = StageSynchronizer::init();
    this->alu_input_1_forwarding_mux = ALUInput1ForwardingMux::init();
    this->alu_input_2_forwarding_mux = ALUInput2ForwardingMux::init();
    this->forwarding_unit = ForwardingUnit::init();
    this->logger = Logger::init();
}

void EXMEMStageRegisters::run() {
    this->initDependencies();

    while (this->isAlive()) {
        this->log("Waiting to be woken up and acquire lock.");

        std::unique_lock<std::mutex> ex_mem_stage_registers_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                ex_mem_stage_registers_lock,
                [this] {
                    return ((this->is_branch_program_counter_set && this->is_alu_result_set &&
                            this->is_read_data_2_set && this->is_register_destination_set &&
                            this->is_alu_result_zero_flag_set && this->is_control_set && this->is_nop_passed_flag_set) &&
                            !this->is_pause_flag_set) || this->is_reset_flag_set || this->isKilled();
                }
        );

        if (this->isKilled()) {
            this->log("Killed.");
            break;
        }

        if (this->is_reset_flag_set) {
            this->log("Resetting stage.");

            this->resetStage();
            this->is_reset_flag_set = false;

            this->stage_synchronizer->arriveReset();

            this->log("Reset.");
            continue;
        }

        this->log("Woken up and acquired lock.");

        this->control->setNop(this->is_nop_passed_flag_asserted);
        this->control->setIsALUResultZero(this->is_alu_result_zero);
        this->control->toggleMEMStageControlSignals();

        if (this->is_verbose_execution_flag_asserted) {
            this->printState();
        }

        std::thread pass_write_data_data_memory_thread (
                &EXMEMStageRegisters::passWriteDataToDataMemory,
                this,
                this->read_data_2
        );

        std::thread pass_alu_result_data_memory_thread (
                &EXMEMStageRegisters::passALUResultToDataMemory,
                this,
                this->alu_result
        );

        std::thread pass_branched_address_if_mux_thread (
                &EXMEMStageRegisters::passBranchedAddressToIFMux,
                this,
                this->branch_program_counter
        );

        std::thread pass_alu_result_alu_input_1_forwarding_mux_thread (
                &EXMEMStageRegisters::passALUResultToALUInput1ForwardingMux,
                this,
                this->alu_result
        );

        std::thread pass_alu_result_alu_input_2_forwarding_mux_thread (
                &EXMEMStageRegisters::passALUResultToALUInput2ForwardingMux,
                this,
                this->alu_result
        );

        std::thread pass_register_destination_forwarding_unit_thread (
                &EXMEMStageRegisters::passRegisterDestinationToForwardingUnit,
                this,
                this->register_destination
        );

        std::thread pass_reg_write_forwarding_unit_thread (
                &EXMEMStageRegisters::passRegWriteToForwardingUnit,
                this,
                this->control->isRegWriteAsserted()
        );

        std::thread pass_alu_result_thread (
                &EXMEMStageRegisters::passALUResultToMEMWBStageRegisters,
                this,
                this->alu_result
        );
        std::thread pass_register_destination_thread (
                &EXMEMStageRegisters::passRegisterDestinationToMEMWBStageRegisters,
                this,
                this->register_destination
        );

        std::thread pass_control_thread (
                &EXMEMStageRegisters::passControlToMEMWBStageRegisters,
                this,
                Control::deepCopy(this->control)
        );

        std::thread pass_nop_mem_wb_stage_registers_thread (
                &EXMEMStageRegisters::passNopToMEMWBStageRegisters,
                this,
                this->is_nop_passed_flag_asserted
        );

        pass_write_data_data_memory_thread.detach();
        pass_alu_result_data_memory_thread.detach();
        pass_branched_address_if_mux_thread.detach();
        pass_alu_result_alu_input_1_forwarding_mux_thread.detach();
        pass_alu_result_alu_input_2_forwarding_mux_thread.detach();
        pass_register_destination_forwarding_unit_thread.detach();
        pass_reg_write_forwarding_unit_thread.detach();
        pass_alu_result_thread.detach();
        pass_register_destination_thread.detach();
        pass_control_thread.detach();
        pass_nop_mem_wb_stage_registers_thread.detach();

        this->is_branch_program_counter_set = false;
        this->is_alu_result_set = false;
        this->is_read_data_2_set = false;
        this->is_register_destination_set = false;
        this->is_alu_result_zero_flag_set = false;
        this->is_control_set = false;
        this->is_nop_flag_asserted = false;
        this->is_nop_passed_flag_set = false;
        this->is_nop_passed_flag_asserted = false;

        this->stage_synchronizer->conditionalArriveSingleStage();
    }
}

void EXMEMStageRegisters::setBranchedProgramCounter(unsigned long value) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    this->log("setBranchedProgramCounter waiting to acquire lock.");

    std::lock_guard<std::mutex> ex_mem_stage_registers_lock (this->getModuleMutex());

    this->log("setBranchedProgramCounter acquired lock. Updating value.");

    if (!this->is_nop_flag_asserted) {
        this->branch_program_counter = value;
        this->log("setBranchedProgramCounter updated value.");
    } else {
        this->log("setBranchedProgramCounter update skipped. NOP asserted.");
    }

    this->is_branch_program_counter_set = true;
    this->notifyModuleConditionVariable();
}

void EXMEMStageRegisters::setALUResult(std::bitset<WORD_BIT_COUNT> value) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    this->log("setALUResult waiting to acquire lock.");

    std::lock_guard<std::mutex> ex_mem_stage_registers_lock (this->getModuleMutex());

    this->log("setALUResult acquired lock. Updating value.");

    if (!this->is_nop_flag_asserted) {
        this->alu_result = value;
        this->log("setALUResult updated value.");
    } else {
        this->log("setALUResult update skipped. NOP asserted.");
    }

    this->is_alu_result_set = true;
    this->notifyModuleConditionVariable();
}

void EXMEMStageRegisters::setIsResultZeroFlag(bool asserted) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    this->log("setIsResultZeroFlag waiting to acquire lock.");

    std::lock_guard<std::mutex> ex_mem_stage_registers_lock (this->getModuleMutex());

    this->log("setIsResultZeroFlag acquired lock. Updating value.");

    if (!this->is_nop_flag_asserted) {
        this->is_alu_result_zero = asserted;
        this->log("setIsResultZeroFlag updated value.");
    } else {
        this->log("setIsResultZeroFlag update skipped. NOP asserted.");
    }

    this->is_alu_result_zero_flag_set = true;
    this->notifyModuleConditionVariable();
}

void EXMEMStageRegisters::setReadData2(std::bitset<WORD_BIT_COUNT> value) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    this->log("setReadData2 waiting to acquire lock.");

    std::lock_guard<std::mutex> ex_mem_stage_registers_lock (this->getModuleMutex());

    this->log("setReadData2 acquired lock. Updating value.");

    if (!this->is_nop_flag_asserted) {
        this->read_data_2 = value;
        this->log("setReadData2 updated value.");
    } else {
        this->log("setReadData2 update skipped. NOP asserted.");
    }

    this->is_read_data_2_set = true;
    this->notifyModuleConditionVariable();
}

void EXMEMStageRegisters::setRegisterDestination(unsigned long value) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    this->log("setRegisterDestination waiting to acquire lock.");

    std::lock_guard<std::mutex> ex_mem_stage_registers_lock (this->getModuleMutex());

    this->log("setRegisterDestination acquired lock. Updating value.");

    if (!this->is_nop_flag_asserted) {
        this->register_destination = value;
        this->log("setRegisterDestination updated value.");
    } else {
        this->log("setRegisterDestination update skipped. NOP asserted.");
    }

    this->is_register_destination_set = true;
    this->notifyModuleConditionVariable();
}

void EXMEMStageRegisters::setControl(Control *new_control) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    this->log("setControl waiting to acquire lock.");

    std::lock_guard<std::mutex> ex_mem_stage_registers_lock (this->getModuleMutex());

    this->log("setControl acquired lock. Updating value.");

    if (this->is_nop_flag_asserted) {
        this->log("setControl update skipped. NOP asserted.");
    } else {
        this->control = new_control;
        this->log("setControl updated value.");
    }

    this->is_control_set = true;
    this->notifyModuleConditionVariable();
}

void EXMEMStageRegisters::setPassedNop(bool is_asserted) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    this->log("setPassedNop waiting to acquire lock.");

    std::lock_guard<std::mutex> ex_mem_stage_registers_lock (this->getModuleMutex());

    this->log("setPassedNop acquired lock.");

    this->is_nop_passed_flag_asserted = is_asserted;
    this->is_nop_passed_flag_set = true;

    this->log("setPassedNop updated value.");
    this->notifyModuleConditionVariable();
}

void EXMEMStageRegisters::passControlToMEMWBStageRegisters(Control *current_control) {
    this->log("Passing ALU result to Data Memory.");
    this->mem_wb_stage_registers->setControl(current_control);
    this->log("Passed ALU result to Data Memory.");
}

void EXMEMStageRegisters::passALUResultToDataMemory(std::bitset<WORD_BIT_COUNT> data) {
    this->log("Passing ALU result to Data Memory.");
    this->data_memory->setAddress(data.to_ulong());
    this->log("Passed ALU result to Data Memory.");
}

void EXMEMStageRegisters::passWriteDataToDataMemory(std::bitset<WORD_BIT_COUNT> data) {
    this->log("Passing write data to Data Memory.");
    this->data_memory->setWriteData(data);
    this->log("Passed write data to Data Memory.");
}

void EXMEMStageRegisters::passALUResultToMEMWBStageRegisters(std::bitset<WORD_BIT_COUNT> data) {
    this->log("Passing ALU result to MEMWBStageRegisters.");
    this->mem_wb_stage_registers->setALUResult(data);
    this->log("Passed ALU result to MEMWBStageRegisters.");
}

void EXMEMStageRegisters::passRegisterDestinationToMEMWBStageRegisters(unsigned long rd) {
    this->log("Passing register destination to MEMWBStageRegisters.");
    this->mem_wb_stage_registers->setRegisterDestination(rd);
    this->log("Passed register destination to MEMWBStageRegisters.");
}

void EXMEMStageRegisters::passBranchedAddressToIFMux(unsigned long branched_address) {
    this->log("Passing branch address to IFMux.");
    this->if_mux->setInput(IFStageMuxInputType::BranchedPc, branched_address);
    this->log("Passed branch address to IFMux.");
}

void EXMEMStageRegisters::assertSystemEnabledNop() {
    this->is_nop_flag_asserted = true;
}

void EXMEMStageRegisters::passALUResultToALUInput1ForwardingMux(std::bitset<WORD_BIT_COUNT> data) {
    this->log("Passing ALU result to ALUInput1ForwardingMux.");
    this->alu_input_1_forwarding_mux->setInput(ALUInputMuxInputTypes::EXMEMStageRegisters, data);
    this->log("Passed ALU result to ALUInput1ForwardingMux.");
}

void EXMEMStageRegisters::passALUResultToALUInput2ForwardingMux(std::bitset<WORD_BIT_COUNT> data) {
    this->log("Passing ALU result to ALUInput2ForwardingMux.");
    this->alu_input_2_forwarding_mux->setInput(ALUInputMuxInputTypes::EXMEMStageRegisters, data);
    this->log("Passed ALU result to ALUInput2ForwardingMux.");
}

void EXMEMStageRegisters::passRegisterDestinationToForwardingUnit(unsigned long rd) {
    this->log("Passing register destination to ForwardingUnit.");
    this->forwarding_unit->setEXMEMStageRegisterDestination(rd);
    this->log("Passed register destination to ForwardingUnit.");
}

void EXMEMStageRegisters::passRegWriteToForwardingUnit(bool is_signal_asserted) {
    this->log("Passing reg write to ForwardingUnit.");
    this->forwarding_unit->setEXMEMStageRegisterRegWrite(is_signal_asserted);
    this->log("Passed reg write to ForwardingUnit.");
}

void EXMEMStageRegisters::passNopToMEMWBStageRegisters(bool is_signal_asserted) {
    this->log("Passing NOP to MEMWBStageRegisters.");
    this->mem_wb_stage_registers->setPassedNop(is_signal_asserted);
    this->log("Passed NOP to MEMWBStageRegisters.");
}

std::string EXMEMStageRegisters::getModuleTag() {
    return "EXMEMStageRegisters";
}

Stage EXMEMStageRegisters::getModuleStage() {
    return Stage::EX;
}

void EXMEMStageRegisters::printState() {
    std::cout << std::string(20, '.') << std::endl;
    std::cout << "EXMEMStageRegisters" << std::endl;
    std::cout << std::string(20, '.') << std::endl;

    std::cout << "branch_program_counter: " << this->branch_program_counter << std::endl;
    std::cout << "register_destination: " << this->register_destination << std::endl;
    std::cout << "alu_result: " << this->alu_result.to_ulong() << std::endl;
    std::cout << "read_data_2: " << this->alu_result.to_ulong() << std::endl;
    std::cout << "is_alu_result_zero: " << this->is_alu_result_zero << std::endl;
    std::cout << "is_branch_program_counter_set: " << this->is_branch_program_counter_set << std::endl;
    std::cout << "is_alu_result_set: " << this->is_alu_result_set << std::endl;
    std::cout << "is_read_data_2_set: " << this->is_read_data_2_set << std::endl;
    std::cout << "is_register_destination_set: " << this->is_register_destination_set << std::endl;
    std::cout << "is_alu_result_zero_flag_set: " << this->is_alu_result_zero_flag_set << std::endl;
    std::cout << "is_control_set: " << this->is_control_set << std::endl;
    std::cout << "is_nop_flag_asserted: " << this->is_nop_flag_asserted << std::endl;
    std::cout << "is_nop_passed_flag_asserted: " << this->is_nop_passed_flag_asserted << std::endl;
    std::cout << "is_reset_flag_set: " << this->is_reset_flag_set << std::endl;
    std::cout << "is_pause_flag_set: " << this->is_pause_flag_set << std::endl;
    std::cout << "is_nop_passed_flag_set: " << this->is_nop_passed_flag_set << std::endl;

    this->control->printState();
}

void EXMEMStageRegisters::assertVerboseExecutionFlag() {
    this->is_verbose_execution_flag_asserted = true;
}