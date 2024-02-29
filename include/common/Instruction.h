#ifndef RISC_V_SIMULATOR_INSTRUCTION_H
#define RISC_V_SIMULATOR_INSTRUCTION_H

#include <bitset>
#include <string>
#include <vector>
#include <variant>
#include <unordered_map>
#include <iostream>

#include "Config.h"

class Instruction {
public:
    static constexpr int FUNCT7_BIT_COUNT = 7;
    static constexpr int FUNCT3_BIT_COUNT = 3;
    static constexpr int REGISTER_BIT_COUNT = 5;
    static constexpr int OPCODE_BIT_COUNT = 7;
    static constexpr int GENERAL_IMMEDIATE_BIT_COUNT = 12;
    static constexpr int J_TYPE_IMMEDIATE_BIT_COUNT = 20;

private:
    std::string instruction;
    std::unordered_map<std::string, InstructionType> opcode_to_type_mapping;

    static constexpr int FUNCT7_BASE_STRING_INDEX = 0;
    static constexpr int FUNCT3_BASE_STRING_INDEX = 17;
    static constexpr int RS1_BASE_STRING_INDEX = 12;
    static constexpr int RS2_BASE_STRING_INDEX = 7;
    static constexpr int RD_BASE_STRING_INDEX = 20;
    static constexpr int OPCODE_BASE_STRING_INDEX = 25;

    // segments contain pairs (start index of segment and segment size)
    const std::vector<std::pair<int, int>> I_TYPE_IMMEDIATE_SEGMENTS {{0, 12}};
    const std::vector<std::pair<int, int>> SB_TYPE_IMMEDIATE_SEGMENTS {{0, 7}, {20, 5}};
    const std::vector<std::pair<int, int>> J_TYPE_IMMEDIATE_SEGMENTS {{0, 20}};

public:
    explicit Instruction(std::string instruction);

    std::bitset<FUNCT7_BIT_COUNT> getFunct7() const;
    std::bitset<FUNCT3_BIT_COUNT> getFunct3() const;
    std::bitset<REGISTER_BIT_COUNT> getRs1() const ;
    std::bitset<REGISTER_BIT_COUNT> getRs2() const;
    std::bitset<REGISTER_BIT_COUNT> getRd() const;
    std::bitset<OPCODE_BIT_COUNT> getOpcode() const;
    std::bitset<GENERAL_IMMEDIATE_BIT_COUNT> getImmediate() const;
    std::bitset<J_TYPE_IMMEDIATE_BIT_COUNT> getImmediateJ() const;

    InstructionType getType() const;
};

#endif //RISC_V_SIMULATOR_INSTRUCTION_H
