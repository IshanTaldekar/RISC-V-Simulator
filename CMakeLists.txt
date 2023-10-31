cmake_minimum_required(VERSION 3.26)
project(RISC_V_Simulator)

set(CMAKE_CXX_STANDARD 20)

add_executable(RISC_V_Simulator
        src/main.cpp
        src/combinational/IFAdder.cpp
        src/common/Config.cpp
        src/state/Driver.cpp
        src/state/InstructionMemory.cpp
        src/combinational/IFMux.cpp
        include/common/Module.h
        src/common/Module.cpp
        src/state/IFIDStageRegisters.cpp
        include/common/Instruction.h
        include/common/logger/Logger.h
)

target_sources(RISC_V_Simulator PUBLIC ${CMAKE_SOURCE_DIR}/include)