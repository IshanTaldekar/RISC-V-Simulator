#ifndef RISC_V_SIMULATOR_INSTRUCTIONMEMORY_H
#define RISC_V_SIMULATOR_INSTRUCTIONMEMORY_H

#include <vector>
#include <string>
#include <stdexcept>
#include <fstream>

#include "stage-registers/IFIDStageRegisters.h"
#include "../common/Module.h"
#include "../common/Logger.h"
#include "../state/Driver.h"

class IFIDStageRegisters;
class Logger;
class IFIDStageRegisters;
class Driver;

class InstructionMemory: public Module {
    std::string instruction_memory_file_path;
    bool is_instruction_file_read;

    std::vector<std::string> data;

    unsigned long program_counter;
    bool is_new_program_counter_set;

    std::string instruction;

    IFIDStageRegisters *if_id_stage_registers;
    Logger *logger;
    Driver *driver;

    static InstructionMemory *current_instance;
    static std::mutex initialization_mutex;

public:
    InstructionMemory();

    static InstructionMemory *init();

    void run() override;

    void setInstructionMemoryInputFilePath(const std::string &file_path);
    void setProgramCounter(unsigned long value);

private:
    void fetchInstructionFromMemory();
    void readInstructionMemoryFile();

    void passInstructionIntoIFIDStageRegisters();
    void passNopToDriver(bool is_asserted);

    void initDependencies() override;
};

#endif //RISC_V_SIMULATOR_INSTRUCTIONMEMORY_H
