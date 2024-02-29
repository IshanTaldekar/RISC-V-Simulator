#ifndef RISC_V_SIMULATOR_IFADDER_H
#define RISC_V_SIMULATOR_IFADDER_H

#include "AdderBase.h"
#include "../../common/Config.h"
#include "../mux/IFMux.h"
#include "../../common/Logger.h"

class IFMux;
class Logger;

class IFAdder: public AdderBase {
    unsigned long program_counter;
    bool is_program_counter_set;

    static IFAdder *current_instance;

    IFMux *if_mux;
    Logger *logger;

public:
    IFAdder();
    static IFAdder *init();

    void run() override;
    void setInput(AdderInputType type, unsigned long value) override;

private:
    void passProgramCounterToIFMux();
    void initDependencies() override;
};

#endif //RISC_V_SIMULATOR_IFADDER_H
