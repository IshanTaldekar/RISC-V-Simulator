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
    std::bitset<WORD_BIT_COUNT> immediate;
    std::bitset<WORD_BIT_COUNT> read_data_2;

    bool is_immediate_set;
    bool is_read_data_2_set;
    bool is_pass_four_flag_set;

    bool is_alu_src_asserted;
    bool is_pass_four_flag_asserted;

    bool is_reset_flag_set;

    ALUInput2ForwardingMux *alu_input_2_forwarding_mux;

    static EXMuxALUInput2 *current_instance;
    static std::mutex initialization_mutex;

public:
    EXMuxALUInput2();

    static EXMuxALUInput2 *init();

    void run() override;
    void setInput(MuxInputType type, MuxInputDataType value) override;
    void assertControlSignal(bool is_asserted) override;
    void assertJALCustomControlSignal(bool is_asserted);

    void reset();

protected:
    void passOutput() override;
    void initDependencies() override;

    void resetState();

    std::string getModuleTag() override;
    Stage getModuleStage() override;
};

#endif //RISC_V_SIMULATOR_EXMUXALUINPUT2_H
