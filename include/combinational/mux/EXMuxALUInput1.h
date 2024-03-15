#ifndef RISC_V_SIMULATOR_EXMUXALUINPUT1_H
#define RISC_V_SIMULATOR_EXMUXALUINPUT1_H

#include <bitset>

#include "MuxBase.h"
#include "forwarding/ALUInput1ForwardingMux.h"
#include "../../common/Logger.h"

class MuxBase;
class ALUInput1ForwardingMux;
class Logger;

class EXMuxALUInput1: public MuxBase {
    std::bitset<WORD_BIT_COUNT> program_counter;
    std::bitset<WORD_BIT_COUNT> read_data_1;

    bool is_program_counter_set;
    bool is_read_data_1_set;

    bool is_pass_program_counter_flag_asserted;
    bool is_pass_program_counter_flag_set;

    bool is_reset_flag_set;

    ALUInput1ForwardingMux *alu_input_1_forwarding_mux;
    Logger *logger;

    static EXMuxALUInput1 *current_instance;
    static std::mutex initialization_mutex;

public:
    EXMuxALUInput1();

    static EXMuxALUInput1 *init();

    void run() override;

    void setInput(MuxInputType type, MuxInputDataType value) override;
    void assertControlSignal(bool is_asserted) override;
    void assertJALCustomControlSignal(bool is_asserted);

    void reset();

protected:
    void passOutput() override;
    void initDependencies() override;

    void resetState();
};

#endif //RISC_V_SIMULATOR_EXMUXALUINPUT1_H
