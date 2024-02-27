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

enum class PipelineType {
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

enum class ALUInputMuxControlSignals {
    IDEXStageRegisters = 0,
    EXMEMStageRegisters,
    MEMWBStageRegisters
};

enum class Stage {
    IF = 0,
    ID,
    EX,
    MEM,
    WB
};

#endif //RISC_V_SIMULATOR_CONFIG_H
