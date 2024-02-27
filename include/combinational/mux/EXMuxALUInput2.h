#ifndef RISC_V_SIMULATOR_EXMUXALUINPUT2_H
#define RISC_V_SIMULATOR_EXMUXALUINPUT2_H

#include "MuxBase.h"
#include "../../common/Config.h"
#include "forwarding/ALUInput2ForwardingMux.h"

#include <iostream>

class MuxBase;
class ALUInput2ForwardingMux;

class EXMuxALUInput2: protected MuxBase {
    unsigned long immediate;
    unsigned long read_data_2;

    bool is_immediate_set;
    bool is_read_data_2_set;

    bool is_alu_src_asserted;

    ALUInput2ForwardingMux *alu_input_2_forwarding_mux;

    static EXMuxALUInput2 *current_instance;

public:
    EXMuxALUInput2();

    static EXMuxALUInput2 *init();

    void run() override;
    void notifyModuleConditionVariable() override;
    void setInput(MuxInputType type, unsigned long value) override;
    void assertControlSignal(bool is_asserted) override;

protected:
    void passOutput() override;
};

#endif //RISC_V_SIMULATOR_EXMUXALUINPUT2_H