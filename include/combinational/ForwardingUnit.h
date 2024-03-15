#ifndef RISC_V_SIMULATOR_FORWARDINGUNIT_H
#define RISC_V_SIMULATOR_FORWARDINGUNIT_H

#include "../common/Module.h"
#include "../common/Config.h"
#include "../common/Logger.h"
#include "../combinational/mux/forwarding/ALUInput1ForwardingMux.h"
#include "../combinational/mux/forwarding/ALUInput2ForwardingMux.h"

#include <iostream>

class ALUInput1ForwardingMux;
class ALUInput2ForwardingMux;
class Logger;

class ForwardingUnit: public Module {
    unsigned long register_source1;
    unsigned long register_source2;
    unsigned long ex_mem_stage_register_destination;
    unsigned long mem_wb_stage_register_destination;

    bool is_single_register_source_set;
    bool is_double_register_source_set;
    bool is_ex_mem_stage_register_destination_set;
    bool is_mem_wb_stage_register_destination_set;

    bool is_ex_mem_reg_write_asserted;
    bool is_mem_wb_reg_write_asserted;

    bool is_ex_mem_reg_write_set;
    bool is_mem_wb_reg_write_set;

    bool is_reset_flag_set;

    static ForwardingUnit *current_instance;
    static std::mutex initialization_mutex;

    ALUInput1ForwardingMux *alu_input_1_mux;
    ALUInput2ForwardingMux *alu_input_2_mux;

    ALUInputMuxControlSignals alu_input_1_mux_control_signal;
    ALUInputMuxControlSignals alu_input_2_mux_control_signal;

    Logger *logger;

public:
    ForwardingUnit();

    static ForwardingUnit *init();

    void run() override;

    void setSingleRegisterSource(unsigned long rs1);
    void setDoubleRegisterSource(unsigned long rs1, unsigned long rs2);
    void setEXMEMStageRegisterDestination(unsigned long rd);
    void setMEMWBStageRegisterDestination(unsigned long rd);
    void setEXMEMStageRegisterRegWrite(bool is_asserted);
    void setMEMWBStageRegisterRegWrite(bool is_asserted);

    void reset();

private:
    void passControlSignalToALUInput1ForwardingMux();
    void passControlSignalToALUInput2ForwardingMux();

    void resetState();
    void computeControlSignals();

    void initDependencies() override;
};

#endif //RISC_V_SIMULATOR_FORWARDINGUNIT_H
