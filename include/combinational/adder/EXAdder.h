#ifndef RISC_V_SIMULATOR_EXADDER_H
#define RISC_V_SIMULATOR_EXADDER_H

#include <bitset>
#include <iostream>

#include "AdderBase.h"
#include "../../common/Config.h"
#include "../../state/stage-registers/EXMEMStageRegisters.h"
#include "../../common/Logger.h"

class EXMEMStageRegisters;

class EXAdder: public AdderBase {
    static constexpr int WORD_BIT_COUNT = 32;

    unsigned long program_counter;
    std::bitset<WORD_BIT_COUNT> immediate;
    unsigned long result;

    bool is_program_counter_set;
    bool is_immediate_set;

    EXMEMStageRegisters *ex_mem_stage_registers;

    static EXAdder *current_instance;
    static std::mutex initialization_mutex;

public:
    EXAdder();
    static EXAdder* init();

    void run() override;
    void setInput(const AdderInputType &type, const AdderInputDataType &value) override;

private:
    void computeResult();
    void passBranchAddressToEXMEMStageRegisters();
    void initDependencies() override;

    std::string getModuleTag() override;
    Stage getModuleStage() override;
};

#endif //RISC_V_SIMULATOR_EXADDER_H
