#ifndef RISC_V_SIMULATOR_IFADDER_H
#define RISC_V_SIMULATOR_IFADDER_H

#include "Adder.h"
#include "../../common/Config.h"
#include "../mux/IFMux.h"
#include "../../common/logger/IFLogger.h"

class IFAdder: protected Adder {
    int program_counter;
    bool program_counter_set;

    static IFAdder *current_instance;

    IFMux *if_mux;
    IFLogger *logger;

public:

    IFAdder();
    static IFAdder *init();

    void run() override;
    void setInput(AdderInputType type, int value) override;
    void notifyModuleConditionVariable() override;

private:
    void loadProgramCounterToIFMux();
};

IFAdder *IFAdder::current_instance = nullptr;

#endif //RISC_V_SIMULATOR_IFADDER_H
