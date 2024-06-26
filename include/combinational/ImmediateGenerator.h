#ifndef RISC_V_SIMULATOR_IMMEDIATEGENERATOR_H
#define RISC_V_SIMULATOR_IMMEDIATEGENERATOR_H

#include "../common/Module.h"
#include "../common/Instruction.h"
#include "../common/Config.h"
#include "../common/Logger.h"
#include "../state/stage-registers/IDEXStageRegisters.h"

class Instruction;
class IDEXStageRegisters;
class Logger;

class ImmediateGenerator: public Module {
    static constexpr int WORD_BIT_COUNT = 32;

    static ImmediateGenerator *current_instance;
    static std::mutex initialization_mutex;

    const Instruction *instruction;

    IDEXStageRegisters *id_ex_stage_registers;

    bool is_instruction_set;

public:
    ImmediateGenerator();

    static ImmediateGenerator *init();

    void run() override;

    void setInstruction(const Instruction *current_instruction);

private:
    void loadImmediateToIDEXStageRegisters();
    void initDependencies() override;

    std::string getModuleTag() override;
    Stage getModuleStage() override;
};

#endif //RISC_V_SIMULATOR_IMMEDIATEGENERATOR_H
