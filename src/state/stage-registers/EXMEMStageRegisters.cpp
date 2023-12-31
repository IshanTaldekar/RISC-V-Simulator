#include "../../../include/state/stage-registers/EXMEMStageRegisters.h"

EXMEMStageRegisters *EXMEMStageRegisters::current_instance = nullptr;

EXMEMStageRegisters::EXMEMStageRegisters() {
    this->branch_program_counter = 0UL;
    this->alu_result = 0UL;
    this->read_data_2 = 0UL;
    this->register_destination = 0UL;

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

    this->control = new Control(new Instruction(std::string(32, '0')));

    this->data_memory = DataMemory::init();
    this->mem_wb_stage_registers = MEMWBStageRegisters::init();
    this->if_mux = IFMux::init();
    this->stage_synchronizer = StageSynchronizer::init();
}

void EXMEMStageRegisters::reset() {
    this->is_reset_flag_set = true;
    this->notifyModuleConditionVariable();
}

void EXMEMStageRegisters::resetStage() {
    if (this->getStage() == Stage::Single) {
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

void EXMEMStageRegisters::pauseStage() {
    this->is_alu_result_zero = false;
    this->is_branch_program_counter_set = false;
    this->is_alu_result_set = false;
    this->is_read_data_2_set = false;
    this->is_register_destination_set = false;
    this->is_alu_result_zero_flag_set = false;
    this->is_control_set = false;
}

void EXMEMStageRegisters::pause() {
    this->is_pause_flag_set = true;
    this->notifyModuleConditionVariable();
}

EXMEMStageRegisters *EXMEMStageRegisters::init() {
    if (EXMEMStageRegisters::current_instance == nullptr) {
        EXMEMStageRegisters::current_instance = new EXMEMStageRegisters();
    }

    return EXMEMStageRegisters::current_instance;
}

void EXMEMStageRegisters::run() {
    while (this->isAlive()) {
        std::unique_lock<std::mutex> ex_mem_stage_registers_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                ex_mem_stage_registers_lock,
                [this] {
                    return (this->is_branch_program_counter_set && this->is_alu_result_set &&
                            this->is_read_data_2_set && this->is_register_destination_set &&
                            this->is_alu_result_zero_flag_set && this->is_control_set) ||
                            this->is_reset_flag_set || this->is_pause_flag_set;
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

        this->control->setIsALUResultZero(this->is_alu_result_zero);
        this->control->toggleMEMStageControlSignals();

        this->passWriteDataToDataMemory();
        this->passALUResultToDataMemory();
        this->passBranchedAddressToIFMux();

        std::thread pass_alu_result_thread (&EXMEMStageRegisters::passALUResultToMEMWBStageRegisters, this);
        std::thread pass_register_destination_thread (&EXMEMStageRegisters::passRegisterDestinationToMEMWBStageRegisters, this);

        this->is_branch_program_counter_set = false;
        this->is_alu_result_set = false;
        this->is_read_data_2_set = false;
        this->is_register_destination_set = false;
        this->is_alu_result_zero_flag_set = false;
        this->is_control_set = false;

        this->stage_synchronizer->conditionalArriveSingleStage();
    }
}

void EXMEMStageRegisters::notifyModuleConditionVariable() {
    this->getModuleConditionVariable().notify_one();
}

void EXMEMStageRegisters::setBranchedProgramCounter(unsigned long value) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    std::lock_guard<std::mutex> ex_mem_stage_registers_lock (this->getModuleMutex());

    this->branch_program_counter = value;
    this->is_branch_program_counter_set = true;

    this->notifyModuleConditionVariable();
}

void EXMEMStageRegisters::setALUResult(unsigned long value) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    std::lock_guard<std::mutex> ex_mem_stage_registers_lock (this->getModuleMutex());

    this->alu_result = value;
    this->is_alu_result_set = true;

    this->notifyModuleConditionVariable();
}

void EXMEMStageRegisters::setIsResultZeroFlag(bool asserted) {
    this->stage_synchronizer->conditionalArriveSingleStage();

    std::lock_guard<std::mutex> ex_mem_stage_registers_lock (this->getModuleMutex());

    this->is_alu_result_zero = asserted;
    this->is_alu_result_zero_flag_set = true;

    this->notifyModuleConditionVariable();
}

void EXMEMStageRegisters::setReadData2(unsigned long value) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    std::lock_guard<std::mutex> ex_mem_stage_registers_lock (this->getModuleMutex());

    this->read_data_2 = value;
    this->is_read_data_2_set = true;

    this->notifyModuleConditionVariable();
}

void EXMEMStageRegisters::setRegisterDestination(unsigned long value) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    std::lock_guard<std::mutex> ex_mem_stage_registers_lock (this->getModuleMutex());

    this->register_destination = value;
    this->is_register_destination_set = true;

    this->notifyModuleConditionVariable();
}

void EXMEMStageRegisters::setControl(Control *new_control) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    std::lock_guard<std::mutex> ex_mem_stage_registers_lock (this->getModuleMutex());

    if (this->is_nop_asserted) {
        this->control = new Control(new Instruction(std::string(32, '0')));
    } else {
        this->control = new_control;
    }

    this->is_control_set = true;
    this->notifyModuleConditionVariable();
}

void EXMEMStageRegisters::passALUResultToDataMemory() {
    this->data_memory->setAddress(this->alu_result);
}

void EXMEMStageRegisters::passWriteDataToDataMemory() {
    this->data_memory->setWriteData(this->read_data_2);
}

void EXMEMStageRegisters::passALUResultToMEMWBStageRegisters() {
    this->mem_wb_stage_registers->setALUResult(this->alu_result);
}

void EXMEMStageRegisters::passRegisterDestinationToMEMWBStageRegisters() {
    this->mem_wb_stage_registers->setRegisterDestination(this->register_destination);
}

void EXMEMStageRegisters::passBranchedAddressToIFMux() {
    this->if_mux->setInput(IFStageMuxInputType::BranchedPc, this->branch_program_counter);
}

void EXMEMStageRegisters::setNop() {
    this->is_nop_asserted = true;
}

unsigned long EXMEMStageRegisters::getRegisterDestination() {
    std::lock_guard<std::mutex> ex_mem_stage_registers_lock (this->getModuleMutex());
    return this->register_destination;
}

unsigned long EXMEMStageRegisters::getRegisterDestinationData() {
    std::lock_guard<std::mutex> ex_mem_stage_registers_lock (this->getModuleMutex());
}