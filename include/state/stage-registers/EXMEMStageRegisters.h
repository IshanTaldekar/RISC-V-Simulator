#ifndef RISC_V_SIMULATOR_EXMEMSTAGEREGISTERS_H
#define RISC_V_SIMULATOR_EXMEMSTAGEREGISTERS_H

#include "../../common/Module.h"
#include "../../common/Control.h"
#include "../../common/Logger.h"
#include "../../combinational/mux/IFMux.h"
#include "MEMWBStageRegisters.h"
#include "../../common/StageSynchronizer.h"
#include "../../combinational/mux/forwarding/ALUInput1ForwardingMux.h"
#include "../../combinational/mux/forwarding/ALUInput2ForwardingMux.h"
#include "../../combinational/ForwardingUnit.h"
#include "../../common/Config.h"


class Control;
class DataMemory;
class MEMWBStageRegisters;
class IFMux;
class StageSynchronizer;
class ALUInput1ForwardingMux;
class ALUInput2ForwardingMux;
class ForwardingUnit;

class EXMEMStageRegisters: public Module {
    unsigned long branch_program_counter;
    unsigned long register_destination;

    std::bitset<WORD_BIT_COUNT> alu_result;
    std::bitset<WORD_BIT_COUNT> read_data_2;

    bool is_alu_result_zero;

    bool is_branch_program_counter_set;
    bool is_alu_result_set;
    bool is_read_data_2_set;
    bool is_register_destination_set;
    bool is_alu_result_zero_flag_set;
    bool is_control_set;
    bool is_nop_asserted;
    bool is_reset_flag_set;
    bool is_pause_flag_set;

    Control *control;
    DataMemory *data_memory;
    MEMWBStageRegisters *mem_wb_stage_registers;
    IFMux *if_mux;
    StageSynchronizer *stage_synchronizer;
    ALUInput1ForwardingMux *alu_input_1_forwarding_mux;
    ALUInput2ForwardingMux *alu_input_2_forwarding_mux;
    ForwardingUnit *forwarding_unit;
    Logger *logger;

    static EXMEMStageRegisters *current_instance;
    static std::mutex initialization_mutex;

public:
    EXMEMStageRegisters();

    static EXMEMStageRegisters *init();

    void run() override;

    void setBranchedProgramCounter(unsigned long value);
    void setALUResult(const std::bitset<WORD_BIT_COUNT> &value);
    void setIsResultZeroFlag(bool asserted);
    void setReadData2(const std::bitset<WORD_BIT_COUNT> &value);
    void setRegisterDestination(unsigned long value);
    void setControl(Control *new_control);

    void assertNop();
    void reset();
    void pause();
    void resume();
    void changeStageAndReset(PipelineType new_pipeline_type);

private:
    void passALUResultToDataMemory();
    void passALUResultToALUInput1ForwardingMux();
    void passALUResultToALUInput2ForwardingMux();
    void passALUResultToMEMWBStageRegisters();
    void passWriteDataToDataMemory();
    void passRegisterDestinationToMEMWBStageRegisters();
    void passRegisterDestinationToForwardingUnit();
    void passBranchedAddressToIFMux();
    void passControlToMEMWBStageRegisters();

    void resetStage();
    void initDependencies() override;
};

#endif //RISC_V_SIMULATOR_EXMEMSTAGEREGISTERS_H
