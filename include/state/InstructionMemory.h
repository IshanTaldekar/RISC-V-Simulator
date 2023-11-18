#ifndef RISC_V_SIMULATOR_INSTRUCTIONMEMORY_H
#define RISC_V_SIMULATOR_INSTRUCTIONMEMORY_H

#include <vector>
#include <string>
#include <stdexcept>
#include <fstream>

#include "stage-registers/IFIDStageRegisters.h"
#include "../common/Module.h"
#include "../common/logger/IFLogger.h"

class IFIDStageRegisters;

class InstructionMemory: public Module {
private:
    std::string instruction_memory_file_path;
    bool is_instruction_file_read;

    std::vector<std::string> data;

    int program_counter;
    bool is_new_program_counter_set;

    std::string instruction;

    IFIDStageRegisters *if_id_stage_registers;
    IFLogger *logger;

public:
    InstructionMemory();

    static InstructionMemory *init();
    static InstructionMemory *current_instance;

    void run() override;
    void notifyModuleConditionVariable() override;

    void setInstructionMemoryFilePath(const std::string &file_path);
    void setProgramCounter(int value);

private:
    void fetchInstructionFromMemory();
    void readInstructionMemoryFile();

    void passInstructionIntoIFIDStageRegisters();
};

InstructionMemory *InstructionMemory::current_instance = nullptr;

#endif //RISC_V_SIMULATOR_INSTRUCTIONMEMORY_H
