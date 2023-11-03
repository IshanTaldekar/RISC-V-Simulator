#ifndef RISC_V_SIMULATOR_IDEXSTAGEREGISTERS_H
#define RISC_V_SIMULATOR_IDEXSTAGEREGISTERS_H

#include "../../common/Module.h"
#include "../../common/Instruction.h"
#include "../../common/Control.h"

#include <bitset>

class IDEXStageRegisters: public Module {
    static constexpr int WORD_BIT_COUNT = 32;

    const Instruction *instruction;
    Control *control;

    std::bitset<WORD_BIT_COUNT> read_data_1;
    std::bitset<WORD_BIT_COUNT> read_data_2;
    std::bitset<WORD_BIT_COUNT> immediate;
    int register_destination;
    int program_counter;

    static IDEXStageRegisters *current_instance;

    bool is_single_read_register_data_set;
    bool is_double_read_register_data_set;
    bool is_immediate_set;
    bool is_register_destination_set;
    bool is_program_counter_set;
    bool is_control_set;

public:
    IDEXStageRegisters();

    static IDEXStageRegisters *init();

    void run() override;
    void notifyModuleConditionVariable() override;

    void setRegisterData(const std::bitset<WORD_BIT_COUNT> &rd1);
    void setRegisterData(const std::bitset<WORD_BIT_COUNT> &rd1, const std::bitset<WORD_BIT_COUNT> &rd2);
    void setImmediate(const std::bitset<WORD_BIT_COUNT> &imm);
    void setRegisterDestination(unsigned long rd);
    void setProgramCounter(int pc);
    void setControlModule(Control *new_control);
};

#endif //RISC_V_SIMULATOR_IDEXSTAGEREGISTERS_H
