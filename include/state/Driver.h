#ifndef RISC_V_SIMULATOR_DRIVER_H
#define RISC_V_SIMULATOR_DRIVER_H

#include "InstructionMemory.h"
#include "stage-registers/IFIDStageRegisters.h"
#include "../combinational/adder/IFAdder.h"
#include "../common/Module.h"
#include "../common/logger/IFLogger.h"
#include "../common/StageSynchronizer.h"

class InstructionMemory;
class IFIDStageRegisters;
class IFAdder;
class StageSynchronizer;

class Driver: public Module {
    unsigned long program_counter;
    bool is_new_program_counter_set;

    bool is_nop_asserted;
    bool is_reset_flag_set;
    bool is_pause_flag_set;

    InstructionMemory *instruction_memory;
    IFIDStageRegisters *if_id_stage_registers;
    IFAdder *if_adder;
    IFLogger *logger;
    StageSynchronizer *stage_synchronizer;

public:
    Driver();
    void setProgramCounter(unsigned long value);

    static Driver *current_instance;
    static Driver *init();

    void run() override;
    void notifyModuleConditionVariable() override;
    void setNop();
    void reset();
    void changeStageAndReset(Stage new_stage);
    void pause();
    void resume();

private:
    void passProgramCounterToInstructionMemory();
    void passProgramCounterToAdder();
    void passProgramCounterToIFIDStageRegisters();

    void resetStage();
};

#endif //RISC_V_SIMULATOR_DRIVER_H
