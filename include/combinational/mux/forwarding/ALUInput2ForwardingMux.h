#ifndef RISC_V_SIMULATOR_ALUINPUT2FORWARDINGMUX_H
#define RISC_V_SIMULATOR_ALUINPUT2FORWARDINGMUX_H

#include "ALUInputForwardingMuxBase.h"

class ALU;
class ALUInputForwardingMuxBase;

class ALUInput2ForwardingMux: public ALUInputForwardingMuxBase {
    static ALUInput2ForwardingMux *current_instance;

public:
    static ALUInput2ForwardingMux *init();

protected:
    void passOutput() override;
    std::string getModuleTag() override;
};

#endif //RISC_V_SIMULATOR_ALUINPUT2FORWARDINGMUX_H
