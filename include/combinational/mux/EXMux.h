#ifndef RISC_V_SIMULATOR_EXMUX_H
#define RISC_V_SIMULATOR_EXMUX_H

#include "Mux.h"
#include "../../common/Config.h"
#include "../ALU.h"

#include <iostream>

class ALU;

class EXMux: protected Mux {
    unsigned long immediate;
    unsigned long read_data_2;

    bool is_immediate_set;
    bool is_read_data_2_set;

    bool is_alu_src_asserted;

    ALU *alu;

    static EXMux *current_instance;

public:
    EXMux();

    static EXMux *init();

    void run() override;
    void notifyModuleConditionVariable() override;
    void setInput(StageMuxInputType type, unsigned long value) override;
    void assertControlSignal(bool is_asserted) override;

protected:
    void passOutput() override;
};

EXMux *EXMux::current_instance = nullptr;

#endif //RISC_V_SIMULATOR_EXMUX_H
