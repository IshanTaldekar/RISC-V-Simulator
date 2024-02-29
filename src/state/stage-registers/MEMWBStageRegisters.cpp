#include "../../../include/state/stage-registers/MEMWBStageRegisters.h"

MEMWBStageRegisters *MEMWBStageRegisters::current_instance = nullptr;
std::mutex MEMWBStageRegisters::initialization_mutex;

MEMWBStageRegisters::MEMWBStageRegisters() {
    this->read_data = 0UL;
    this->alu_result = 0UL;
    this->register_destination = 0L;

    this->is_read_data_set = false;
    this->is_alu_result_set = false;
    this->is_register_destination_set = false;
    this->is_control_set = false;
    this->is_reset_flag_set = false;
    this->is_pause_flag_set = false;

    this->control = nullptr;

    this->register_file = nullptr;
    this->wb_mux = nullptr;
    this->stage_synchronizer = nullptr;
    this->forwarding_unit = nullptr;
    this->logger = nullptr;
}

void MEMWBStageRegisters::reset() {
    this->is_reset_flag_set = true;
    this->notifyModuleConditionVariable();
}

void MEMWBStageRegisters::resetStage() {
    if (this->getPipelineType() == PipelineType::Single) {
        this->is_read_data_set = false;
        this->is_alu_result_set = false;
        this->is_register_destination_set = false;
        this->is_control_set = false;
    } else {
        this->is_read_data_set = true;
        this->is_alu_result_set = true;
        this->is_register_destination_set = true;
        this->is_control_set = true;
    }

    this->read_data = 0UL;
    this->alu_result = 0UL;
    this->register_destination = 0L;

    this->control = new Control(new Instruction(std::string(32, '0')));
}

void MEMWBStageRegisters::pause() {
    this->is_pause_flag_set = true;
}

void MEMWBStageRegisters::resume() {
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
    this->control = new Control(new Instruction(std::string(32, '0')));

    this->register_file = RegisterFile::init();
    this->wb_mux = WBMux::init();
    this->stage_synchronizer = StageSynchronizer::init();
    this->forwarding_unit = ForwardingUnit::init();
    this->logger = Logger::init();
}

void MEMWBStageRegisters::run() {
    this->initDependencies();
    
    while (this->isAlive()) {
        this->logger->log(Stage::MEM, "[MEMWBStageRegisters] Waiting to be woken up and acquire lock.");

        std::unique_lock<std::mutex> mem_wb_stage_registers_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                mem_wb_stage_registers_lock,
                [this] {
                    return (this->is_read_data_set && this->is_alu_result_set && this->is_register_destination_set &&
                            this->is_control_set && !this->is_pause_flag_set) || this->is_reset_flag_set || this->isKilled();
                }
        );

        if (this->isKilled()) {
            this->logger->log(Stage::MEM, "[MEMWBStageRegisters] Killed.");
            break;
        }

        if (this->is_reset_flag_set) {
            this->logger->log(Stage::MEM, "[MEMWBStageRegisters] Resetting stage.");

            this->resetStage();
            this->is_reset_flag_set = false;

            this->logger->log(Stage::MEM, "[MEMWBStageRegisters] Reset.");
            continue;
        }

        this->logger->log(Stage::MEM, "[MEMWBStageRegisters] Woken up and acquired lock.");

        this->control->toggleWBStageControlSignals();

        this->passReadDataToWBMux();
        this->passALUResultToWBMux();
        this->passRegisterDestinationToForwardingUnit();
        this->passRegisterDestinationToRegisterFile();

        this->is_read_data_set = false;
        this->is_alu_result_set = false;
        this->is_register_destination_set = false;
        this->is_control_set = false;

        this->stage_synchronizer->conditionalArriveSingleStage();
    }
}

void MEMWBStageRegisters::setReadData(unsigned long value) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    this->logger->log(Stage::MEM, "[MEMWBStageRegisters] setReadData waiting to acquire lock.");

    std::lock_guard<std::mutex> mem_wb_stage_registers_lock (this->getModuleMutex());

    this->logger->log(Stage::MEM, "[MEMWBStageRegisters] setReadData acquired lock. Updating value.");

    this->read_data = value;
    this->is_read_data_set = true;

    this->logger->log(Stage::MEM, "[MEMWBStageRegisters] setReadData updated value.");
    this->notifyModuleConditionVariable();
}

void MEMWBStageRegisters::setALUResult(unsigned long value) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    this->logger->log(Stage::MEM, "[MEMWBStageRegisters] setALUResult waiting to acquire lock.");

    std::lock_guard<std::mutex> mem_wb_stage_registers_lock (this->getModuleMutex());

    this->logger->log(Stage::MEM, "[MEMWBStageRegisters] setALUResult acquired lock. Updating value.");

    this->alu_result = value;
    this->is_alu_result_set = true;

    this->logger->log(Stage::MEM, "[MEMWBStageRegisters] setALUResult updated value.");
    this->notifyModuleConditionVariable();
}

void MEMWBStageRegisters::setRegisterDestination(unsigned long value) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    this->logger->log(Stage::MEM, "[MEMWBStageRegisters] setRegisterDestination waiting to acquire lock.");

    std::lock_guard<std::mutex> mem_wb_stage_registers_lock (this->getModuleMutex());

    this->logger->log(Stage::MEM, "[MEMWBStageRegisters] setRegisterDestination acquired lock. Updating value.");

    this->register_destination = value;
    this->is_register_destination_set = true;

    this->logger->log(Stage::MEM, "[MEMWBStageRegisters] setRegisterDestination updated value.");
    this->notifyModuleConditionVariable();
}

void MEMWBStageRegisters::passALUResultToWBMux() {
    this->logger->log(Stage::MEM, "[MEMWBStageRegisters] Passing ALU result to WBMux.");
    this->wb_mux->setInput(WBStageMuxInputType::ALUResult, this->alu_result);
    this->logger->log(Stage::MEM, "[MEMWBStageRegisters] Passed ALU result to WBMux.");
}

void MEMWBStageRegisters::passReadDataToWBMux() {
    this->logger->log(Stage::MEM, "[MEMWBStageRegisters] Passing read data to WBMux.");
    this->wb_mux->setInput(WBStageMuxInputType::ReadData, this->read_data);
    this->logger->log(Stage::MEM, "[MEMWBStageRegisters] Passed read data to WBMux.");
}

void MEMWBStageRegisters::passRegisterDestinationToRegisterFile() {
    this->logger->log(Stage::MEM, "[MEMWBStageRegisters] Passing register destination to RegisterFile.");
    this->register_file->setWriteRegister(this->register_destination);
    this->logger->log(Stage::MEM, "[MEMWBStageRegisters] Passed register destination to RegisterFile.");
}

void MEMWBStageRegisters::passRegisterDestinationToForwardingUnit() {
    this->logger->log(Stage::MEM, "[MEMWBStageRegisters] Passing register destination to ForwardingUnit.");
    this->forwarding_unit->setMEMWBStageRegisterDestination(this->register_destination);
    this->logger->log(Stage::MEM, "[MEMWBStageRegisters] Passed register destination to ForwardingUnit.");
}