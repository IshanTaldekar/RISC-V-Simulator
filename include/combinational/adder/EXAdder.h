#ifndef RISC_V_SIMULATOR_EXADDER_H
#define RISC_V_SIMULATOR_EXADDER_H

#include "Adder.h"
#include "../../common/Config.h"
#include "../../state/stage-registers/EXMEMStageRegisters.h"

#include <bitset>
#include <iostream>

class EXAdder: protected Adder {
    static constexpr int WORD_BIT_COUNT = 32;

    unsigned long program_counter;
    unsigned long immediate;
    unsigned long result;

    bool is_program_counter_set;
    bool is_immediate_set;

    EXMEMStageRegisters *ex_mem_stage_registers;

    static EXAdder *current_instance;

public:
    EXAdder();
    static EXAdder* init();

    void run() override;
    void setInput(AdderInputType type, unsigned long value) override;
    void notifyModuleConditionVariable() override;

private:
    void computeResult();
    void passBranchAddressToEXMEMStageRegisters();
};

EXAdder *EXAdder::current_instance = nullptr;

#endif //RISC_V_SIMULATOR_EXADDER_H
