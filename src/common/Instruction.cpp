#include "../../include/common/Instruction.h"

Instruction::Instruction(std::string instruction) {
    this->instruction = std::move(instruction);

    this->opcode_to_type_mapping = std::unordered_map<std::string, InstructionType>{
            {"0110011", InstructionType::R},
            {"0010011", InstructionType::I},
            {"1101111", InstructionType::J},
            {"1100011", InstructionType::B},
            {"0000011", InstructionType::I},
            {"0100011", InstructionType::S},
            {"1111111", InstructionType::HALT},
            {"0000000", InstructionType::CUSTOM}
    };
}

std::bitset<Instruction::FUNCT7_BIT_COUNT> Instruction::getFunct7() const {  // bits 31 - 25
    return std::bitset<FUNCT7_BIT_COUNT>(this->instruction.substr(FUNCT7_BASE_STRING_INDEX, FUNCT7_BIT_COUNT));
}

std::bitset<Instruction::FUNCT3_BIT_COUNT> Instruction::getFunct3() const {  // bits 14 - 12
    return std::bitset<FUNCT3_BIT_COUNT>(this->instruction.substr(FUNCT3_BASE_STRING_INDEX, FUNCT3_BIT_COUNT));
}

std::bitset<Instruction::REGISTER_BIT_COUNT> Instruction::getRs1() const {  // bits 19 - 15
    return std::bitset<REGISTER_BIT_COUNT>(this->instruction.substr(RS1_BASE_STRING_INDEX, REGISTER_BIT_COUNT));
}

std::bitset<Instruction::REGISTER_BIT_COUNT> Instruction::getRs2() const {  // bits 24 - 20
    return std::bitset<REGISTER_BIT_COUNT>(this->instruction.substr(RS2_BASE_STRING_INDEX, REGISTER_BIT_COUNT));
}

std::bitset<Instruction::REGISTER_BIT_COUNT> Instruction::getRd() const {  // bits 11 - 7
    return std::bitset<REGISTER_BIT_COUNT>(this->instruction.substr(RD_BASE_STRING_INDEX, REGISTER_BIT_COUNT));
}

std::bitset<Instruction::OPCODE_BIT_COUNT> Instruction::getOpcode() const {  // bits 6 - 0
    return std::bitset<OPCODE_BIT_COUNT>(this->instruction.substr(OPCODE_BASE_STRING_INDEX, OPCODE_BIT_COUNT));
}

std::bitset<Instruction::GENERAL_IMMEDIATE_BIT_COUNT> Instruction::getImmediate() const {
    std::vector<std::pair<int, int>> segments;

    if (this->getType() == InstructionType::I) {
        segments = I_TYPE_IMMEDIATE_SEGMENTS;
    } else if (this->getType() == InstructionType::S || this->getType() == InstructionType::B) {
        segments = SB_TYPE_IMMEDIATE_SEGMENTS;
    }

    std::string immediate;

    for (const std::pair<int, int> &segment: segments) {
        immediate += this->instruction.substr(segment.first, segment.second);
    }

    return std::bitset<GENERAL_IMMEDIATE_BIT_COUNT>(immediate);
}

std::bitset<Instruction::J_TYPE_IMMEDIATE_BIT_COUNT> Instruction::getImmediateJ() const {
    std::vector<std::pair<int, int>> segments;

    if (this->getType() == InstructionType::J) {
        segments = J_TYPE_IMMEDIATE_SEGMENTS;
    }

    std::string immediate;

    for (const std::pair<int, int> &segment: segments) {
        immediate += this->instruction.substr(segment.first, segment.second);
    }

    return std::bitset<J_TYPE_IMMEDIATE_BIT_COUNT>(immediate);
}

InstructionType Instruction::getType() const {
    const auto element_iterator = this->opcode_to_type_mapping.find(this->getOpcode().to_string());

    if (element_iterator == this->opcode_to_type_mapping.end()) {
        std::cerr << "[Instruction] Unknown opcode" << std::endl;
        return InstructionType::UNKNOWN;
    }

    return element_iterator->second;
}

Instruction *Instruction::deepCopy(const Instruction *source) {
    if (!source) {
        return nullptr;
    }

    return new Instruction(source->instruction);
}
