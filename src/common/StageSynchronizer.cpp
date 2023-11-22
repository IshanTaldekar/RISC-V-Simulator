#include "../../include/common/StageSynchronizer.h"

StageSynchronizer *StageSynchronizer::current_instance = nullptr;

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

    this->is_single_stage_mode = true;
}

void StageSynchronizer::conditionalArriveFiveStage() {
    if (!this->is_single_stage_mode) {
        this->five_stage_barrier->arrive_and_wait();
    }
}

StageSynchronizer* StageSynchronizer::init() {
    if (StageSynchronizer::current_instance == nullptr) {
        StageSynchronizer::current_instance = new StageSynchronizer();
    }

    return StageSynchronizer::current_instance;
}

void StageSynchronizer::conditionalArriveSingleStage() {
    if (this->is_single_stage_mode) {
        this->single_stage_barrier->arrive_and_wait();
    }
}

void StageSynchronizer::onCompletionFiveStage() {
    // TODO
}

void StageSynchronizer::onCompletionSingleStage() {
    // TODO
}

void StageSynchronizer::setStage(Stage current_stage) {
    this->is_single_stage_mode = current_stage == Stage::Single;
}