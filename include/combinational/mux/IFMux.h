#ifndef RISC_V_SIMULATOR_IFMUX_H
#define RISC_V_SIMULATOR_IFMUX_H

#include <iostream>

#include "MuxBase.h"
#include "../../state/Driver.h"
#include "../../common/Config.h"
#include "../../common/Logger.h"

class Driver;
class Logger;

class IFMux: public MuxBase {
    unsigned long incremented_pc;
    unsigned long branched_pc;

    bool is_incremented_pc_set;
    bool is_branched_pc_set;

    bool is_pc_src_signal_asserted;

    Driver *driver;

    static IFMux *current_instance;
    static std::mutex initialization_mutex;

public:
    IFMux();

    static IFMux *init();

    void run() override;
    void setInput(MuxInputType type, MuxInputDataType value) override;
    void assertControlSignal(bool is_asserted) override;

protected:
    void passOutput() override;
    void initDependencies() override;

    std::string getModuleTag() override;
    Stage getModuleStage() override;
};

#endif //RISC_V_SIMULATOR_IFMUX_H
