#ifndef RISC_V_SIMULATOR_MUXBASE_H
#define RISC_V_SIMULATOR_MUXBASE_H

#include <variant>

#include "../../common/Module.h"

class Module;

class MuxBase: public Module {
public:
    virtual void setInput(MuxInputType type, MuxInputDataType value) = 0;
    virtual void assertControlSignal(bool is_asserted);

protected:
    bool is_control_signal_set;
    virtual void passOutput() = 0;
};

#endif //RISC_V_SIMULATOR_MUXBASE_H
