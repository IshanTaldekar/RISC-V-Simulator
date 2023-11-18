#ifndef RISC_V_SIMULATOR_IFMUX_H
#define RISC_V_SIMULATOR_IFMUX_H

#include "Mux.h"
#include "../../state/Driver.h"
#include "../../common/Config.h"
#include "../../common/logger/IFLogger.h"

#include <iostream>

class Driver;
class IFLogger;

class IFMux: protected Mux {
    unsigned long incremented_pc;
    unsigned long branched_pc;

    bool is_incremented_pc_set;
    bool is_branched_pc_set;

    bool is_pc_src_signal_asserted;

    Driver *driver;
    IFLogger *logger;

    static IFMux *current_instance;

public:
    IFMux();

    static IFMux *init();

    void run() override;
    void notifyModuleConditionVariable() override;
    void setInput(StageMuxInputType type, unsigned long value) override;
    void assertControlSignal(bool is_asserted) override;

protected:
    void passOutput() override;
};

IFMux *IFMux::current_instance = nullptr;

#endif //RISC_V_SIMULATOR_IFMUX_H
