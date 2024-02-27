#include "../../include/combinational/ForwardingUnit.h"

ForwardingUnit *ForwardingUnit::current_instance = nullptr;

ForwardingUnit::ForwardingUnit() {
    this->register_source1 = 0UL;
    this->register_source2 = 0UL;
    this->ex_mem_stage_register_destination = 0UL;
    this->mem_wb_stage_register_destination = 0UL;

    this->is_single_register_source_set = false;
    this->is_double_register_source_set = false;
    this->is_ex_mem_stage_register_destination_set = false;
    this->is_mem_wb_stage_register_destination_set = false;
    this->is_reset_flag_set = false;

    this->alu_input_1_mux_control_signal = ALUInputMuxControlSignals::IDEXStageRegisters;
    this->alu_input_2_mux_control_signal = ALUInputMuxControlSignals::IDEXStageRegisters;

    this->alu_input_1_mux = ALUInput1ForwardingMux::init();
    this->alu_input_2_mux = ALUInput2ForwardingMux::init();
}

ForwardingUnit *ForwardingUnit::init() {
    if (ForwardingUnit::current_instance == nullptr) {
        ForwardingUnit::current_instance = new ForwardingUnit();
    }

    return ForwardingUnit::current_instance;
}

void ForwardingUnit::run() {
    while (this->isAlive()) {
        std::unique_lock<std::mutex> forwarding_unit_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                forwarding_unit_lock,
                [this] {
                    return ((this->is_single_register_source_set || this->is_double_register_source_set) &&
                        (this->getStage() == PipelineType::Single || (this->is_ex_mem_stage_register_destination_set &&
                                                                      this->is_mem_wb_stage_register_destination_set)) || this->is_reset_flag_set);
                }
        );

        if (this->isKilled()) {
            break;
        }

        if (this->is_reset_flag_set) {
            this->resetState();
            continue;
        }

        this->computeControlSignals();
        this->passControlSignalToALUInputMux1();
        this->passControlSignalToALUInputMux2();

        this->is_single_register_source_set = false;
        this->is_double_register_source_set = false;
        this->is_ex_mem_stage_register_destination_set = false;
        this->is_mem_wb_stage_register_destination_set = false;
    }
}

void ForwardingUnit::notifyModuleConditionVariable() {
    this->getModuleConditionVariable().notify_one();
}

void ForwardingUnit::setSingleRegisterSource(unsigned long rs1) {
    std::lock_guard<std::mutex> forwarding_unit_lock (this->getModuleMutex());

    this->register_source1 = rs1;
    this->is_single_register_source_set = true;
}

void ForwardingUnit::setDoubleRegisterSource(unsigned long rs1, unsigned long rs2) {
    std::lock_guard<std::mutex> forwarding_unit_lock (this->getModuleMutex());

    this->register_source1 = rs1;
    this->register_source2 = rs2;
    this->is_double_register_source_set = true;
}

void ForwardingUnit::setEXMEMStageRegisterDestination(unsigned long rd) {
    std::lock_guard<std::mutex> forwarding_unit_lock (this->getModuleMutex());

    this->ex_mem_stage_register_destination = rd;
    this->is_ex_mem_stage_register_destination_set = true;
}

void ForwardingUnit::setMEMWBStageRegisterDestination(unsigned long rd) {
    std::lock_guard<std::mutex> forwarding_unit_lock (this->getModuleMutex());

    this->mem_wb_stage_register_destination = rd;
    this->is_mem_wb_stage_register_destination_set = true;
}

void ForwardingUnit::reset() {
    std::lock_guard<std::mutex> forwarding_unit_lock (this->getModuleMutex());

    this->is_reset_flag_set = true;
}

void ForwardingUnit::resetState() {
    this->is_single_register_source_set = false;
    this->is_double_register_source_set = false;
    this->is_ex_mem_stage_register_destination_set = false;
    this->is_mem_wb_stage_register_destination_set = false;
    this->is_reset_flag_set = false;
}

void ForwardingUnit::computeControlSignals() {
    if (this->getStage() == PipelineType::Single) {
        this->alu_input_1_mux_control_signal = ALUInputMuxControlSignals::IDEXStageRegisters;
        this->alu_input_2_mux_control_signal = ALUInputMuxControlSignals::IDEXStageRegisters;
    } else {
        this->alu_input_1_mux_control_signal = ALUInputMuxControlSignals::IDEXStageRegisters;
        this->alu_input_2_mux_control_signal = ALUInputMuxControlSignals::IDEXStageRegisters;

        if (this->register_source1 == this->ex_mem_stage_register_destination) {
            this->alu_input_1_mux_control_signal = ALUInputMuxControlSignals::EXMEMStageRegisters;
        } else if (this->register_source1 == this->mem_wb_stage_register_destination) {
            this->alu_input_1_mux_control_signal = ALUInputMuxControlSignals::MEMWBStageRegisters;
        }

        if (this->is_double_register_source_set) {
            if (this->register_source2 == this->ex_mem_stage_register_destination) {
                this->alu_input_2_mux_control_signal = ALUInputMuxControlSignals::EXMEMStageRegisters;
            } else if (this->register_source1 == this->mem_wb_stage_register_destination) {
                this->alu_input_2_mux_control_signal = ALUInputMuxControlSignals::MEMWBStageRegisters;
            }
        }
    }
}

void ForwardingUnit::passControlSignalToALUInputMux1() {
    this->alu_input_1_mux->setMuxControlSignal(this->alu_input_1_mux_control_signal);
}

void ForwardingUnit::passControlSignalToALUInputMux2() {
    this->alu_input_2_mux->setMuxControlSignal(this->alu_input_2_mux_control_signal);
}

