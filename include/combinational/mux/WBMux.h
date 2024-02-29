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
    unsigned int read_data;
    unsigned int alu_result;

    bool is_mem_to_reg_asserted;

    bool is_read_data_set;
    bool is_alu_result_set;

    static WBMux *current_instance;

    RegisterFile *register_file;

    Logger *logger;
    ALUInput1ForwardingMux *alu_input_1_forwarding_mux;
    ALUInput2ForwardingMux *alu_input_2_forwarding_mux;

public:
    WBMux();

    static WBMux *init();

    void run() override;
    void setInput(MuxInputType type, unsigned long value) override;
    void assertControlSignal(bool is_asserted) override;

protected:
    void passOutput() override;
    void passOutputToForwardingMuxes();
};

#endif //RISC_V_SIMULATOR_WBMUX_H
