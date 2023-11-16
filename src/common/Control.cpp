#include "../../include/common/Control.h"

Control::Control(const Instruction *current_instruction) {
    this->instruction = current_instruction;

    this->register_file = RegisterFile::init();
    this->if_mux = IFMux::init();
    this->ex_mux = EXMux::init();
    this->alu = ALU::init();
    this->data_memory = DataMemory::init();

    this->is_reg_write_asserted = false;
    this->is_pc_src_asserted = false;
    this->is_alu_src_asserted = false;
    this->is_mem_write_asserted = false;
    this->is_mem_read_asserted = false;
    this->is_reg_write_asserted = false;
    this->is_mem_to_reg_asserted = false;
    this->is_branch_instruction = false;
    this->is_alu_result_zero = false;

    this->generateSignals();
    this->generateALUOpCode();
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

void Control::generateALUOpCode() {
    InstructionType type = this->instruction->getType();

    if (type == InstructionType::R || type == InstructionType::I) {
        std::bitset<Instruction::FUNCT3_BIT_COUNT> funct3 = this->instruction->getFunct3();
        std::bitset<Instruction::FUNCT7_BIT_COUNT> funct7 = this->instruction->getFunct7();

        if (funct3 == std::bitset<Instruction::FUNCT3_BIT_COUNT>("000")) {
            if (this->instruction->getOpcode() == std::bitset<Instruction::OPCODE_BIT_COUNT>("0010011") ||
                    this->instruction->getOpcode() == std::bitset<Instruction::OPCODE_BIT_COUNT>("0000011") ||
                    funct7 == std::bitset<Instruction::FUNCT7_BIT_COUNT>("0000000")) {
                this->alu_op = std::bitset<ALU::ALU_OP_BIT_COUNT>("0000");  // Add
            } else if (funct7 == std::bitset<Instruction::FUNCT7_BIT_COUNT>("0100000") &&
                    this->instruction->getOpcode() != std::bitset<Instruction::OPCODE_BIT_COUNT>("0010011")) {
                this->alu_op = std::bitset<ALU::ALU_OP_BIT_COUNT>("0001");  // Subtract
            }
        } else if (funct3 == std::bitset<Instruction::FUNCT3_BIT_COUNT>("100")) {
            this->alu_op = std::bitset<ALU::ALU_OP_BIT_COUNT>("0010");  // Xor
        } else if (funct3 == std::bitset<Instruction::FUNCT3_BIT_COUNT>("110")) {
            this->alu_op = std::bitset<ALU::ALU_OP_BIT_COUNT>("0011");  // Or
        } else if (funct3 == std::bitset<Instruction::FUNCT3_BIT_COUNT>("111")) {
            this->alu_op = std::bitset<ALU::ALU_OP_BIT_COUNT>("0100");  // And
        }
    } else if (type == InstructionType::J || type == InstructionType::S) {
        this->alu_op = std::bitset<ALU::ALU_OP_BIT_COUNT>("0000");  // Add
    } else if (type == InstructionType::B) {
        this->alu_op = std::bitset<ALU::ALU_OP_BIT_COUNT>("0001");  // Subtract
    }
}

void Control::setIsALUResultZero(bool is_result_zero) {
    this->is_alu_result_zero = is_result_zero;
}

void Control::toggleEXStageControlSignals() {
    this->alu->setALUOp(this->alu_op);
    this->ex_mux->assertControlSignal(this->is_alu_src_asserted);
}

void Control::toggleMEMStageControlSignals() {
    this->is_pc_src_asserted &= this->is_alu_result_zero;

    this->if_mux->assertControlSignal(this->is_pc_src_asserted);
    this->data_memory->setMemWrite(this->is_mem_write_asserted);
    this->data_memory->setMemRead(this->is_mem_read_asserted);
}

void Control::toggleWBStageControlSignals() {

}