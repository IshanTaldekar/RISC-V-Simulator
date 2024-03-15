#ifndef RISC_V_SIMULATOR_ALUINPUTFORWARDINGMUXBASE_H
#define RISC_V_SIMULATOR_ALUINPUTFORWARDINGMUXBASE_H

#include <string>

#include "../../../common/Config.h"
#include "../MuxBase.h"
#include "../../../common/Logger.h"

class ALU;
class Logger;
class MuxBase;

class ALUInputForwardingMuxBase: public MuxBase {
protected:
    std::bitset<WORD_BIT_COUNT> id_ex_stage_registers_value;
    std::bitset<WORD_BIT_COUNT> ex_mem_stage_registers_value;
    std::bitset<WORD_BIT_COUNT> mem_wb_stage_registers_value;

    ALUInputMuxControlSignals control_signal;

    bool is_id_ex_stage_registers_value_set;
    bool is_ex_mem_stage_registers_value_set;
    bool is_mem_wb_stage_registers_value_set;

    bool is_reset_flag_set;

    ALU *alu;
    Logger *logger;

public:
    ALUInputForwardingMuxBase();

    void run() override;

    void setInput(MuxInputType type, MuxInputDataType value) override;
    void setMuxControlSignal(ALUInputMuxControlSignals new_signal);

    void reset();

protected:
    virtual std::string getModuleTag() = 0;

    void initDependencies() override;
    void resetState();
};

#endif //RISC_V_SIMULATOR_ALUINPUTFORWARDINGMUXBASE_H
