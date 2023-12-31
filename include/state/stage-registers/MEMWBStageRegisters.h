#ifndef RISC_V_SIMULATOR_MEMWBSTAGEREGISTERS_H
#define RISC_V_SIMULATOR_MEMWBSTAGEREGISTERS_H

#include "../../common/Module.h"
#include "../../common/Control.h"
#include "../../state/RegisterFile.h"
#include "../../combinational/mux/WBMux.h"
#include "../../common/StageSynchronizer.h"

class Control;
class RegisterFile;
class WBMux;
class StageSynchronizer;

class MEMWBStageRegisters: public Module {
    unsigned long read_data;
    unsigned long alu_result;
    unsigned long register_destination;

    bool is_read_data_set;
    bool is_alu_result_set;
    bool is_register_destination_set;
    bool is_control_set;
    bool is_reset_flag_set;
    bool is_pause_flag_set;

    static MEMWBStageRegisters *current_instance;

    Control *control;

    RegisterFile *register_file;
    WBMux *wb_mux;
    StageSynchronizer *stage_synchronizer;

public:
    MEMWBStageRegisters();

    static MEMWBStageRegisters *init();

    void run() override;
    void notifyModuleConditionVariable() override;

    void setReadData(unsigned long value);
    void setALUResult(unsigned long value);
    void setRegisterDestination(unsigned long value);

    void reset();
    void pause();

private:
    void passReadDataToWBMux();
    void passALUResultToWBMux();
    void passRegisterDestinationToRegisterFile();

    void resetStage();
    void pauseStage();
};

#endif //RISC_V_SIMULATOR_MEMWBSTAGEREGISTERS_H
