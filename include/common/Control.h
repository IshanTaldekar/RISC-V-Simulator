#ifndef RISC_V_SIMULATOR_CONTROL_H
#define RISC_V_SIMULATOR_CONTROL_H

#include "Module.h"
#include "Instruction.h"
#include "../state/RegisterFile.h"
#include "../combinational/mux/IFMux.h"
#include "../combinational/mux/WBMux.h"
#include "../combinational/mux/EXMux.h"
#include "../combinational/ALU.h"
#include "../state/DataMemory.h"

#include <bitset>

class RegisterFile;
class IFMux;
class EXMux;
class ALU;
class DataMemory;
class WBMux;

class Control {
public:
    static constexpr int ALU_OP_BIT_COUNT = 4;

private:
    const Instruction *instruction;

    RegisterFile *register_file;
    IFMux *if_mux;
    EXMux *ex_mux;
    ALU *alu;
    DataMemory *data_memory;
    WBMux *wb_mux;

    bool is_reg_write_asserted;
    bool is_pc_src_asserted;
    bool is_alu_src_asserted;
    bool is_mem_read_asserted;
    bool is_mem_write_asserted;
    bool is_mem_to_reg_asserted;
    bool is_branch_instruction;
    bool is_alu_result_zero;

    std::bitset<ALU_OP_BIT_COUNT> alu_op;

public:
    explicit Control(const Instruction *instruction);

    void setIsALUResultZero(bool is_result_zero);

    void toggleEXStageControlSignals();
    void toggleMEMStageControlSignals();
    void toggleWBStageControlSignals();

private:
    void generateSignals();
    void generateALUOpCode();
};

#endif //RISC_V_SIMULATOR_CONTROL_H
