#ifndef RISC_V_SIMULATOR_MUXBASE_H
#define RISC_V_SIMULATOR_MUXBASE_H

#include "../../common/Module.h"
#include "../../common/Config.h"

#include <variant>

class MuxBase: public Module {
public:
    virtual void setInput(MuxInputType type, unsigned long value) = 0;
    virtual void assertControlSignal(bool is_asserted);

protected:
    bool is_control_signal_set;
    virtual void passOutput() = 0;
};

#endif //RISC_V_SIMULATOR_MUXBASE_H