#ifndef RISC_V_SIMULATOR_DRIVER_H
#define RISC_V_SIMULATOR_DRIVER_H

#include "InstructionMemory.h"
#include "stage-registers/IFIDStageRegisters.h"
#include "../combinational/adder/IFAdder.h"
#include "../common/Module.h"
#include "../common/logger/IFLogger.h"

class Driver: public Module {
    int program_counter;
    bool is_new_program_counter_set;

    InstructionMemory *instruction_memory;
    IFIDStageRegisters *if_id_stage_registers;
    IFAdder *if_adder;
    IFLogger *logger;

public:
    Driver();
    void setProgramCounter(int value);

    static Driver *current_instance;
    static Driver *init();

    void run() override;
    void notifyModuleConditionVariable() override;

private:
    void passProgramCounterToInstructionMemory();
    void passProgramCounterToAdder();
    void passProgramCounterToIFIDStageRegisters();
};

Driver *Driver::current_instance = nullptr;

#endif //RISC_V_SIMULATOR_DRIVER_H
