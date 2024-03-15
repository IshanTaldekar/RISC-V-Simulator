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
#include "../../combinational/HazardDetectionUnit.h"

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
class HazardDetectionUnit;

class IDEXStageRegisters: public Module {
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
    static std::mutex initialization_mutex;

    bool is_single_read_register_data_set;
    bool is_double_read_register_data_set;
    bool is_immediate_set;
    bool is_register_destination_set;
    bool is_register_source1_set;
    bool is_register_source2_set;
    bool is_program_counter_set;
    bool is_control_set;
    bool is_instruction_set;
    bool is_nop_passed_flag_set;
    bool is_nop_asserted;
    bool is_reset_flag_set;
    bool is_pause_flag_set;
    bool is_nop_flag_set;
    bool is_nop_passed_flag_asserted;

    EXMuxALUInput2 *ex_mux_alu_input_2;
    EXMuxALUInput1 *ex_mux_alu_input_1;
    EXAdder *ex_adder;
    EXMEMStageRegisters *ex_mem_stage_register;
    StageSynchronizer *stage_synchronizer;
    ForwardingUnit *forwarding_unit;
    Logger *logger;
    HazardDetectionUnit *hazard_detection_unit;

    static constexpr int REQUIRED_NOP_FLAG_SET_OPERATIONS = 2;
    int current_nop_set_operations{};

public:
    IDEXStageRegisters();

    static IDEXStageRegisters *init();

    void run() override;

    void setRegisterData(std::bitset<WORD_BIT_COUNT> reg_data);
    void setRegisterData(std::bitset<WORD_BIT_COUNT> reg_data1, std::bitset<WORD_BIT_COUNT> reg_data2);
    void setImmediate(std::bitset<WORD_BIT_COUNT> imm);
    void setRegisterDestination(unsigned long rd);
    void setProgramCounter(unsigned long pc);
    void setControlModule(Control *new_control);
    void setRegisterSource1(unsigned long rs1);
    void setRegisterSource2(unsigned long rs2);
    void setInstruction(Instruction *current_instruction);
    void setPassedNop(bool is_asserted);  // Pass Nop by previous stages
    void setNop(bool is_asserted);  // Nop set by operations
    void assertSystemEnabledNop(); // assert Nop by system

    void reset();
    void pause();
    void resume();
    void changeStageAndReset(PipelineType new_pipeline_type);

private:
    void passProgramCounterToEXAdder(unsigned long pc);
    void passProgramCounterToEXMuxALUInput1(unsigned long pc);
    void passReadData1ToExMuxALUInput1(std::bitset<WORD_BIT_COUNT> data);
    void passReadData2ToExMuxALUInput2(std::bitset<WORD_BIT_COUNT> data);
    void passImmediateToEXMuxALUInput2(std::bitset<WORD_BIT_COUNT> imm);
    void passImmediateToEXAdder(std::bitset<WORD_BIT_COUNT> imm);
    void passRegisterDestinationToEXMEMStageRegisters(unsigned long rd);
    void passReadData2ToEXMEMStageRegisters(std::bitset<WORD_BIT_COUNT> data);
    void passControlToEXMEMStageRegisters(Control *current_control);
    void passRegisterSourceToForwardingUnit(bool single_register_used, unsigned long rs1, unsigned long rs2);
    void passNopToEXMEMStageRegisters(bool is_signal_asserted);
    void passMemReadToHazardDetectionUnit(bool is_signal_asserted);
    void passRegisterDestinationToHazardDetectionUnit(unsigned long rd);

    void resetStage();
    void initDependencies() override;

    void delayUpdateUntilNopFlagSet();
};

#endif //RISC_V_SIMULATOR_IDEXSTAGEREGISTERS_H
