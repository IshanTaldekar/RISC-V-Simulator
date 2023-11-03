#ifndef RISC_V_SIMULATOR_IFIDSTAGEREGISTERS_H
#define RISC_V_SIMULATOR_IFIDSTAGEREGISTERS_H

#include "../../common/Config.h"
#include "../../common/Module.h"
#include "../../common/logger/IFLogger.h"
#include "../../common/logger/IDLogger.h"
#include "../../state/stage-registers/IDEXStageRegisters.h"
#include "../../common/Instruction.h"
#include "../../common/Control.h"
#include "../../state/RegisterFile.h"
#include "../../combinational/ImmediateGenerator.h"

#include <bitset>
#include <iostream>
#include <queue>

class IFIDStageRegisters: public Module {
    static constexpr int WORD_BIT_COUNT = 32;

    int program_counter;

    std::string instruction_bits;
    Instruction *instruction;

    Control *control;

    IFLogger *if_logger;
    IDLogger *id_logger;

    IDEXStageRegisters *id_ex_stage_registers;
    RegisterFile *register_file;
    ImmediateGenerator *immediate_generator;

    bool is_program_counter_set;
    bool is_instruction_set;

public:
    IFIDStageRegisters();

    static IFIDStageRegisters *current_instance;
    static IFIDStageRegisters *init();

    void run() override;
    void notifyModuleConditionVariable() override;

    void setInput(std::variant<int, std::string> input);

private:
    void passProgramCounterToIDEXStageRegisters();
    void passControlToIDEXStageRegisters();
    void passReadRegistersToRegisterFile();
    void passInstructionToImmediateGenerator();
    void passRegisterDestinationToIDEXStageRegisters();
};

#endif //RISC_V_SIMULATOR_IFIDSTAGEREGISTERS_H
