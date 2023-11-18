#ifndef RISC_V_SIMULATOR_IMMEDIATEGENERATOR_H
#define RISC_V_SIMULATOR_IMMEDIATEGENERATOR_H

#include "../common/Module.h"
#include "../common/Instruction.h"
#include "../common/Config.h"
#include "../state/stage-registers/IDEXStageRegisters.h"

class Instruction;
class IDEXStageRegisters;

class ImmediateGenerator: public Module {
    static constexpr int WORD_BIT_COUNT = 32;

    static ImmediateGenerator *current_instance;

    const Instruction *instruction;
    IDEXStageRegisters *id_ex_stage_registers;

    bool is_instruction_set;

public:
    ImmediateGenerator();

    static ImmediateGenerator *init();

    void run() override;
    void notifyModuleConditionVariable() override;

    void setInstruction(const Instruction *current_instruction);

private:
    void loadImmediateToIDEXStageRegisters();
};

ImmediateGenerator *ImmediateGenerator::current_instance = nullptr;

#endif //RISC_V_SIMULATOR_IMMEDIATEGENERATOR_H
