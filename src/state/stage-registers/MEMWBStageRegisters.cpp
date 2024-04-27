#include "../../../include/state/stage-registers/MEMWBStageRegisters.h"

MEMWBStageRegisters *MEMWBStageRegisters::current_instance = nullptr;
std::mutex MEMWBStageRegisters::initialization_mutex;

MEMWBStageRegisters::MEMWBStageRegisters() {
    this->read_data = std::bitset<WORD_BIT_COUNT>(std::string(32, '0'));
    this->alu_result = std::bitset<WORD_BIT_COUNT>(std::string(32, '0'));
    this->register_destination = 0L;

    this->is_read_data_set = false;
    this->is_alu_result_set = false;
    this->is_register_destination_set = false;
    this->is_control_set = false;
    this->is_reset_flag_set = false;
    this->is_pause_flag_set = false;
    this->is_nop_asserted = false;
    this->is_nop_passed_flag_set = false;
    this->is_nop_passed_flag_asserted = false;
    this->is_verbose_execution_flag_asserted = false;

    this->control = nullptr;

    this->register_file = nullptr;
    this->wb_mux = nullptr;
    this->stage_synchronizer = nullptr;
    this->forwarding_unit = nullptr;
    this->logger = nullptr;
}

void MEMWBStageRegisters::reset() {
    std::lock_guard<std::mutex> mem_wb_stage_registers_lock (this->getModuleMutex());

    this->is_reset_flag_set = true;
    this->notifyModuleConditionVariable();
}

void MEMWBStageRegisters::resetStage() {
    if (this->getPipelineType() == PipelineType::Single) {
        this->is_read_data_set = false;
        this->is_alu_result_set = false;
        this->is_register_destination_set = false;
        this->is_control_set = false;
        this->is_nop_passed_flag_set = false;
        this->is_nop_passed_flag_asserted = false;
    } else {
        this->is_read_data_set = true;
        this->is_alu_result_set = true;
        this->is_register_destination_set = true;
        this->is_control_set = true;
        this->is_nop_passed_flag_set = true;
        this->is_nop_passed_flag_asserted = true;
    }

    this->is_nop_asserted = false;

    this->read_data = std::bitset<WORD_BIT_COUNT>(std::string(32, '0'));
    this->alu_result = std::bitset<WORD_BIT_COUNT>(std::string(32, '0'));
    this->register_destination = 0L;

    this->control = new Control(new Instruction(std::string(32, '0')), this->getPipelineType());
}

void MEMWBStageRegisters::pause() {
    this->is_pause_flag_set = true;
}

void MEMWBStageRegisters::resume() {
    std::lock_guard<std::mutex> mem_wb_stage_registers_lock (this->getModuleMutex());

    this->is_pause_flag_set = false;
    this->notifyModuleConditionVariable();
}

MEMWBStageRegisters *MEMWBStageRegisters::init() {
    std::lock_guard<std::mutex> mem_wb_stage_registers_lock (MEMWBStageRegisters::initialization_mutex);

    if (MEMWBStageRegisters::current_instance == nullptr) {
        MEMWBStageRegisters::current_instance = new MEMWBStageRegisters();
    }

    return MEMWBStageRegisters::current_instance;
}

void MEMWBStageRegisters::initDependencies() {
    std::unique_lock<std::mutex> mem_wb_stage_registers_lock (this->getModuleDependencyMutex());

    if (this->control && this->register_file && this->wb_mux && this->stage_synchronizer && this->forwarding_unit &&
        this->logger) {
        return;
    }

    this->control = new Control(new Instruction(std::string(32, '0')), this->getPipelineType());

    this->register_file = RegisterFile::init();
    this->wb_mux = WBMux::init();
    this->stage_synchronizer = StageSynchronizer::init();
    this->forwarding_unit = ForwardingUnit::init();
    this->logger = Logger::init();
}

void MEMWBStageRegisters::run() {
    this->initDependencies();
    
    while (this->isAlive()) {
        this->log("Waiting to be woken up and acquire lock.");

        std::unique_lock<std::mutex> mem_wb_stage_registers_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                mem_wb_stage_registers_lock,
                [this] {
                    return (this->is_read_data_set && this->is_alu_result_set && this->is_register_destination_set &&
                            this->is_control_set && !this->is_pause_flag_set && this->is_nop_passed_flag_set) ||
                           this->is_reset_flag_set || this->isKilled();
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
        this->control->toggleWBStageControlSignals();

        if (this->is_verbose_execution_flag_asserted) {
            this->printState();
        }

        this->passReadDataToWBMux();
        this->passALUResultToWBMux();

        std::thread pass_register_destination_register_file (
                &MEMWBStageRegisters::passRegisterDestinationToRegisterFile,
                this,
                this->register_destination
        );

        std::thread pass_register_destination_forwarding_unit (
                &MEMWBStageRegisters::passRegisterDestinationToForwardingUnit,
                this,
                this->register_destination
        );

        std::thread pass_reg_write_forwarding_unit (
                &MEMWBStageRegisters::passRegWriteToForwardingUnit,
                this,
                this->control->isRegWriteAsserted()
        );

        pass_register_destination_register_file.detach();
        pass_register_destination_forwarding_unit.detach();
        pass_reg_write_forwarding_unit.detach();

        this->is_read_data_set = false;
        this->is_alu_result_set = false;
        this->is_register_destination_set = false;
        this->is_control_set = false;
        this->is_nop_asserted = false;
        this->is_nop_passed_flag_set = false;

        this->stage_synchronizer->conditionalArriveSingleStage();
    }
}

void MEMWBStageRegisters::setReadData(std::bitset<WORD_BIT_COUNT> value) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    this->log("setReadData waiting to acquire lock.");

    std::lock_guard<std::mutex> mem_wb_stage_registers_lock (this->getModuleMutex());

    this->log("setReadData acquired lock. Updating value.");

    this->read_data = value;
    this->is_read_data_set = true;

    this->log("setReadData updated value.");
    this->notifyModuleConditionVariable();
}

void MEMWBStageRegisters::setALUResult(std::bitset<WORD_BIT_COUNT> value) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    this->log("setALUResult waiting to acquire lock.");

    std::lock_guard<std::mutex> mem_wb_stage_registers_lock (this->getModuleMutex());

    this->log("setALUResult acquired lock. Updating value.");

    this->alu_result = value;
    this->is_alu_result_set = true;

    this->log("setALUResult updated value.");
    this->notifyModuleConditionVariable();
}

void MEMWBStageRegisters::setRegisterDestination(unsigned long value) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    this->log("setRegisterDestination waiting to acquire lock.");

    std::lock_guard<std::mutex> mem_wb_stage_registers_lock (this->getModuleMutex());

    this->log("setRegisterDestination acquired lock. Updating value.");

    this->register_destination = value;
    this->is_register_destination_set = true;

    this->log("setRegisterDestination updated value.");
    this->notifyModuleConditionVariable();
}

void MEMWBStageRegisters::setControl(Control *new_control) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    this->log("setControl waiting to acquire lock.");

    std::lock_guard<std::mutex> mem_wb_stage_registers_lock (this->getModuleMutex());

    this->log("setControl acquired lock. Updating value.");

    this->control = new_control;
    this->is_control_set = true;

    this->log("setControl updated value.");
    this->notifyModuleConditionVariable();
}

void MEMWBStageRegisters::setPassedNop(bool is_asserted) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    this->log("setPassedNop waiting to acquire lock.");

    std::lock_guard<std::mutex> if_id_stage_registers_lock (this->getModuleMutex());

    this->log("setPassedNop acquired lock.");

    this->is_nop_passed_flag_asserted = is_asserted;
    this->is_nop_passed_flag_set = true;

    this->log("setPassedNop updated value.");
    this->notifyModuleConditionVariable();
}

void MEMWBStageRegisters::passALUResultToWBMux() {
    this->log("Passing ALU result to WBMux.");
    this->wb_mux->setInput(WBStageMuxInputType::ALUResult, this->alu_result);
    this->log("Passed ALU result to WBMux.");
}

void MEMWBStageRegisters::passReadDataToWBMux() {
    this->log("Passing read data to WBMux.");
    this->wb_mux->setInput(WBStageMuxInputType::ReadData, this->read_data);
    this->log("Passed read data to WBMux.");
}

void MEMWBStageRegisters::passRegisterDestinationToRegisterFile(unsigned long rd) {
    this->log("Passing register destination to RegisterFile.");
    this->register_file->setWriteRegister(rd);
    this->log("Passed register destination to RegisterFile.");
}

void MEMWBStageRegisters::passRegisterDestinationToForwardingUnit(unsigned long rd) {
    this->log("Passing register destination to ForwardingUnit.");
    this->forwarding_unit->setMEMWBStageRegisterDestination(rd);
    this->log("Passed register destination to ForwardingUnit.");
}

void MEMWBStageRegisters::passRegWriteToForwardingUnit(bool is_asserted) {
    this->log("Passing reg write to ForwardingUnit.");
    this->forwarding_unit->setMEMWBStageRegisterRegWrite(is_asserted);
    this->log("Passed reg write to ForwardingUnit.");
}

bool MEMWBStageRegisters::isExecutingHaltInstruction() {
    return this->control->is_halt_instruction;
}

void MEMWBStageRegisters::assertNop() {
    this->is_nop_asserted = true;
}

std::string MEMWBStageRegisters::getModuleTag() {
    return "MEMWBStageRegisters";
}

Stage MEMWBStageRegisters::getModuleStage() {
    return Stage::MEM;
}

void MEMWBStageRegisters::printState() {
    std::cout << std::string(20, '.') << std::endl;
    std::cout << "MEMWBStageRegisters" << std::endl;
    std::cout << std::string(20, '.') << std::endl;

    std::cout << "read_data: " << this->read_data.to_ulong() << std::endl;
    std::cout << "alu_result: " << this->alu_result.to_ulong() << std::endl;
    std::cout << "register_destination: " << this->register_destination << std::endl;
    std::cout << "is_read_data_set: " << this->is_read_data_set << std::endl;
    std::cout << "is_alu_result_set: " << this->is_alu_result_set << std::endl;
    std::cout << "is_register_destination_set: " << this->is_register_destination_set << std::endl;
    std::cout << "is_control_set: " << this->is_control_set << std::endl;
    std::cout << "is_reset_flag_set: " << this->is_reset_flag_set << std::endl;
    std::cout << "is_pause_flag_set: " << this->is_pause_flag_set << std::endl;
    std::cout << "is_nop_asserted: " << this->is_nop_asserted << std::endl;
    std::cout << "is_nop_passed_flag_set: " << this->is_nop_passed_flag_set << std::endl;
    std::cout << "is_nop_passed_flag_asserted: " << this->is_nop_passed_flag_asserted << std::endl;

    this->control->printState();
}

void MEMWBStageRegisters::assertVerboseExecutionFlag() {
    this->is_verbose_execution_flag_asserted = true;
}