#ifndef RISC_V_SIMULATOR_MUX_H
#define RISC_V_SIMULATOR_MUX_H

#include "../../common/Module.h"
#include "../../common/Config.h"

#include <variant>

class Mux: protected Module {
public:
    virtual void setInput(StageMuxInputType type, unsigned long value) = 0;
    virtual void assertControlSignal(bool is_asserted) = 0;

protected:
    bool is_control_signal_set;
    virtual void passOutput() = 0;
};

#endif //RISC_V_SIMULATOR_MUX_H
