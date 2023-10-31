#ifndef RISC_V_SIMULATOR_IFMUX_H
#define RISC_V_SIMULATOR_IFMUX_H

#include "Mux.h"
#include "../../state/Driver.h"
#include "../../common/Config.h"

#include <iostream>

class IFMux: protected Mux {
    int incremented_pc;
    int branched_pc;

    bool is_incremented_pc_set;
    bool is_branched_pc_set;

    bool is_pc_src_signal_asserted;

    Driver *driver;
public:
    IFMux();

    static IFMux *current_instance;
    static IFMux *init();

    void run() override;
    void notifyConditionVariable() override;

    void setInput(StageMuxInputType type, int value) override;
    void assertControlSignal() override;

protected:
    void loadOutput() override;
};

IFMux *IFMux::current_instance = nullptr;

#endif //RISC_V_SIMULATOR_IFMUX_H
