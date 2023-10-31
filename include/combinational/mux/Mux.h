#ifndef RISC_V_SIMULATOR_MUX_H
#define RISC_V_SIMULATOR_MUX_H

#include "../../common/Module.h"
#include "../../common/Config.h"

#include <variant>

class Mux: protected Module {
public:
    virtual void setInput(StageMuxInputType type, int value) = 0;
    virtual void assertControlSignal() = 0;

protected:
    virtual void loadOutput() = 0;
};

#endif //RISC_V_SIMULATOR_MUX_H
