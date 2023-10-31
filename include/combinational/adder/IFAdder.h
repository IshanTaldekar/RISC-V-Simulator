#ifndef RISC_V_SIMULATOR_IFADDER_H
#define RISC_V_SIMULATOR_IFADDER_H

#include "Adder.h"
#include "../../common/Config.h"
#include "../mux/IFMux.h"

class IFAdder: protected Adder {
    int program_counter;
    bool program_counter_set;

    IFMux *if_mux;

public:
    IFAdder();

    static IFAdder *init();
    static IFAdder *current_instance;

    void run() override;
    void setInput(AdderInputType type, int value) override;
    void notifyConditionVariable() override;

private:
    void loadProgramCounterToIFMux();
};

IFAdder *IFAdder::current_instance = nullptr;

#endif //RISC_V_SIMULATOR_IFADDER_H
