#include "../../include/common/StageSynchronizer.h"

StageSynchronizer *StageSynchronizer::current_instance = nullptr;
std::mutex StageSynchronizer::initialization_mutex;

StageSynchronizer::StageSynchronizer() {
    std::function<void()> single_stage_on_completion = [this]() -> void { this->onCompletionSingleStage(); };
    std::function<void()> five_stage_on_completion = [this]() -> void { this->onCompletionFiveStage(); };

    this->single_stage_barrier = new std::barrier<std::function<void()>>(SINGLE_STAGE_THREAD_COUNT, single_stage_on_completion);
    this->five_stage_barrier = new std::barrier<std::function<void()>>(FIVE_STAGE_THREAD_COUNT, five_stage_on_completion);

    this->driver = Driver::init();
    this->if_id_stage_registers = IFIDStageRegisters::init();
    this->id_ex_stage_registers = IDEXStageRegisters::init();
    this->ex_mem_stage_registers = EXMEMStageRegisters::init();
    this->mem_wb_stage_registers = MEMWBStageRegisters::init();
    this->hazard_detection_unit = HazardDetectionUnit::init();

    this->register_file = RegisterFile::init();
    this->data_memory = DataMemory::init();
    this->is_paused = false;
    this->halt_detected = false;

    this->current_cycle = 0;

    this->current_pipeline_type = PipelineType::Single;
}

void StageSynchronizer::conditionalArriveFiveStage() {
    if (this->current_pipeline_type == PipelineType::Five) {
        this->five_stage_barrier->arrive_and_wait();
    }
}

StageSynchronizer* StageSynchronizer::init() {
    std::lock_guard<std::mutex> stage_synchronizer_lock (StageSynchronizer::initialization_mutex);

    if (StageSynchronizer::current_instance == nullptr) {
        StageSynchronizer::current_instance = new StageSynchronizer();
    }

    return StageSynchronizer::current_instance;
}

void StageSynchronizer::conditionalArriveSingleStage() {
    if (this->current_pipeline_type == PipelineType::Single) {
        this->single_stage_barrier->arrive_and_wait();
    }
}

void StageSynchronizer::onCompletionFiveStage() {
    if (this->current_cycle == 0) {
        std::cout << std::endl << "Five Stage Execution: " << std::endl << std::string(20, '-') << std::endl;
    }

    std::cout << "Cycle: " << this->current_cycle << std::endl;

    this->register_file->writeRegisterFileContentsToOutputFile(this->current_cycle++);

    if (this->mem_wb_stage_registers->isExecutingHaltInstruction()) {
        if (!this->mem_wb_stage_registers->is_nop_asserted) {
            this->halt_detected = true;
        }
    }

    if (this->halt_detected) {
        this->driver->pause();
        this->if_id_stage_registers->pause();
        this->id_ex_stage_registers->pause();
        this->ex_mem_stage_registers->pause();
        this->mem_wb_stage_registers->pause();

        this->data_memory->writeDataMemoryContentsToOutput();

        this->is_paused = true;
    }
}

void StageSynchronizer::onCompletionSingleStage() {
    if (this->current_cycle == 0) {
        std::cout << std::endl << "Single Stage Execution: " << std::endl << std::string(20, '-') << std::endl;
    }

    std::cout << "Cycle: " << this->current_cycle << std::endl;

    this->register_file->writeRegisterFileContentsToOutputFile(this->current_cycle++);

    if (this->if_id_stage_registers->getInstruction()->getType() == InstructionType::HALT) {
        this->driver->pause();
        this->hazard_detection_unit->pause();

        this->data_memory->writeDataMemoryContentsToOutput();
        this->is_paused = true;
    }
}

void StageSynchronizer::setPipelineType(PipelineType new_pipeline_type) {
    this->current_pipeline_type = new_pipeline_type;
}

void StageSynchronizer::reset() {
    this->current_cycle = 0;
    this->is_paused = false;
}

bool StageSynchronizer::isPaused() const {
    return this->is_paused;
}

PipelineType StageSynchronizer::getPipelineType() {
    return this->current_pipeline_type;
}