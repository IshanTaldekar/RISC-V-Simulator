#ifndef RISC_V_SIMULATOR_MEMWBSTAGEREGISTERS_H
#define RISC_V_SIMULATOR_MEMWBSTAGEREGISTERS_H

#include "../../common/Module.h"
#include "../../common/Control.h"
#include "../../state/RegisterFile.h"
#include "../../combinational/mux/WBMux.h"

class Control;
class RegisterFile;
class WBMux;

class MEMWBStageRegisters: public Module {
    unsigned long read_data;
    unsigned long alu_result;
    unsigned long register_destination;

    bool is_read_data_set;
    bool is_alu_result_set;
    bool is_register_destination_set;
    bool is_control_set;

    static MEMWBStageRegisters *current_instance;

    Control *control;

    RegisterFile *register_file;
    WBMux *wb_mux;

public:
    MEMWBStageRegisters();

    static MEMWBStageRegisters *init();

    void run() override;
    void notifyModuleConditionVariable() override;

    void setReadData(unsigned long value);
    void setALUResult(unsigned long value);
    void setRegisterDestination(unsigned long value);

private:
    void passReadDataToWBMux();
    void passALUResultToWBMux();
    void passRegisterDestinationToRegisterFile();
};

MEMWBStageRegisters *MEMWBStageRegisters::current_instance = nullptr;

#endif //RISC_V_SIMULATOR_MEMWBSTAGEREGISTERS_H
