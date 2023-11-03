#ifndef RISC_V_SIMULATOR_REGISTERFILE_H
#define RISC_V_SIMULATOR_REGISTERFILE_H

#include <vector>
#include <bitset>
#include <string>
#include <iostream>

#include "../common/Module.h"
#include "../common/logger/IDLogger.h"
#include "../state/stage-registers/IDEXStageRegisters.h"

class RegisterFile: public Module {
    static constexpr int REGISTERS_COUNT = 32;
    static constexpr int WORD_BIT_COUNT = 32;

    static RegisterFile *current_instance;

    std::vector<std::bitset<WORD_BIT_COUNT>> registers;

    unsigned long register_source2;  // corresponds to the RISC-V rs2 register
    unsigned long register_source1;  // corresponds to the RISC-V rs1 register

    unsigned long register_destination;  // corresponds to the RISC-V rd register
    std::bitset<WORD_BIT_COUNT> write_data;

    bool is_single_read_register_set;
    bool is_double_read_register_set;
    bool is_write_register_set;
    bool is_reg_write_signal_set;

    std::mutex write_load_mutex;
    std::condition_variable load_condition_variable;

    bool is_write_thread_finished;

    IDLogger *logger;
    IDEXStageRegisters *id_ex_stage_registers;

public:
    RegisterFile();

    static RegisterFile *init();

    void run() override;
    void notifyModuleConditionVariable() override;

    void setReadRegister(unsigned long rs1);
    void setReadRegisters(unsigned long rs1, unsigned long rs2);

    void setWriteRegister(unsigned long rd, const std::bitset<WORD_BIT_COUNT> &write_data);
    void setRegWriteSignal();

private:
    void passReadRegisterDataToIDEXStageRegister();
    void writeDataToRegisterFile();
};

RegisterFile *RegisterFile::current_instance = nullptr;

#endif //RISC_V_SIMULATOR_REGISTERFILE_H
