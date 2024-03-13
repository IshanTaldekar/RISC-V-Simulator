#ifndef RISC_V_SIMULATOR_WBMUX_H
#define RISC_V_SIMULATOR_WBMUX_H

#include "MuxBase.h"
#include "../../state/RegisterFile.h"
#include "../../common/Logger.h"
#include "forwarding/ALUInput1ForwardingMux.h"
#include "forwarding/ALUInput2ForwardingMux.h"

class ALUInput1ForwardingMux;
class ALUInput2ForwardingMux;
class RegisterFile;
class Logger;

class WBMux: public MuxBase {
    std::bitset<WORD_BIT_COUNT> read_data;
    std::bitset<WORD_BIT_COUNT> alu_result;

    bool is_mem_to_reg_asserted;

    bool is_read_data_set;
    bool is_alu_result_set;

    static WBMux *current_instance;
    static std::mutex initialization_mutex;

    RegisterFile *register_file;

    Logger *logger;
    ALUInput1ForwardingMux *alu_input_1_forwarding_mux;
    ALUInput2ForwardingMux *alu_input_2_forwarding_mux;

public:
    WBMux();

    static WBMux *init();

    void run() override;
    void setInput(const MuxInputType &type, const MuxInputDataType &value) override;
    void assertControlSignal(bool is_asserted) override;

protected:
    void passOutput() override;
    void passOutputToForwardingMuxes();
    void initDependencies() override;
};

#endif //RISC_V_SIMULATOR_WBMUX_H
