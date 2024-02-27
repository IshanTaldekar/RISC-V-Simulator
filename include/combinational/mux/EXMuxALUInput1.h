#ifndef RISC_V_SIMULATOR_EXMUXALUINPUT1_H
#define RISC_V_SIMULATOR_EXMUXALUINPUT1_H

#include "MuxBase.h"
#include "forwarding/ALUInput1ForwardingMux.h"

class MuxBase;
class ALUInput1ForwardingMux;

class EXMuxALUInput1: protected MuxBase {
    unsigned long program_counter;
    unsigned long read_data_1;

    bool is_program_counter_set;
    bool is_read_data_1_set;

    bool is_pass_program_counter_flag_asserted;

    ALUInput1ForwardingMux *alu_input_1_forwarding_mux;

    static EXMuxALUInput1 *current_instance;

public:
    EXMuxALUInput1();

    static EXMuxALUInput1 *init();

    void run() override;
    void notifyModuleConditionVariable() override;
    void setInput(MuxInputType type, unsigned long value) override;
    void assertControlSignal(bool is_asserted) override;

protected:
    void passOutput() override;
};

#endif //RISC_V_SIMULATOR_EXMUXALUINPUT1_H
