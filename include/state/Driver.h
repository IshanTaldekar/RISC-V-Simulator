#ifndef RISC_V_SIMULATOR_DRIVER_H
#define RISC_V_SIMULATOR_DRIVER_H

#include "InstructionMemory.h"
#include "stage-registers/IFIDStageRegisters.h"
#include "../combinational/adder/IFAdder.h"
#include "../common/Module.h"
#include "../common/Logger.h"
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
    bool is_nop_flag_set;

    bool is_verbose_execution_flag_asserted;

    InstructionMemory *instruction_memory;
    IFIDStageRegisters *if_id_stage_registers;
    IFAdder *if_adder;
    StageSynchronizer *stage_synchronizer;

    static Driver *current_instance;
    static std::mutex initialization_mutex;

    static constexpr int REQUIRED_NOP_FLAG_SET_OPERATIONS = 2;
    int current_nop_set_operations;

public:
    Driver();
    void setProgramCounter(unsigned long value);

    static Driver *init();

    void run() override;
    void setNop(bool is_asserted);
    void assertSystemEnabledNop();
    void reset();
    void changeStageAndReset(PipelineType new_stage);
    void pause();
    void resume();

    void assertVerboseExecutionFlag();

private:
    void passProgramCounterToInstructionMemory(unsigned long pc);
    void passProgramCounterToIFAdder(unsigned long pc);
    void passProgramCounterToIFIDStageRegisters(unsigned long pc);
    void passNopToIFIDStageRegisters(bool is_asserted);

    void resetStage();
    void initDependencies() override;

    void delayUpdateUntilNopFlagSet();

    std::string getModuleTag() override;
    Stage getModuleStage() override;

    void printState() const;
};

#endif //RISC_V_SIMULATOR_DRIVER_H
