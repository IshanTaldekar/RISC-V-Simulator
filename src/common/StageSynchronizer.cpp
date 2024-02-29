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
    std::cout << this->current_cycle++ << std::endl;
}

void StageSynchronizer::onCompletionSingleStage() {
    std::cout << this->current_cycle++ << std::endl;
}

void StageSynchronizer::setStage(PipelineType new_pipeline_type) {
    this->current_pipeline_type = new_pipeline_type;
}