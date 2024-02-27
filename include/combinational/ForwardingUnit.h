#ifndef RISC_V_SIMULATOR_FORWARDINGUNIT_H
#define RISC_V_SIMULATOR_FORWARDINGUNIT_H

#include "../common/Module.h"
#include "../common/Config.h"
#include "../combinational/mux/forwarding/ALUInput1ForwardingMux.h"
#include "../combinational/mux/forwarding/ALUInput2ForwardingMux.h"

class ALUInput1ForwardingMux;
class ALUInput2ForwardingMux;

class ForwardingUnit: public Module {
    unsigned long register_source1;
    unsigned long register_source2;
    unsigned long ex_mem_stage_register_destination;
    unsigned long mem_wb_stage_register_destination;

    bool is_single_register_source_set;
    bool is_double_register_source_set;
    bool is_ex_mem_stage_register_destination_set;
    bool is_mem_wb_stage_register_destination_set;
    bool is_reset_flag_set;

    static ForwardingUnit *current_instance;

    ALUInput1ForwardingMux *alu_input_1_mux;
    ALUInput2ForwardingMux *alu_input_2_mux;

    ALUInputMuxControlSignals alu_input_1_mux_control_signal;
    ALUInputMuxControlSignals alu_input_2_mux_control_signal;

public:
    ForwardingUnit();

    static ForwardingUnit *init();

    void run() override;
    void notifyModuleConditionVariable() override;

    void setSingleRegisterSource(unsigned long rs1);
    void setDoubleRegisterSource(unsigned long rs1, unsigned long rs2);
    void setEXMEMStageRegisterDestination(unsigned long rd);
    void setMEMWBStageRegisterDestination(unsigned long rd);

    void reset();

private:
    void passControlSignalToALUInputMux1();
    void passControlSignalToALUInputMux2();

    void resetState();
    void computeControlSignals();
};

#endif //RISC_V_SIMULATOR_FORWARDINGUNIT_H
