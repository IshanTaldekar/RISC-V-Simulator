cmake_minimum_required(VERSION 3.22)
project(RISC_V_Simulator)

set(CMAKE_CXX_STANDARD 20)

#add_executable(RISC_V_Simulator
#        src/main.cpp
#        src/combinational/adder/IFAdder.cpp
#        src/common/Config.cpp
#        src/state/Driver.cpp
#        src/state/InstructionMemory.cpp
#        src/combinational/mux/IFMux.cpp
#        src/common/Module.cpp
#        src/state/stage-registers/IFIDStageRegisters.cpp
#        src/common/logger/LoggerBase.cpp
#        src/common/logger/IFLogger.cpp
#        src/common/logger/IDLogger.cpp
#        src/common/logger/EXLogger.cpp
#        src/common/logger/MEMLogger.cpp
#        src/common/logger/WBLogger.cpp
#        src/common/Instruction.cpp
#        src/state/RegisterFile.cpp
#        src/combinational/ImmediateGenerator.cpp
#        src/common/Control.cpp
#        src/state/stage-registers/IDEXStageRegisters.cpp
#        src/combinational/adder/EXAdder.cpp
#        src/combinational/mux/EXMuxALUInput2.cpp
#        src/combinational/ALU.cpp
#        src/state/stage-registers/EXMEMStageRegisters.cpp
#        src/state/DataMemory.cpp
#        src/state/stage-registers/MEMWBStageRegisters.cpp
#        src/combinational/mux/WBMux.cpp
#)

include_directories("${CMAKE_CURRENT_LIST_DIR}/include")

add_executable(RISC_V_Simulator
        src/common/Control.cpp
        src/combinational/ALU.cpp
        src/combinational/mux/MuxBase.cpp
        src/common/Module.cpp
        src/combinational/mux/forwarding/ALUInputForwardingMuxBase.cpp
        src/combinational/mux/forwarding/ALUInput2ForwardingMux.cpp
        src/combinational/mux/forwarding/ALUInput1ForwardingMux.cpp
        src/state/stage-registers/MEMWBStageRegisters.cpp
        src/state/DataMemory.cpp
        src/state/stage-registers/EXMEMStageRegisters.cpp
        src/combinational/mux/WBMux.cpp
        src/combinational/mux/EXMuxALUInput2.cpp
        src/combinational/adder/EXAdder.cpp
        src/state/stage-registers/IDEXStageRegisters.cpp
        src/combinational/ImmediateGenerator.cpp
        src/state/RegisterFile.cpp
        src/common/Instruction.cpp
        src/common/logger/WBLogger.cpp
        src/common/logger/MEMLogger.cpp
        src/common/logger/EXLogger.cpp
        src/common/logger/IDLogger.cpp
        src/common/logger/IFLogger.cpp
        src/common/logger/LoggerBase.cpp
        src/state/stage-registers/IFIDStageRegisters.cpp
        src/combinational/mux/IFMux.cpp
        src/state/InstructionMemory.cpp
        src/state/Driver.cpp
        src/combinational/adder/IFAdder.cpp
        src/main.cpp
        src/common/StageSynchronizer.cpp
        src/combinational/ForwardingUnit.cpp
        src/combinational/mux/EXMuxALUInput1.cpp
)

#target_sources(RISC_V_Simulator PUBLIC ${CMAKE_SOURCE_DIR}/include)