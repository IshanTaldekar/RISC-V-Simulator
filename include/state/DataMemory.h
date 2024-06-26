#ifndef RISC_V_SIMULATOR_DATAMEMORY_H
#define RISC_V_SIMULATOR_DATAMEMORY_H

#include <bitset>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

#include "../common/Module.h"
#include "../common/Logger.h"
#include "../common/StageSynchronizer.h"
#include "../state/stage-registers/MEMWBStageRegisters.h"

class MEMWBStageRegisters;
class StageSynchronizer;
class Logger;

class DataMemory: public Module {
    static constexpr int WORD_BIT_COUNT = 32;

    std::vector<std::string> data_memory;

    std::string data_memory_file_path;

    unsigned long address;

    std::bitset<WORD_BIT_COUNT> write_data;
    std::bitset<WORD_BIT_COUNT> read_data;

    bool is_mem_write_asserted;
    bool is_mem_read_asserted;

    bool is_address_set;
    bool is_write_data_set;
    bool is_mem_write_flag_set;
    bool is_mem_read_flag_set;
    bool is_input_file_read;
    bool is_reset_flag_set;

    static DataMemory *current_instance;
    static std::mutex initialization_mutex;

    MEMWBStageRegisters *mem_wb_stage_registers;
    StageSynchronizer *stage_synchronizer;

    std::string output_file_path;

public:
    DataMemory();

    static DataMemory *init();

    void run() override;

    void setDataMemoryInputFilePath(const std::string &file_path);

    void setAddress(unsigned long value);
    void setWriteData(std::bitset<WORD_BIT_COUNT> value);
    void setMemWrite(bool is_asserted);
    void setMemRead(bool is_asserted);

    void reset();
    void writeDataMemoryContentsToOutput();
    void clearDataMemoryOutputFile();

private:
    void readDataMemoryFile();
    void writeData();
    void readData();
    void passReadData(std::bitset<WORD_BIT_COUNT> data);
    void resetState();
    void initDependencies() override;

    std::string getModuleTag() override;
    Stage getModuleStage() override;
};

#endif //RISC_V_SIMULATOR_DATAMEMORY_H
