#ifndef RISC_V_SIMULATOR_MEMWBSTAGEREGISTERS_H
#define RISC_V_SIMULATOR_MEMWBSTAGEREGISTERS_H

#include "../../common/Module.h"
#include "../../common/Control.h"
#include "../../common/Logger.h"
#include "../../state/RegisterFile.h"
#include "../../combinational/mux/WBMux.h"
#include "../../common/StageSynchronizer.h"
#include "../../combinational/ForwardingUnit.h"

class Control;
class RegisterFile;
class WBMux;
class StageSynchronizer;
class ForwardingUnit;
class Logger;

class MEMWBStageRegisters: public Module {
    std::bitset<WORD_BIT_COUNT> read_data;
    std::bitset<WORD_BIT_COUNT> alu_result;
    unsigned long register_destination;

    bool is_read_data_set;
    bool is_alu_result_set;
    bool is_register_destination_set;
    bool is_control_set;
    bool is_reset_flag_set;
    bool is_pause_flag_set;
    bool is_nop_asserted;
    bool is_nop_passed_flag_set;
    bool is_nop_passed_flag_asserted{};

    static MEMWBStageRegisters *current_instance;
    static std::mutex initialization_mutex;

    Control *control;

    RegisterFile *register_file;
    WBMux *wb_mux;
    StageSynchronizer *stage_synchronizer;
    ForwardingUnit *forwarding_unit;

    friend StageSynchronizer;

public:
    MEMWBStageRegisters();

    static MEMWBStageRegisters *init();

    void run() override;

    void setReadData(std::bitset<WORD_BIT_COUNT> value);
    void setALUResult(std::bitset<WORD_BIT_COUNT> value);
    void setRegisterDestination(unsigned long value);
    void setControl(Control *new_control);
    void setPassedNop(bool is_asserted);
    bool isExecutingHaltInstruction();
    void assertNop();

    void reset();
    void pause();
    void resume();

private:
    void passReadDataToWBMux();
    void passALUResultToWBMux();
    void passRegisterDestinationToRegisterFile();
    void passRegisterDestinationToForwardingUnit();
    void passRegWriteToForwardingUnit();

    void resetStage();
    void initDependencies() override;

    std::string getModuleTag() override;
    Stage getModuleStage() override;
};

#endif //RISC_V_SIMULATOR_MEMWBSTAGEREGISTERS_H
