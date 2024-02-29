#ifndef RISC_V_SIMULATOR_ALUINPUT1FORWARDINGMUX_H
#define RISC_V_SIMULATOR_ALUINPUT1FORWARDINGMUX_H

#include <string>

#include "../../../common/Config.h"
#include "../../../../include/combinational/mux/forwarding/ALUInputForwardingMuxBase.h"

class MuxBase;
class ALUInput1ForwardingMux;
class ALU;

class ALUInput1ForwardingMux: public ALUInputForwardingMuxBase {
    static ALUInput1ForwardingMux *current_instance;
    static std::mutex initialization_mutex;

public:
    static ALUInput1ForwardingMux *init();

protected:
    void passOutput() override;
    std::string getModuleTag() override;
};

#endif //RISC_V_SIMULATOR_ALUINPUT1FORWARDINGMUX_H
