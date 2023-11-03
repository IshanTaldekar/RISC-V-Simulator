#include "../../include/common/Control.h"

Control::Control(const Instruction *current_instruction) {
    this->instruction = current_instruction;

    this->register_file = RegisterFile::init();
    this->if_mux = IFMux::init();

    this->is_reg_write_asserted = false;
    this->is_pc_src_asserted = false;
    this->is_alu_src_asserted = false;
    this->is_mem_read_asserted = false;

    this->generateSignals();
}

void Control::generateSignals() {
    InstructionType type = this->instruction->getType();

    if (type == InstructionType::R || type == InstructionType::I || type == InstructionType::J) {
        this->is_reg_write_asserted = true;
    }

    if (type == InstructionType::I || type == InstructionType::J || type == InstructionType::S) {
        this->is_alu_src_asserted = true;
    }

    if (type == InstructionType::B || type == InstructionType::J) {
        this->is_pc_src_asserted = true;
    }

    if (type == InstructionType::I && this->instruction->getOpcode().to_string() == "0000011") {
        this->is_mem_read_asserted = true;
        this->is_mem_to_reg_asserted = true;
    }

    if (type == InstructionType::S) {
        this->is_mem_write_asserted = true;
    }
}

void Control::setEXStageControlSignals() {

}

void Control::setMEMStageControlSignals() {

}

void Control::setWBStageControlSignals() {

}