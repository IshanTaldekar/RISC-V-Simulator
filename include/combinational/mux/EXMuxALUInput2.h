#ifndef RISC_V_SIMULATOR_EXMUXALUINPUT2_H
#define RISC_V_SIMULATOR_EXMUXALUINPUT2_H

#include "MuxBase.h"
#include "../../common/Config.h"
#include "../../common/Logger.h"
#include "forwarding/ALUInput2ForwardingMux.h"

#include <iostream>

class MuxBase;
class ALUInput2ForwardingMux;

class EXMuxALUInput2: public MuxBase {
    unsigned long immediate;
    unsigned long read_data_2;

    bool is_immediate_set;
    bool is_read_data_2_set;
    bool is_pass_four_flag_set;

    bool is_alu_src_asserted;
    bool is_pass_four_flag_asserted;

    ALUInput2ForwardingMux *alu_input_2_forwarding_mux;
    Logger *logger;

    static EXMuxALUInput2 *current_instance;
    static std::mutex initialization_mutex;

public:
    EXMuxALUInput2();

    static EXMuxALUInput2 *init();

    void run() override;
    void setInput(MuxInputType type, unsigned long value) override;
    void assertControlSignal(bool is_asserted) override;
    void assertJALCustomControlSignal(bool is_asserted);

protected:
    void passOutput() override;
    void initDependencies() override;
};

#endif //RISC_V_SIMULATOR_EXMUXALUINPUT2_H
