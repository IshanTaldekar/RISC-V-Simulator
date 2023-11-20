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
    int program_counter;
    bool is_new_program_counter_set;

    bool is_nop_asserted;

    InstructionMemory *instruction_memory;
    IFIDStageRegisters *if_id_stage_registers;
    IFAdder *if_adder;
    IFLogger *logger;
    StageSynchronizer *stage_synchronizer;

public:
    Driver();
    void setProgramCounter(int value);

    static Driver *current_instance;
    static Driver *init();

    void run() override;
    void notifyModuleConditionVariable() override;
    void setNop();

private:
    void passProgramCounterToInstructionMemory();
    void passProgramCounterToAdder();
    void passProgramCounterToIFIDStageRegisters();
};

#endif //RISC_V_SIMULATOR_DRIVER_H
