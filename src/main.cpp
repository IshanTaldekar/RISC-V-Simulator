#include <string>

#include "../include/common/StageSynchronizer.h"

class EXAdder;
class IFAdder;
class ALUInput1ForwardingMux;
class ALUInput2ForwardingMux;
class EXMuxALUInput1;
class EXMuxALUInput2;
class IFMux;
class WBMux;
class ALU;
class ForwardingUnit;
class ImmediateGenerator;
class StageSynchronizer;
class EXMEMStageRegisters;
class IDEXStageRegisters;
class IFIDStageRegisters;
class MEMWBStageRegisters;
class DataMemory;
class Driver;
class InstructionMemory;
class RegisterFile;

struct Pipeline {
    EXAdder *ex_adder = nullptr;
    IFAdder *if_adder = nullptr;
    ALUInput1ForwardingMux *alu_input_1_forwarding_mux = nullptr;
    ALUInput2ForwardingMux *alu_input_2_forwarding_mux = nullptr;
    EXMuxALUInput1 *ex_mux_alu_input_1 = nullptr;
    EXMuxALUInput2 *ex_mux_alu_input_2 = nullptr;
    IFMux *if_mux = nullptr;
    WBMux *wb_mux = nullptr;
    ALU *alu = nullptr;
    Logger *logger = nullptr;
    ForwardingUnit *forwarding_unit = nullptr;
    ImmediateGenerator *immediate_generator = nullptr;
    StageSynchronizer *stage_synchronizer = nullptr;
    EXMEMStageRegisters *ex_mem_stage_registers = nullptr;
    IDEXStageRegisters *id_ex_stage_registers = nullptr;
    IFIDStageRegisters *if_id_stage_registers = nullptr;
    MEMWBStageRegisters *mem_wb_stage_registers = nullptr;
    DataMemory *data_memory = nullptr;
    Driver *driver = nullptr;
    InstructionMemory *instruction_memory = nullptr;
    RegisterFile *register_file = nullptr;
};

struct ActiveThreads {
    std::thread ex_adder_thread;
    std::thread if_adder_thread;
    std::thread alu_input_1_forwarding_mux_thread;
    std::thread alu_input_2_forwarding_mux_thread;
    std::thread ex_mux_alu_input_1_thread;
    std::thread ex_mux_alu_input_2_thread;
    std::thread if_mux_thread;
    std::thread wb_mux_thread;
    std::thread alu_thread;
    std::thread forwarding_unit_thread;
    std::thread immediate_generator_thread;
    std::thread ex_mem_stage_registers_thread;
    std::thread id_ex_stage_registers_thread;
    std::thread if_id_stage_registers_thread;
    std::thread mem_wb_stage_registers_thread;
    std::thread data_memory_thread;
    std::thread logger_thread;
    std::thread driver_thread;
    std::thread instruction_memory_thread;
    std::thread register_file_thread;
};

void resetPipeline(Pipeline &pipeline) {
    pipeline.ex_mem_stage_registers->reset();
    pipeline.id_ex_stage_registers->reset();
    pipeline.if_id_stage_registers->reset();
    pipeline.mem_wb_stage_registers->reset();
    pipeline.driver->reset();
    pipeline.register_file->reset();
    pipeline.data_memory->reset();
}

void pausePipeline(Pipeline &pipeline) {
    pipeline.ex_mem_stage_registers->pause();
    pipeline.id_ex_stage_registers->pause();
    pipeline.if_id_stage_registers->pause();
    pipeline.mem_wb_stage_registers->pause();
    pipeline.driver->pause();
}

void resumePipeline(Pipeline &pipeline) {
    pipeline.ex_mem_stage_registers->resume();
    pipeline.id_ex_stage_registers->resume();
    pipeline.if_id_stage_registers->resume();
    pipeline.mem_wb_stage_registers->resume();
    pipeline.driver->resume();
}

void changePipelineType(Pipeline &pipeline, const PipelineType &type) {
    pipeline.ex_adder->setPipelineType(type);
    pipeline.if_adder->setPipelineType(type);
    pipeline.alu_input_1_forwarding_mux->setPipelineType(type);
    pipeline.alu_input_2_forwarding_mux->setPipelineType(type);
    pipeline.ex_mux_alu_input_1->setPipelineType(type);
    pipeline.ex_mux_alu_input_2->setPipelineType(type);
    pipeline.if_mux->setPipelineType(type);
    pipeline.wb_mux->setPipelineType(type);
    pipeline.alu->setPipelineType(type);
    pipeline.forwarding_unit->setPipelineType(type);
    pipeline.immediate_generator->setPipelineType(type);
    pipeline.ex_mem_stage_registers->setPipelineType(type);
    pipeline.id_ex_stage_registers->setPipelineType(type);
    pipeline.if_id_stage_registers->setPipelineType(type);
    pipeline.mem_wb_stage_registers->setPipelineType(type);
    pipeline.data_memory->setPipelineType(type);
    pipeline.driver->setPipelineType(type);
    pipeline.instruction_memory->setPipelineType(type);
    pipeline.register_file->setPipelineType(type);
}

void killPipeline(Pipeline &pipeline) {
    pipeline.ex_adder->kill();
    pipeline.if_adder->kill();
    pipeline.alu_input_1_forwarding_mux->kill();
    pipeline.alu_input_2_forwarding_mux->kill();
    pipeline.ex_mux_alu_input_1->kill();
    pipeline.ex_mux_alu_input_2->kill();
    pipeline.if_mux->kill();
    pipeline.wb_mux->kill();
    pipeline.alu->kill();
    pipeline.logger->kill();
    pipeline.forwarding_unit->kill();
    pipeline.immediate_generator->kill();
    pipeline.ex_mem_stage_registers->kill();
    pipeline.id_ex_stage_registers->kill();
    pipeline.if_id_stage_registers->kill();
    pipeline.mem_wb_stage_registers->kill();
    pipeline.data_memory->kill();
    pipeline.driver->kill();
    pipeline.instruction_memory->kill();
    pipeline.register_file->kill();
}

Pipeline initializePipeline() {
    Pipeline pipeline;

    pipeline.ex_adder = EXAdder::init();
    pipeline.if_adder = IFAdder::init();
    pipeline.alu_input_1_forwarding_mux =  ALUInput1ForwardingMux::init();
    pipeline.alu_input_2_forwarding_mux = ALUInput2ForwardingMux::init();
    pipeline.ex_mux_alu_input_1 = EXMuxALUInput1::init();
    pipeline.ex_mux_alu_input_2 = EXMuxALUInput2::init();
    pipeline.if_mux = IFMux::init();
    pipeline.wb_mux = WBMux::init();
    pipeline.alu = ALU::init();
    pipeline.logger = Logger::init();
    pipeline.forwarding_unit = ForwardingUnit::init();
    pipeline.immediate_generator = ImmediateGenerator::init();
    pipeline.stage_synchronizer = StageSynchronizer::init();
    pipeline.ex_mem_stage_registers = EXMEMStageRegisters::init();
    pipeline.id_ex_stage_registers = IDEXStageRegisters::init();
    pipeline.if_id_stage_registers = IFIDStageRegisters::init();
    pipeline.mem_wb_stage_registers = MEMWBStageRegisters::init();
    pipeline.data_memory = DataMemory::init();
    pipeline.driver = Driver::init();
    pipeline.instruction_memory = InstructionMemory::init();
    pipeline.register_file = RegisterFile::init();

    return pipeline;
}

ActiveThreads runPipeline(const Pipeline &pipeline) {
    ActiveThreads active_threads;

    active_threads.ex_adder_thread = std::thread(&EXAdder::run, pipeline.ex_adder);
    active_threads.if_adder_thread = std::thread(&IFAdder::run, pipeline.if_adder);

    active_threads.alu_input_1_forwarding_mux_thread = std::thread(
            &ALUInput1ForwardingMux::run,
            pipeline.alu_input_1_forwarding_mux
    );

    active_threads.alu_input_2_forwarding_mux_thread = std::thread(
            &ALUInput2ForwardingMux::run,
            pipeline.alu_input_2_forwarding_mux
    );

    active_threads.ex_mux_alu_input_1_thread = std::thread(&EXMuxALUInput1::run, pipeline.ex_mux_alu_input_1);
    active_threads.ex_mux_alu_input_2_thread = std::thread(&EXMuxALUInput2::run, pipeline.ex_mux_alu_input_2);
    active_threads.if_mux_thread = std::thread(&IFMux::run, pipeline.if_mux);
    active_threads.wb_mux_thread = std::thread(&WBMux::run, pipeline.wb_mux);
    active_threads.alu_thread = std::thread(&ALU::run, pipeline.alu);
    active_threads.forwarding_unit_thread = std::thread(&ForwardingUnit::run, pipeline.forwarding_unit);
    active_threads.immediate_generator_thread = std::thread(&ImmediateGenerator::run, pipeline.immediate_generator);
    active_threads.ex_mem_stage_registers_thread = std::thread(
            &EXMEMStageRegisters::run,
            pipeline.ex_mem_stage_registers
    );

    active_threads.id_ex_stage_registers_thread = std::thread(
            &IDEXStageRegisters::run,
            pipeline.id_ex_stage_registers
    );

    active_threads.if_id_stage_registers_thread = std::thread(
            &IFIDStageRegisters::run,
            pipeline.if_id_stage_registers
    );

    active_threads.mem_wb_stage_registers_thread = std::thread(
            &MEMWBStageRegisters::run,
            pipeline.mem_wb_stage_registers
    );

    active_threads.data_memory_thread = std::thread(&DataMemory::run, pipeline.data_memory);
    active_threads.driver_thread = std::thread(&Driver::run, pipeline.driver);
    active_threads.instruction_memory_thread = std::thread(&InstructionMemory::run, pipeline.instruction_memory);
    active_threads.register_file_thread = std::thread(&RegisterFile::run, pipeline.register_file);

    active_threads.ex_adder_thread.detach();
    active_threads.if_adder_thread.detach();
    active_threads.alu_input_1_forwarding_mux_thread.detach();
    active_threads.alu_input_2_forwarding_mux_thread.detach();
    active_threads.ex_mux_alu_input_1_thread.detach();
    active_threads.ex_mux_alu_input_2_thread.detach();
    active_threads.if_mux_thread.detach();
    active_threads.wb_mux_thread.detach();
    active_threads.alu_thread.detach();
    active_threads.forwarding_unit_thread.detach();
    active_threads.immediate_generator_thread.detach();
    active_threads.ex_mem_stage_registers_thread.detach();
    active_threads.id_ex_stage_registers_thread.detach();
    active_threads.if_id_stage_registers_thread.detach();
    active_threads.mem_wb_stage_registers_thread.detach();
    active_threads.data_memory_thread.detach();
    active_threads.driver_thread.detach();
    active_threads.instruction_memory_thread.detach();
    active_threads.register_file_thread.detach();

    return active_threads;
}

void changeInstructionMemoryFile(Pipeline &pipeline, const std::string &new_file_path) {
    pipeline.instruction_memory->setInstructionMemoryInputFilePath(new_file_path);
}

void changeDataMemoryFile(Pipeline &pipeline, const std::string &new_file_path) {
    pipeline.data_memory->setDataMemoryInputFilePath(new_file_path);
}

int main() {
    Pipeline pipeline = initializePipeline();

    changeInstructionMemoryFile(pipeline, "../input/imem.txt");
    changeDataMemoryFile(pipeline, "../input/dmem.txt");

    ActiveThreads active_threads = runPipeline(pipeline);

    while (true) {
        sleep(1);
    }
}