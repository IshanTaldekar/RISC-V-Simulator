#ifndef RISC_V_SIMULATOR_IDEXSTAGEREGISTERS_H
#define RISC_V_SIMULATOR_IDEXSTAGEREGISTERS_H

#include "../../common/Module.h"
#include "../../common/Instruction.h"
#include "../../common/Control.h"
#include "../../combinational/adder/EXAdder.h"
#include "../../combinational/mux/EXMuxALUInput1.h"
#include "../../combinational/mux/EXMuxALUInput2.h"
#include "../../combinational/ALU.h"
#include "EXMEMStageRegisters.h"
#include "../../common/StageSynchronizer.h"
#include "../../common/Logger.h"
#include "../../combinational/ForwardingUnit.h"

#include <bitset>

class Control;
class Instruction;
class EXMuxALUInput2;
class EXMuxALUInput1;
class EXAdder;
class EXMEMStageRegisters;
class StageSynchronizer;
class ForwardingUnit;
class Logger;

class IDEXStageRegisters: public Module {
    static constexpr int WORD_BIT_COUNT = 32;

    Control *control;
    Instruction *instruction;

    std::bitset<WORD_BIT_COUNT> read_data_1;
    std::bitset<WORD_BIT_COUNT> read_data_2;
    std::bitset<WORD_BIT_COUNT> immediate;

    unsigned long register_source1;
    unsigned long register_source2;
    unsigned long register_destination;
    unsigned long program_counter;

    static IDEXStageRegisters *current_instance;

    bool is_single_read_register_data_set;
    bool is_double_read_register_data_set;
    bool is_immediate_set;
    bool is_register_destination_set;
    bool is_register_source1_set;
    bool is_register_source2_set;
    bool is_program_counter_set;
    bool is_control_set;
    bool is_instruction_set;

    bool is_nop_asserted;
    bool is_reset_flag_set;
    bool is_pause_flag_set;

    EXMuxALUInput2 *ex_mux_alu_input_2;
    EXMuxALUInput1 *ex_mux_alu_input_1;
    EXAdder *ex_adder;
    EXMEMStageRegisters *ex_mem_stage_register;
    StageSynchronizer *stage_synchronizer;
    ForwardingUnit *forwarding_unit;
    Logger *logger;

public:
    IDEXStageRegisters();

    static IDEXStageRegisters *init();

    void run() override;

    void setRegisterData(const std::bitset<WORD_BIT_COUNT> &rd1);
    void setRegisterData(const std::bitset<WORD_BIT_COUNT> &rd1, const std::bitset<WORD_BIT_COUNT> &rd2);
    void setImmediate(const std::bitset<WORD_BIT_COUNT> &imm);
    void setRegisterDestination(unsigned long rd);
    void setProgramCounter(unsigned long pc);
    void setControlModule(Control *new_control);
    void setRegisterSource1(unsigned long rs1);
    void setRegisterSource2(unsigned long rs2);
    void setInstruction(Instruction *current_instruction);

    void assertNop();
    void reset();
    void pause();
    void resume();
    void changeStageAndReset(PipelineType new_pipeline_type);

private:
    void passProgramCounterToEXAdder();
    void passProgramCounterToEXMuxALUInput1();
    void passReadData1ToExMuxALUInput1();
    void passReadData2ToExMuxALUInput2();
    void passImmediateToEXMuxALUInput2();
    void passImmediateToEXAdder();
    void passRegisterDestinationToEXMEMStageRegisters();
    void passReadData2ToEXMEMStageRegisters();
    void passControlToEXMEMStageRegisters();
    void passRegisterSourceToForwardingUnit();

    void resetStage();
};

#endif //RISC_V_SIMULATOR_IDEXSTAGEREGISTERS_H
