#ifndef RISC_V_SIMULATOR_ALU_H
#define RISC_V_SIMULATOR_ALU_H

#include <bitset>

#include "../common/Module.h"
#include "../common/BitwiseOperations.h"
#include "../common/StageSynchronizer.h"
#include "../common/Control.h"
#include "../common/Logger.h"

class EXMEMStageRegisters;
class Logger;
class StageSynchronizer;

class ALU: public Module {
public:
    static constexpr int ALU_OP_BIT_COUNT = 4;

private:
    std::bitset<WORD_BIT_COUNT> input1;
    std::bitset<WORD_BIT_COUNT> input2;
    std::bitset<WORD_BIT_COUNT> result;

    bool is_result_zero;
    bool is_input1_set;
    bool is_input2_set;
    bool is_alu_op_set;

    bool is_reset_flag_set;

    std::bitset<ALU_OP_BIT_COUNT> alu_op;

    static ALU *current_instance;
    static std::mutex initialization_mutex;

    EXMEMStageRegisters *ex_mem_stage_registers;
    StageSynchronizer *stage_synchronizer;

public:
    ALU();

    static ALU *init();

    void run() override;

    void reset();

    void setInput1(std::bitset<WORD_BIT_COUNT> value);
    void setInput2(std::bitset<WORD_BIT_COUNT> value);

    void setALUOp(std::bitset<ALU_OP_BIT_COUNT> value);

private:
    void computeResult();
    void passZeroFlagToEXMEMStageRegisters(bool is_flag_asserted);
    void passResultToEXMEMStageRegisters(std::bitset<WORD_BIT_COUNT> data);
    void initDependencies() override;

    void resetState();

    std::string getModuleTag() override;
    Stage getModuleStage() override;
};

#endif //RISC_V_SIMULATOR_ALU_H
