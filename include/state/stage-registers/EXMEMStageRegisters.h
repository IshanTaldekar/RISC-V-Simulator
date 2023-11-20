#ifndef RISC_V_SIMULATOR_EXMEMSTAGEREGISTERS_H
#define RISC_V_SIMULATOR_EXMEMSTAGEREGISTERS_H

#include "../../common/Module.h"
#include "../../common/Control.h"
#include "../../combinational/mux/IFMux.h"
#include "MEMWBStageRegisters.h"
#include "../../common/StageSynchronizer.h"

class Control;
class DataMemory;
class MEMWBStageRegisters;
class IFMux;
class StageSynchronizer;

class EXMEMStageRegisters: public Module {
    unsigned long branch_program_counter;
    unsigned long alu_result;
    unsigned long read_data_2;
    unsigned long register_destination;

    bool is_alu_result_zero;

    bool is_branch_program_counter_set;
    bool is_alu_result_set;
    bool is_read_data_2_set;
    bool is_register_destination_set;
    bool is_alu_result_zero_flag_set;
    bool is_control_set;
    bool is_nop_asserted;

    Control *control;
    DataMemory *data_memory;
    MEMWBStageRegisters *mem_wb_stage_registers;
    IFMux *if_mux;
    StageSynchronizer *stage_synchronizer;

    static EXMEMStageRegisters *current_instance;

public:
    EXMEMStageRegisters();

    static EXMEMStageRegisters *init();

    void run() override;
    void notifyModuleConditionVariable() override;

    void setBranchedProgramCounter(unsigned long value);
    void setALUResult(unsigned long value);
    void setIsResultZeroFlag(bool asserted);
    void setReadData2(unsigned long value);
    void setRegisterDestination(unsigned long value);
    void setControl(Control *new_control);
    void setNop();

private:
    void passALUResultToDataMemory();
    void passALUResultToMEMWBStageRegisters();
    void passWriteDataToDataMemory();
    void passRegisterDestinationToMEMWBStageRegisters();
    void passBranchedAddressToIFMux();
};

#endif //RISC_V_SIMULATOR_EXMEMSTAGEREGISTERS_H
