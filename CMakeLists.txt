cmake_minimum_required(VERSION 3.26)
project(RISC_V_Simulator)

set(CMAKE_CXX_STANDARD 20)

add_executable(RISC_V_Simulator
        src/main.cpp
        src/combinational/adder/IFAdder.cpp
        src/common/Config.cpp
        src/state/Driver.cpp
        src/state/InstructionMemory.cpp
        src/combinational/mux/IFMux.cpp
        src/common/Module.cpp
        src/state/stage-registers/IFIDStageRegisters.cpp
        src/common/logger/Logger.cpp
        src/common/logger/IFLogger.cpp
        src/common/logger/IDLogger.cpp
        src/common/logger/EXLogger.cpp
        src/common/logger/MEMLogger.cpp
        src/common/logger/WBLogger.cpp
        src/common/Instruction.cpp
        include/state/RegisterFile.h
        src/state/RegisterFile.cpp
        include/combinational/ImmediateGenerator.h
        src/combinational/ImmediateGenerator.cpp
        include/common/Control.h
        src/common/Control.cpp
        include/state/stage-registers/IDEXStageRegisters.h
        src/state/stage-registers/IDEXStageRegisters.cpp
)

target_sources(RISC_V_Simulator PUBLIC ${CMAKE_SOURCE_DIR}/include)