#ifndef RISC_V_SIMULATOR_CONTROL_H
#define RISC_V_SIMULATOR_CONTROL_H

#include "Module.h"
#include "Instruction.h"
#include "../state/RegisterFile.h"
#include "../combinational/mux/IFMux.h"

class Control {
    const Instruction *instruction;

    RegisterFile *register_file;
    IFMux *if_mux;

    bool is_reg_write_asserted;
    bool is_pc_src_asserted;
    bool is_alu_src_asserted;
    bool is_mem_read_asserted;
    bool is_mem_write_asserted;
    bool is_mem_to_reg_asserted;

public:
    explicit Control(const Instruction *instruction);

    void setEXStageControlSignals();
    void setMEMStageControlSignals();
    void setWBStageControlSignals();

private:
    void generateSignals();
};

#endif //RISC_V_SIMULATOR_CONTROL_H
