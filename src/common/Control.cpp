#include "../../include/common/Control.h"

Control::Control(Instruction *current_instruction) {
    this->instruction = current_instruction;

    this->is_reg_write_asserted = false;
    this->is_pc_src_asserted = false;
    this->is_alu_src_asserted = false;
    this->is_mem_write_asserted = false;
    this->is_mem_read_asserted = false;
    this->is_reg_write_asserted = false;
    this->is_mem_to_reg_asserted = false;
    this->is_branch_instruction = false;
    this->is_alu_result_zero = false;
    this->is_jal_instruction = false;
    this->is_halt_instruction = false;
    this->is_nop_asserted_flag = false;

    this->register_file = nullptr;
    this->if_mux = nullptr;
    this->ex_mux_alu_input_1 = nullptr;
    this->ex_mux_alu_input_2 = nullptr;
    this->alu = nullptr;
    this->data_memory = nullptr;
    this->wb_mux = nullptr;
    this->if_id_stage_registers = nullptr;
    this->id_ex_stage_registers = nullptr;
    this->ex_mem_stage_registers = nullptr;

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

        if (type == InstructionType::J) {
            this->is_jal_instruction = true;
        }
    }

    if (type == InstructionType::B || type == InstructionType::J) {
        this->is_pc_src_asserted = true;
        this->is_branch_instruction = true;
    }

    if (type == InstructionType::I && this->instruction->getOpcode().to_string() == "0000011") {
        this->is_mem_read_asserted = true;
        this->is_mem_to_reg_asserted = true;
    }

    if (type == InstructionType::S) {
        this->is_mem_write_asserted = true;
    }

    if (type == InstructionType::HALT) {
        this->is_nop_asserted_flag = true;
        this->is_halt_instruction = true;
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
                this->alu_op = std::bitset<ALU_OP_BIT_COUNT>("0000");  // Add
            } else if (funct7 == std::bitset<Instruction::FUNCT7_BIT_COUNT>("0100000") &&
                    this->instruction->getOpcode() != std::bitset<Instruction::OPCODE_BIT_COUNT>("0010011")) {
                this->alu_op = std::bitset<ALU_OP_BIT_COUNT>("0001");  // Subtract
            }
        } else if (funct3 == std::bitset<Instruction::FUNCT3_BIT_COUNT>("100")) {
            this->alu_op = std::bitset<ALU_OP_BIT_COUNT>("0010");  // Xor
        } else if (funct3 == std::bitset<Instruction::FUNCT3_BIT_COUNT>("110")) {
            this->alu_op = std::bitset<ALU_OP_BIT_COUNT>("0011");  // Or
        } else if (funct3 == std::bitset<Instruction::FUNCT3_BIT_COUNT>("111")) {
            this->alu_op = std::bitset<ALU_OP_BIT_COUNT>("0100");  // And
        }
    } else if (type == InstructionType::J || type == InstructionType::S) {
        this->alu_op = std::bitset<ALU_OP_BIT_COUNT>("0000");  // Add
    } else if (type == InstructionType::B) {
        this->alu_op = std::bitset<ALU_OP_BIT_COUNT>("0001");  // Subtract
    } else {
        this->alu_op = std::bitset<ALU_OP_BIT_COUNT>("0000");  // Add (Default)
    }
}

void Control::setIsALUResultZero(bool is_result_zero) {
    this->is_alu_result_zero = is_result_zero;
}

void Control::toggleEXStageControlSignals() {
    if (!this->alu) {
        this->initDependencies();
    }

    this->alu->setALUOp(this->alu_op);

    this->ex_mux_alu_input_2->assertControlSignal(this->is_alu_src_asserted && !this->is_nop_asserted_flag);
    this->ex_mux_alu_input_1->assertJALCustomControlSignal(this->is_jal_instruction && !this->is_nop_asserted_flag);
    this->ex_mux_alu_input_2->assertJALCustomControlSignal(this->is_jal_instruction && !this->is_nop_asserted_flag);
}

void Control::toggleMEMStageControlSignals() {
    if (!this->if_mux) {
        this->initDependencies();
    }

    this->is_pc_src_asserted = this->is_pc_src_asserted && this->is_alu_result_zero && this->is_branch_instruction &&
            !this->is_nop_asserted_flag;

    this->if_mux->assertControlSignal(this->is_pc_src_asserted && !this->is_nop_asserted_flag);
    this->data_memory->setMemWrite(this->is_mem_write_asserted && !this->is_nop_asserted_flag);
    this->data_memory->setMemRead(this->is_mem_read_asserted && !this->is_nop_asserted_flag);

    std::thread pass_nop_if_id_registers_thread (
            &Control::passNopToIFIDStageRegisters,
            this,
            (this->is_pc_src_asserted || this->is_jal_instruction) && !this->is_nop_asserted_flag
    );

    std::thread pass_nop_id_ex_registers_thread (
            &Control::passNopToIDEXStageRegisters,
            this,
            (this->is_pc_src_asserted || this->is_jal_instruction) && !this->is_nop_asserted_flag
    );

    if ((this->is_pc_src_asserted || this->is_jal_instruction) && !this->is_nop_asserted_flag) {
        this->ex_mem_stage_registers->assertSystemEnabledNop();
    }

    pass_nop_if_id_registers_thread.detach();
    pass_nop_id_ex_registers_thread.detach();
}

void Control::toggleWBStageControlSignals() {
    if (!this->register_file) {
        this->initDependencies();
    }

    this->register_file->setRegWriteSignal(this->is_reg_write_asserted && !this->is_nop_asserted_flag);
    this->wb_mux->assertControlSignal(this->is_mem_to_reg_asserted && !this->is_nop_asserted_flag);
}

void Control::initDependencies() {
    this->register_file = RegisterFile::init();
    this->if_mux = IFMux::init();
    this->ex_mux_alu_input_1 = EXMuxALUInput1::init();
    this->ex_mux_alu_input_2 = EXMuxALUInput2::init();
    this->alu = ALU::init();
    this->data_memory = DataMemory::init();
    this->wb_mux = WBMux::init();
    this->if_id_stage_registers = IFIDStageRegisters::init();
    this->id_ex_stage_registers = IDEXStageRegisters::init();
    this->ex_mem_stage_registers = EXMEMStageRegisters::init();
}

void Control::setNop(bool is_asserted) {
    this->is_nop_asserted_flag = is_asserted;
}

Control *Control::deepCopy(Control *source) {
    if (!source) {
        return nullptr;
    }

    auto *deep_copy = new Control(source->instruction);
    if (source->is_nop_asserted_flag) {
        deep_copy->is_nop_asserted_flag = true;
    } else {
        deep_copy->is_nop_asserted_flag = false;
    }
    return deep_copy;
}

bool Control::isRegWriteAsserted() const {
    return this->is_reg_write_asserted && !this->is_nop_asserted_flag;
}

bool Control::isMemReadAsserted() const {
    return this->is_mem_read_asserted && !this->is_nop_asserted_flag;
}

void Control::passNopToIFIDStageRegisters(bool is_signal_asserted) {
    this->if_id_stage_registers->setNop(is_signal_asserted);
}

void Control::passNopToIDEXStageRegisters(bool is_signal_asserted) {
    this->id_ex_stage_registers->setNop(is_signal_asserted);
}

void Control::printState() {
    std::cout << "control::is_reg_write_asserted: " << this->is_reg_write_asserted << std::endl;
    std::cout << "control::is_pc_src_asserted: " << this->is_pc_src_asserted << std::endl;
    std::cout << "control::is_alu_src_asserted: " << this->is_alu_src_asserted << std::endl;
    std::cout << "control::is_mem_read_asserted: " << this->is_mem_read_asserted << std::endl;
    std::cout << "control::is_mem_write_asserted: " << this->is_mem_write_asserted << std::endl;
    std::cout << "control::is_mem_to_reg_asserted: " << this->is_mem_to_reg_asserted << std::endl;
    std::cout << "control::is_branch_instruction: " << this->is_branch_instruction << std::endl;
    std::cout << "control::is_alu_result_zero: " << this->is_alu_result_zero << std::endl;
    std::cout << "control::is_jal_instruction: " << this->is_jal_instruction << std::endl;
    std::cout << "control::is_halt_instruction: " << this->is_halt_instruction << std::endl;
    std::cout << "control::is_nop_asserted_flag: " << this->is_nop_asserted_flag << std::endl;
    std::cout << "control::alu_op: " << this->alu_op.to_string() << std::endl;
}
