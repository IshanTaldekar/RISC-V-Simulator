#include "../../../include/state/stage-registers/MEMWBStageRegisters.h"

MEMWBStageRegisters *MEMWBStageRegisters::current_instance = nullptr;

MEMWBStageRegisters::MEMWBStageRegisters() {
    this->read_data = 0UL;
    this->alu_result = 0UL;
    this->register_destination = 0L;

    this->is_read_data_set = true;
    this->is_alu_result_set = true;
    this->is_register_destination_set = true;
    this->is_control_set = true;
    this->is_reset_flag_set = false;
    this->is_pause_flag_set = false;

    this->control = new Control(new Instruction(std::string(32, '0')));

    this->register_file = RegisterFile::init();
    this->wb_mux = WBMux::init();
    this->stage_synchronizer = StageSynchronizer::init();
}

void MEMWBStageRegisters::reset() {
    this->is_reset_flag_set = true;
    this->notifyModuleConditionVariable();
}

void MEMWBStageRegisters::resetStage() {
    if (this->getStage() == Stage::Single) {
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

void MEMWBStageRegisters::pauseStage() {
    this->is_read_data_set = false;
    this->is_alu_result_set = false;
    this->is_register_destination_set = false;
    this->is_control_set = false;
}

void MEMWBStageRegisters::pause() {
    this->is_pause_flag_set = true;
    this->notifyModuleConditionVariable();
}

MEMWBStageRegisters *MEMWBStageRegisters::init() {
    if (MEMWBStageRegisters::current_instance == nullptr) {
        MEMWBStageRegisters::current_instance = new MEMWBStageRegisters();
    }

    return MEMWBStageRegisters::current_instance;
}

void MEMWBStageRegisters::run() {
    while (this->isAlive()) {
        std::unique_lock<std::mutex> mem_wb_stage_registers_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                mem_wb_stage_registers_lock,
                [this] {
                    return (this->is_read_data_set && this->is_alu_result_set && this->is_register_destination_set &&
                            this->is_control_set) || this->is_pause_flag_set || this->is_reset_flag_set;
                }
        );

        if (this->isKilled()) {
            break;
        }

        if (this->is_reset_flag_set) {
            this->resetStage();
            this->is_reset_flag_set = false;

            continue;
        }

        this->control->toggleWBStageControlSignals();

        this->passReadDataToWBMux();
        this->passALUResultToWBMux();

        this->passRegisterDestinationToRegisterFile();

        this->is_read_data_set = false;
        this->is_alu_result_set = false;
        this->is_register_destination_set = false;
        this->is_control_set = false;

        this->stage_synchronizer->conditionalArriveSingleStage();
    }
}

void MEMWBStageRegisters::notifyModuleConditionVariable() {
    this->getModuleConditionVariable().notify_one();
}

void MEMWBStageRegisters::setReadData(unsigned long value) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    std::lock_guard<std::mutex> mem_wb_stage_registers_lock (this->getModuleMutex());

    this->read_data = value;
    this->is_read_data_set = true;

    this->notifyModuleConditionVariable();
}

void MEMWBStageRegisters::setALUResult(unsigned long value) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    std::lock_guard<std::mutex> mem_wb_stage_registers_lock (this->getModuleMutex());

    this->alu_result = value;
    this->is_alu_result_set = true;

    this->notifyModuleConditionVariable();
}

void MEMWBStageRegisters::setRegisterDestination(unsigned long value) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    std::lock_guard<std::mutex> mem_wb_stage_registers_lock (this->getModuleMutex());

    this->register_destination = value;
    this->is_register_destination_set = true;

    this->notifyModuleConditionVariable();
}

void MEMWBStageRegisters::passALUResultToWBMux() {
    this->wb_mux->setInput(WBStageMuxInputType::ALUResult, this->alu_result);
}

void MEMWBStageRegisters::passReadDataToWBMux() {
    this->wb_mux->setInput(WBStageMuxInputType::ReadData, this->read_data);
}

void MEMWBStageRegisters::passRegisterDestinationToRegisterFile() {
    this->register_file->setWriteRegister(this->register_destination);
}