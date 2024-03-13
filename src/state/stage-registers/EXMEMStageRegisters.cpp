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
    this->is_nop_asserted = false;
    this->is_reset_flag_set = false;
    this->is_pause_flag_set = false;

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

        this->logger->log(Stage::EX, "[EXMEMStageRegisters] PipelineType change.");
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
        this->is_alu_result_zero = false;
        this->is_branch_program_counter_set = false;
        this->is_alu_result_set = false;
        this->is_read_data_2_set = false;
        this->is_register_destination_set = false;
        this->is_alu_result_zero_flag_set = false;
        this->is_control_set = false;
    } else {
        this->is_alu_result_zero = false;
        this->is_branch_program_counter_set = true;
        this->is_alu_result_set = true;
        this->is_read_data_2_set = true;
        this->is_register_destination_set = true;
        this->is_alu_result_zero_flag_set = true;
        this->is_control_set = true;
    }

    this->branch_program_counter = 0UL;
    this->alu_result = 0UL;
    this->read_data_2 = 0UL;
    this->register_destination = 0UL;

    this->is_nop_asserted = false;
    this->is_reset_flag_set = false;

    this->control = new Control(new Instruction(std::string(32, '0')));
}

void EXMEMStageRegisters::pause() {
    this->logger->log(Stage::EX, "[EXMEMStageRegisters] Paused.");
    this->is_pause_flag_set = true;
}

void EXMEMStageRegisters::resume() {
    this->logger->log(Stage::EX, "[EXMEMStageRegisters] Resumed.");
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
        this->logger->log(Stage::EX, "[EXMEMStageRegisters] Waiting to be woken up and acquire lock.");

        std::unique_lock<std::mutex> ex_mem_stage_registers_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                ex_mem_stage_registers_lock,
                [this] {
                    return ((this->is_branch_program_counter_set && this->is_alu_result_set &&
                            this->is_read_data_2_set && this->is_register_destination_set &&
                            this->is_alu_result_zero_flag_set && this->is_control_set) && !this->is_pause_flag_set) ||
                            this->is_reset_flag_set || this->isKilled();
                }
        );

        if (this->isKilled()) {
            this->logger->log(Stage::EX, "[EXMEMStageRegisters] Killed.");
            break;
        }

        if (this->is_reset_flag_set) {
            this->logger->log(Stage::EX, "[EXMEMStageRegisters] Resetting stage.");

            this->resetStage();
            this->is_reset_flag_set = false;

            this->logger->log(Stage::EX, "[EXMEMStageRegisters] Reset.");
            continue;
        }

        this->logger->log(Stage::EX, "[EXMEMStageRegisters] Woken up and acquired lock.");

        this->control->setIsALUResultZero(this->is_alu_result_zero);
        this->control->toggleMEMStageControlSignals();

        this->passWriteDataToDataMemory();
        this->passALUResultToDataMemory();
        this->passBranchedAddressToIFMux();
        this->passALUResultToALUInput1ForwardingMux();
        this->passALUResultToALUInput2ForwardingMux();
        this->passRegisterDestinationToForwardingUnit();

        std::thread pass_alu_result_thread (&EXMEMStageRegisters::passALUResultToMEMWBStageRegisters, this);
        std::thread pass_register_destination_thread (&EXMEMStageRegisters::passRegisterDestinationToMEMWBStageRegisters, this);
        std::thread pass_control_thread (&EXMEMStageRegisters::passControlToMEMWBStageRegisters, this);

        pass_alu_result_thread.join();
        pass_register_destination_thread.join();
        pass_control_thread.join();

        this->is_branch_program_counter_set = false;
        this->is_alu_result_set = false;
        this->is_read_data_2_set = false;
        this->is_register_destination_set = false;
        this->is_alu_result_zero_flag_set = false;
        this->is_control_set = false;
        this->is_nop_asserted = false;

        this->stage_synchronizer->conditionalArriveSingleStage();
    }
}

void EXMEMStageRegisters::setBranchedProgramCounter(unsigned long value) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    this->logger->log(Stage::EX, "[EXMEMStageRegisters] setBranchedProgramCounter waiting to acquire lock.");

    std::lock_guard<std::mutex> ex_mem_stage_registers_lock (this->getModuleMutex());

    this->logger->log(Stage::EX, "[EXMEMStageRegisters] setBranchedProgramCounter acquired lock. Updating value.");

    if (!this->is_nop_asserted) {
        this->branch_program_counter = value;
        this->logger->log(Stage::EX, "[EXMEMStageRegisters] setBranchedProgramCounter updated value.");
    } else {
        this->logger->log(Stage::EX, "[EXMEMStageRegisters] setBranchedProgramCounter update skipped. NOP asserted.");
    }

    this->is_branch_program_counter_set = true;
    this->notifyModuleConditionVariable();
}

void EXMEMStageRegisters::setALUResult(const std::bitset<WORD_BIT_COUNT> &value) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    this->logger->log(Stage::EX, "[EXMEMStageRegisters] setALUResult waiting to acquire lock.");

    std::lock_guard<std::mutex> ex_mem_stage_registers_lock (this->getModuleMutex());

    this->logger->log(Stage::EX, "[EXMEMStageRegisters] setALUResult acquired lock. Updating value.");

    if (!this->is_nop_asserted) {
        this->alu_result = value;
        this->logger->log(Stage::EX, "[EXMEMStageRegisters] setALUResult updated value.");
    } else {
        this->logger->log(Stage::EX, "[EXMEMStageRegisters] setALUResult update skipped. NOP asserted.");
    }

    this->is_alu_result_set = true;
    this->notifyModuleConditionVariable();
}

void EXMEMStageRegisters::setIsResultZeroFlag(bool asserted) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    this->logger->log(Stage::EX, "[EXMEMStageRegisters] setIsResultZeroFlag waiting to acquire lock.");

    std::lock_guard<std::mutex> ex_mem_stage_registers_lock (this->getModuleMutex());

    this->logger->log(Stage::EX, "[EXMEMStageRegisters] setIsResultZeroFlag acquired lock. Updating value.");

    if (!this->is_nop_asserted) {
        this->is_alu_result_zero = asserted;
        this->logger->log(Stage::EX, "[EXMEMStageRegisters] setIsResultZeroFlag updated value.");
    } else {
        this->logger->log(Stage::EX, "[EXMEMStageRegisters] setIsResultZeroFlag update skipped. NOP asserted.");
    }

    this->is_alu_result_zero_flag_set = true;
    this->notifyModuleConditionVariable();
}

void EXMEMStageRegisters::setReadData2(const std::bitset<WORD_BIT_COUNT> &value) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    this->logger->log(Stage::EX, "[EXMEMStageRegisters] setReadData2 waiting to acquire lock.");

    std::lock_guard<std::mutex> ex_mem_stage_registers_lock (this->getModuleMutex());

    this->logger->log(Stage::EX, "[EXMEMStageRegisters] setReadData2 acquired lock. Updating value.");

    if (!this->is_nop_asserted) {
        this->read_data_2 = value;
        this->logger->log(Stage::EX, "[EXMEMStageRegisters] setReadData2 updated value.");
    } else {
        this->logger->log(Stage::EX, "[EXMEMStageRegisters] setReadData2 update skipped. NOP asserted.");
    }

    this->is_read_data_2_set = true;
    this->notifyModuleConditionVariable();
}

void EXMEMStageRegisters::setRegisterDestination(unsigned long value) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    this->logger->log(Stage::EX, "[EXMEMStageRegisters] setRegisterDestination waiting to acquire lock.");

    std::lock_guard<std::mutex> ex_mem_stage_registers_lock (this->getModuleMutex());

    this->logger->log(Stage::EX, "[EXMEMStageRegisters] setRegisterDestination acquired lock. Updating value.");

    if (!this->is_nop_asserted) {
        this->register_destination = value;
        this->logger->log(Stage::EX, "[EXMEMStageRegisters] setRegisterDestination updated value.");
    } else {
        this->logger->log(Stage::EX, "[EXMEMStageRegisters] setRegisterDestination update skipped. NOP asserted.");
    }

    this->is_register_destination_set = true;
    this->notifyModuleConditionVariable();
}

void EXMEMStageRegisters::setControl(Control *new_control) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    this->logger->log(Stage::EX, "[EXMEMStageRegisters] setControl waiting to acquire lock.");

    std::lock_guard<std::mutex> ex_mem_stage_registers_lock (this->getModuleMutex());

    this->logger->log(Stage::EX, "[EXMEMStageRegisters] setControl acquired lock. Updating value.");

    if (this->is_nop_asserted) {
        this->control = new Control(new Instruction(std::string(32, '0')));
        this->logger->log(Stage::EX, "[EXMEMStageRegisters] setControl update skipped. NOP asserted.");
    } else {
        this->control = new_control;
        this->logger->log(Stage::EX, "[EXMEMStageRegisters] setControl updated value.");
    }

    this->is_control_set = true;
    this->notifyModuleConditionVariable();
}

void EXMEMStageRegisters::passControlToMEMWBStageRegisters() {
    this->logger->log(Stage::EX, "[EXMEMStageRegisters] Passing ALU result to Data Memory.");
    this->mem_wb_stage_registers->setControl(this->control);
    this->logger->log(Stage::EX, "[EXMEMStageRegisters] Passed ALU result to Data Memory.");
}

void EXMEMStageRegisters::passALUResultToDataMemory() {
    this->logger->log(Stage::EX, "[EXMEMStageRegisters] Passing ALU result to Data Memory.");
    this->data_memory->setAddress(this->alu_result.to_ulong());
    this->logger->log(Stage::EX, "[EXMEMStageRegisters] Passed ALU result to Data Memory.");
}

void EXMEMStageRegisters::passWriteDataToDataMemory() {
    this->logger->log(Stage::EX, "[EXMEMStageRegisters] Passing write data to Data Memory.");
    this->data_memory->setWriteData(this->read_data_2);
    this->logger->log(Stage::EX, "[EXMEMStageRegisters] Passed write data to Data Memory.");
}

void EXMEMStageRegisters::passALUResultToMEMWBStageRegisters() {
    this->logger->log(Stage::EX, "[EXMEMStageRegisters] Passing ALU result to MEMWBStageRegisters.");
    this->mem_wb_stage_registers->setALUResult(this->alu_result);
    this->logger->log(Stage::EX, "[EXMEMStageRegisters] Passed ALU result to MEMWBStageRegisters.");
}

void EXMEMStageRegisters::passRegisterDestinationToMEMWBStageRegisters() {
    this->logger->log(Stage::EX, "[EXMEMStageRegisters] Passing register destination to MEMWBStageRegisters.");
    this->mem_wb_stage_registers->setRegisterDestination(this->register_destination);
    this->logger->log(Stage::EX, "[EXMEMStageRegisters] Passed register destination to MEMWBStageRegisters.");
}

void EXMEMStageRegisters::passBranchedAddressToIFMux() {
    this->logger->log(Stage::EX, "[EXMEMStageRegisters] Passing branch address to IFMux.");
    this->if_mux->setInput(IFStageMuxInputType::BranchedPc, this->branch_program_counter);
    this->logger->log(Stage::EX, "[EXMEMStageRegisters] Passed branch address to IFMux.");
}

void EXMEMStageRegisters::assertNop() {
    this->logger->log(Stage::EX, "[EXMEMStageRegisters] NOP asserted.");
    this->is_nop_asserted = true;
}

void EXMEMStageRegisters::passALUResultToALUInput1ForwardingMux() {
    this->logger->log(Stage::EX, "[EXMEMStageRegisters] Passing ALU result to ALUInput1ForwardingMux.");
    this->alu_input_1_forwarding_mux->setInput(ALUInputMuxInputTypes::EXMEMStageRegisters, this->alu_result);
    this->logger->log(Stage::EX, "[EXMEMStageRegisters] Passed ALU result to ALUInput1ForwardingMux.");
}

void EXMEMStageRegisters::passALUResultToALUInput2ForwardingMux() {
    this->logger->log(Stage::EX, "[EXMEMStageRegisters] Passing ALU result to ALUInput2ForwardingMux.");
    this->alu_input_2_forwarding_mux->setInput(ALUInputMuxInputTypes::EXMEMStageRegisters, this->alu_result);
    this->logger->log(Stage::EX, "[EXMEMStageRegisters] Passed ALU result to ALUInput2ForwardingMux.");
}

void EXMEMStageRegisters::passRegisterDestinationToForwardingUnit() {
    this->logger->log(Stage::EX, "[EXMEMStageRegisters] Passing register destination to ForwardingUnit.");
    this->forwarding_unit->setEXMEMStageRegisterDestination(this->register_destination);
    this->logger->log(Stage::EX, "[EXMEMStageRegisters] Passed register destination to ForwardingUnit.");
}
