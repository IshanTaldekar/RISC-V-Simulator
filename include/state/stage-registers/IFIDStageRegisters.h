#ifndef RISC_V_SIMULATOR_IFIDSTAGEREGISTERS_H
#define RISC_V_SIMULATOR_IFIDSTAGEREGISTERS_H

#include "../../common/Config.h"
#include "../../common/Module.h"
#include "../../common/Logger.h"
#include "../../state/stage-registers/IDEXStageRegisters.h"
#include "../../common/Instruction.h"
#include "../../common/Control.h"
#include "../../state/RegisterFile.h"
#include "../../combinational/ImmediateGenerator.h"
#include "../../combinational/HazardDetectionUnit.h"
#include "../../common/StageSynchronizer.h"

#include <bitset>
#include <iostream>
#include <queue>

class Control;
class RegisterFile;
class ImmediateGenerator;
class StageSynchronizer;
class IDEXStageRegisters;
class Logger;
class HazardDetectionUnit;

class IFIDStageRegisters: public Module {
    static constexpr int WORD_BIT_COUNT = 32;

    unsigned long program_counter;

    std::string instruction_bits;

    Instruction *instruction;
    Control *control;

    IDEXStageRegisters *id_ex_stage_registers;
    RegisterFile *register_file;
    ImmediateGenerator *immediate_generator;
    StageSynchronizer *stage_synchronizer;
    HazardDetectionUnit *hazard_detection_unit;

    bool is_program_counter_set;
    bool is_instruction_set;
    bool is_nop_flag_set;

    bool is_nop_passed_flag_asserted;
    bool is_nop_passed_flag_set;

    bool is_nop_asserted;
    bool is_reset_flag_set;
    bool is_pause_flag_set;

    bool is_verbose_execution_flag_asserted;

    static IFIDStageRegisters *current_instance;
    static std::mutex initialization_mutex;

    static constexpr int REQUIRED_NOP_FLAG_SET_OPERATIONS = 2;
    int current_nop_set_operations;

public:
    IFIDStageRegisters();

    static IFIDStageRegisters *init();

    void run() override;

    void setInput(std::variant<unsigned long, std::string> input);
    void assertSystemEnabledNop();  // System asserted NOP
    void setNop(bool is_asserted);
    void setPassedNop(bool is_asserted);  // NOP passed from previous stage

    void reset();
    void pause();
    void resume();
    void changeStageAndReset(PipelineType new_stage);

    void assertVerboseExecutionFlag();

private:
    void passProgramCounterToIDEXStageRegisters(unsigned long pc);
    void passControlToIDEXStageRegisters(Control *new_control);
    void passReadRegistersToRegisterFile(Instruction *current_instruction);
    void passInstructionToImmediateGenerator(Instruction *current_instruction);
    void passRegisterDestinationToIDEXStageRegisters(Instruction *instruction);
    void passRegisterSource1ToIDEXStageRegisters(Instruction *instruction);
    void passRegisterSource2ToIDEXStageRegisters(Instruction *instruction);
    void passInstructionToIDEXStageRegisters(Instruction *instruction);
    void passNopToIDEXStageRegisters(bool is_asserted);
    void passInstructionToHazardDetectionUnit(Instruction *next_instruction);

    void resetStage();
    void initDependencies() override;

    void delayUpdateUntilNopFlagSet();

    std::string getModuleTag() override;
    Stage getModuleStage() override;

    void printState();
};

#endif //RISC_V_SIMULATOR_IFIDSTAGEREGISTERS_H
