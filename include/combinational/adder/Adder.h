#ifndef RISC_V_SIMULATOR_ADDER_H
#define RISC_V_SIMULATOR_ADDER_H

#include "../../common/Config.h"
#include "../../common/Module.h"

#include <stdexcept>

class Adder: public Module {
public:
    virtual void setInput(AdderInputType type, unsigned long value) = 0;
};


#endif //RISC_V_SIMULATOR_ADDER_H
