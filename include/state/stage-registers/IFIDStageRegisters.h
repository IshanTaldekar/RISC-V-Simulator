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
#include "../../common/StageSynchronizer.h"

#include <bitset>
#include <iostream>
#include <queue>

class Control;
class RegisterFile;
class ImmediateGenerator;
class StageSynchronizer;
class Logger;

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
    Logger *logger;

    bool is_program_counter_set;
    bool is_instruction_set;

    bool is_nop_asserted;
    bool is_reset_flag_set;
    bool is_pause_flag_set;

    static IFIDStageRegisters *current_instance;
    static std::mutex initialization_mutex;

public:
    IFIDStageRegisters();

    static IFIDStageRegisters *init();

    void run() override;

    void setInput(const std::variant<unsigned long, std::string> &input);
    void assertNop();

    void reset();
    void pause();
    void resume();
    void changeStageAndReset(PipelineType new_stage);

    Instruction *getInstruction();

private:
    void passProgramCounterToIDEXStageRegisters();
    void passControlToIDEXStageRegisters();
    void passReadRegistersToRegisterFile();
    void passInstructionToImmediateGenerator();
    void passRegisterDestinationToIDEXStageRegisters();
    void passRegisterSource1ToIDEXStageRegisters();
    void passRegisterSource2ToIDEXStageRegisters();
    void passInstructionToIDEXStageRegisters();

    void resetStage();
    void initDependencies() override;
};

#endif //RISC_V_SIMULATOR_IFIDSTAGEREGISTERS_H
