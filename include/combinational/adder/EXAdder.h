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
    unsigned long immediate;
    unsigned long result;

    bool is_program_counter_set;
    bool is_immediate_set;

    EXMEMStageRegisters *ex_mem_stage_registers;
    Logger *logger;

    static EXAdder *current_instance;

public:
    EXAdder();
    static EXAdder* init();

    void run() override;
    void setInput(AdderInputType type, unsigned long value) override;

private:
    void computeResult();
    void passBranchAddressToEXMEMStageRegisters();
};

#endif //RISC_V_SIMULATOR_EXADDER_H
