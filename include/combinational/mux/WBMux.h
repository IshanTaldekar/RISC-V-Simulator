#ifndef RISC_V_SIMULATOR_WBMUX_H
#define RISC_V_SIMULATOR_WBMUX_H

#include "Mux.h"
#include "../../state/RegisterFile.h"
#include "../../common/logger/WBLogger.h"

class RegisterFile;
class WBLogger;

class WBMux: protected Mux {
    unsigned int read_data;
    unsigned int alu_result;

    bool is_mem_to_reg_asserted;

    bool is_read_data_set;
    bool is_alu_result_set;

    static WBMux *current_instance;

    RegisterFile *register_file;

    WBLogger *logger;

public:
    WBMux();

    static WBMux *init();

    void run() override;
    void notifyModuleConditionVariable() override;
    void setInput(StageMuxInputType type, unsigned long value) override;
    void assertControlSignal(bool is_asserted) override;

protected:
    void passOutput() override;
};

WBMux *WBMux::current_instance = nullptr;

#endif //RISC_V_SIMULATOR_WBMUX_H
