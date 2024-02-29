#ifndef RISC_V_SIMULATOR_ALU_H
#define RISC_V_SIMULATOR_ALU_H

#include <bitset>

#include "../common/Module.h"
#include "../common/Control.h"
#include "../common/Logger.h"

class EXMEMStageRegisters;

class ALU: public Module {
public:
    static constexpr int ALU_OP_BIT_COUNT = 4;

private:
    unsigned long input1;
    unsigned long input2;
    unsigned long result;

    bool is_result_zero;
    bool is_input1_set;
    bool is_input2_set;
    bool is_alu_op_set;

    std::bitset<ALU_OP_BIT_COUNT> alu_op;

    static ALU *current_instance;

    EXMEMStageRegisters *ex_mem_stage_registers;
    Logger *logger;

public:
    ALU();

    static ALU *init();

    void run() override;

    void setInput1(unsigned long value);
    void setInput2(unsigned long value);

    void setALUOp(const std::bitset<ALU_OP_BIT_COUNT> &value);

private:
    void computeResult();
    void passZeroFlagToEXMEMStageRegisters();
    void passResultToEXMEMStageRegisters();
    void initDependencies() override;
};

#endif //RISC_V_SIMULATOR_ALU_H
