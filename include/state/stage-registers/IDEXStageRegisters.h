#ifndef RISC_V_SIMULATOR_IDEXSTAGEREGISTERS_H
#define RISC_V_SIMULATOR_IDEXSTAGEREGISTERS_H

#include "../../common/Module.h"
#include "../../common/Instruction.h"
#include "../../common/Control.h"
#include "../../combinational/adder/EXAdder.h"
#include "../../combinational/mux/EXMux.h"
#include "../../combinational/ALU.h"
#include "EXMEMStageRegisters.h"
#include "../../common/StageSynchronizer.h"

#include <bitset>

class Control;
class Instruction;
class EXMux;
class EXAdder;
class ALU;
class EXMEMStageRegisters;
class StageSynchronizer;

class IDEXStageRegisters: public Module {
    static constexpr int WORD_BIT_COUNT = 32;

    Control *control;
    Instruction *instruction;

    std::bitset<WORD_BIT_COUNT> read_data_1;
    std::bitset<WORD_BIT_COUNT> read_data_2;
    std::bitset<WORD_BIT_COUNT> immediate;

    unsigned long register_destination;
    unsigned long program_counter;

    static IDEXStageRegisters *current_instance;

    bool is_single_read_register_data_set;
    bool is_double_read_register_data_set;
    bool is_immediate_set;
    bool is_register_destination_set;
    bool is_program_counter_set;
    bool is_control_set;

    bool is_nop_asserted;
    bool is_reset_flag_set;
    bool is_pause_flag_set;

    EXMux *ex_mux;
    EXAdder *ex_adder;
    ALU *alu;
    EXMEMStageRegisters *ex_mem_stage_register;
    MEMWBStageRegisters *mem_wb_stage_register;
    StageSynchronizer *stage_synchronizer;

public:
    IDEXStageRegisters();

    static IDEXStageRegisters *init();

    void run() override;
    void notifyModuleConditionVariable() override;

    void setRegisterData(const std::bitset<WORD_BIT_COUNT> &rd1);
    void setRegisterData(const std::bitset<WORD_BIT_COUNT> &rd1, const std::bitset<WORD_BIT_COUNT> &rd2);
    void setImmediate(const std::bitset<WORD_BIT_COUNT> &imm);
    void setRegisterDestination(unsigned long rd);
    void setProgramCounter(int pc);
    void setControlModule(Control *new_control);
    void setInstruction(Instruction *current_instruction);
    void setNop();

    void reset();
    void pause();

private:
    void passProgramCounterToEXAdder();
    void passReadData1ToALU();
    void passReadData2ToExMux();
    void passImmediateToEXMux();
    void passImmediateToEXAdder();
    void passRegisterDestinationToEXMEMStageRegisters();
    void passReadData2ToEXMEMStageRegisters();
    void passControlToEXMEMStageRegisters();

    void attemptForwarding();

    void resetStage();
    void pauseStage();
};

#endif //RISC_V_SIMULATOR_IDEXSTAGEREGISTERS_H
