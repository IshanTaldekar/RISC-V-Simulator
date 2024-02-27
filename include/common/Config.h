#ifndef RISC_V_SIMULATOR_CONFIG_H
#define RISC_V_SIMULATOR_CONFIG_H

#include <variant>
#include <string>

enum class IFAdderInputType {
    PCValue = 0
};

enum class EXAdderInputType {
    PCValue = 0,
    ImmediateValue
};

using AdderInputType = std::variant<IFAdderInputType, EXAdderInputType>;

enum class IFStageMuxInputType {
    IncrementedPc = 0,
    BranchedPc
};

enum class EXStageMuxALUInput2InputType {
    ReadData2 = 0,
    ImmediateValue
};

enum class EXStageMuxALUInput1InputType {
    ProgramCounter = 0,
    ReadData1
};

enum class WBStageMuxInputType {
    ReadData = 0,
    ALUResult
};

enum class Stage {
    Single = 0,
    Five
};

enum class ALUInputMuxInputTypes {
    IDEXStageRegisters = 0,
    EXMEMStageRegisters,
    MEMWBStageRegisters
};


/**
 * MuxInputType can hold either the value of IFStageMuxInputType, EXStageMuxALUInput2InputType,
 * WBStageMuxInputType.
 * */
using MuxInputType = std::variant<IFStageMuxInputType, EXStageMuxALUInput2InputType, EXStageMuxALUInput1InputType,
                                    WBStageMuxInputType, ALUInputMuxInputTypes>;

enum class InstructionType {
    R = 0,
    I,
    J,
    B,
    S,
    CUSTOM,
    UNKNOWN
};

const std::string IF_STAGE_LOG_FILE_PATH = "../logs/IFStage.log";
const std::string ID_STAGE_LOG_FILE_PATH = "../logs/IDStage.log";
const std::string EX_STAGE_LOG_FILE_PATH = "../logs/EXStage.log";
const std::string MEM_STAGE_LOG_FILE_PATH = "../logs/MEMStage.log";
const std::string WB_STAGE_LOG_FILE_PATH = "../logs/MEMStage.log";

enum class ALUInputMuxControlSignals {
    IDEXStageRegisters = 0,
    EXMEMStageRegisters,
    MEMWBStageRegisters
};


#endif //RISC_V_SIMULATOR_CONFIG_H
