#ifndef RISC_V_SIMULATOR_CONTROL_H
#define RISC_V_SIMULATOR_CONTROL_H

#include "Module.h"
#include "Instruction.h"
#include "../state/RegisterFile.h"
#include "../combinational/mux/IFMux.h"
#include "../combinational/mux/WBMux.h"
#include "../combinational/mux/EXMuxALUInput2.h"
#include "../combinational/mux/EXMuxALUInput1.h"
#include "../combinational/ALU.h"
#include "../state/DataMemory.h"
#include "../state/stage-registers/IFIDStageRegisters.h"
#include "../state/stage-registers/IDEXStageRegisters.h"
#include "../state/stage-registers/EXMEMStageRegisters.h"

#include <bitset>

class RegisterFile;
class IFMux;
class EXMuxALUInput1;
class EXMuxALUInput2;
class ALU;
class DataMemory;
class WBMux;
class IFIDStageRegisters;
class IDEXStageRegisters;
class EXMEMStageRegisters;

class Control {
public:
    static constexpr int ALU_OP_BIT_COUNT = 4;

private:
    Instruction *instruction;

    RegisterFile *register_file;
    IFMux *if_mux;
    EXMuxALUInput1 *ex_mux_alu_input_1;
    EXMuxALUInput2 *ex_mux_alu_input_2;
    ALU *alu;
    DataMemory *data_memory;
    WBMux *wb_mux;
    IFIDStageRegisters *if_id_stage_registers;
    IDEXStageRegisters *id_ex_stage_registers;
    EXMEMStageRegisters *ex_mem_stage_registers;

    bool is_reg_write_asserted;
    bool is_pc_src_asserted;
    bool is_alu_src_asserted;
    bool is_mem_read_asserted;
    bool is_mem_write_asserted;
    bool is_mem_to_reg_asserted;
    bool is_branch_instruction;
    bool is_alu_result_zero;
    bool is_jal_instruction;

    std::bitset<ALU_OP_BIT_COUNT> alu_op;

public:
    explicit Control(Instruction *instruction);

    void setIsALUResultZero(bool is_result_zero);

    void toggleEXStageControlSignals();
    void toggleMEMStageControlSignals();
    void toggleWBStageControlSignals();

private:
    void generateSignals();
    void generateALUOpCode();
    void initDependencies();
};

#endif //RISC_V_SIMULATOR_CONTROL_H
