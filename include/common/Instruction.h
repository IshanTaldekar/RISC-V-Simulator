#ifndef RISC_V_SIMULATOR_INSTRUCTION_H
#define RISC_V_SIMULATOR_INSTRUCTION_H

#include <bitset>

class Instruction {
public:
    static constexpr int FUNCT7_BIT_COUNT = 7;
    static constexpr int FUNCT3_BIT_COUNT = 3;
    static constexpr int REGISTER_BIT_COUNT = 5;
    static constexpr int OPCODE_BIT_COUNT = 7;
    static constexpr int IMMEDIATE_BIT_COUNT = 12;

private:
    std::bitset<FUNCT7_BIT_COUNT> funct7;
    std::bitset<FUNCT3_BIT_COUNT> funct3;
    std::bitset<REGISTER_BIT_COUNT> rs1;
    std::bitset<REGISTER_BIT_COUNT> rs2;
    std::bitset<REGISTER_BIT_COUNT> rd;
    std::bitset<IMMEDIATE_BIT_COUNT> immediate;
    std::bitset<OPCODE_BIT_COUNT> opcode;
};

#endif //RISC_V_SIMULATOR_INSTRUCTION_H
