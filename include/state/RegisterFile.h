#ifndef RISC_V_SIMULATOR_REGISTERFILE_H
#define RISC_V_SIMULATOR_REGISTERFILE_H

#include <vector>
#include <bitset>
#include <string>
#include <iostream>

#include "../common/Module.h"
#include "../common/Logger.h"
#include "../common/StageSynchronizer.h"
#include "../state/stage-registers/IDEXStageRegisters.h"

class IDEXStageRegisters;
class Logger;
class StageSynchronizer;

class RegisterFile: public Module {
    static constexpr int REGISTERS_COUNT = 32;

    static RegisterFile *current_instance;
    static std::mutex initialization_mutex;

    std::vector<std::bitset<WORD_BIT_COUNT>> registers;

    unsigned long register_source2;  // corresponds to the RISC-V rs2 register
    unsigned long register_source1;  // corresponds to the RISC-V rs1 register

    unsigned long register_destination;  // corresponds to the RISC-V rd register
    std::bitset<WORD_BIT_COUNT> write_data;  // data to be written to rd

    bool is_reg_write_signal_asserted;

    bool is_single_read_register_set;
    bool is_double_read_register_set;
    bool is_write_register_set;
    bool is_reg_write_signal_set;
    bool is_write_data_set;
    bool is_reset_flag_set;
    bool is_pause_flag_set;

    int cycle_count;

    IDEXStageRegisters *id_ex_stage_registers;
    StageSynchronizer *stage_synchronizer;

    std::string output_file_path;

public:
    RegisterFile();

    static RegisterFile *init();

    void run() override;

    void setReadRegister(unsigned long rs1);
    void setReadRegisters(unsigned long rs1, unsigned long rs2);

    void setWriteRegister(unsigned long rd);
    void setWriteData(std::bitset<WORD_BIT_COUNT> value);

    void setRegWriteSignal(bool is_asserted);

    void reset();
    void pause();
    void resume();

    void writeRegisterFileContentsToOutputFile();
    void clearRegisterFileOutputFile();

private:
    void passReadRegisterDataToIDEXStageRegister();
    void writeDataToRegisterFile();
    void resetRegisterFileContents();

    void initDependencies() override;
    void resetState();

    std::string getModuleTag() override;
    Stage getModuleStage() override;
};

#endif //RISC_V_SIMULATOR_REGISTERFILE_H
