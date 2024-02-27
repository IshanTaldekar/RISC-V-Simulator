#ifndef RISC_V_SIMULATOR_ALUINPUT1FORWARDINGMUX_H
#define RISC_V_SIMULATOR_ALUINPUT1FORWARDINGMUX_H

#include <string>

#include "../../../common/Config.h"
#include "ALUInputForwardingMuxBase.h"


class ALUInput1ForwardingMux: public ALUInputForwardingMuxBase {
    static ALUInput1ForwardingMux *current_instance;

public:
    static ALUInput1ForwardingMux *init();

protected:
    void passOutput() override;
    std::string getModuleTag() override;
};

#endif //RISC_V_SIMULATOR_ALUINPUT1FORWARDINGMUX_H
