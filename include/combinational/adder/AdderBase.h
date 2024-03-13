#ifndef RISC_V_SIMULATOR_ADDERBASE_H
#define RISC_V_SIMULATOR_ADDERBASE_H

#include "../../common/Config.h"
#include "../../common/Module.h"

#include <stdexcept>

class AdderBase: public Module {
public:
    virtual void setInput(const AdderInputType &type, const AdderInputDataType &value) = 0;
};


#endif //RISC_V_SIMULATOR_ADDERBASE_H
