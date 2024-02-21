#ifndef RISC_V_SIMULATOR_IFADDER_H
#define RISC_V_SIMULATOR_IFADDER_H

#include "Adder.h"
#include "../../common/Config.h"
#include "../mux/IFMux.h"
#include "../../common/logger/IFLogger.h"

class IFMux;
class IFLogger;

class IFAdder: protected Adder {
    unsigned long program_counter;
    bool is_program_counter_set;

    static IFAdder *current_instance;

    IFMux *if_mux;
    IFLogger *logger;

public:
    IFAdder();
    static IFAdder *init();

    void run() override;
    void setInput(AdderInputType type, unsigned long value) override;
    void notifyModuleConditionVariable() override;

private:
    void passProgramCounterToIFMux();
};

#endif //RISC_V_SIMULATOR_IFADDER_H
