#ifndef RISC_V_SIMULATOR_STAGESYNCHRONIZER_H
#define RISC_V_SIMULATOR_STAGESYNCHRONIZER_H

#include <barrier>
#include <functional>

#include "Config.h"
#include "../state/stage-registers/IFIDStageRegisters.h"
#include "../state/stage-registers/IDEXStageRegisters.h"
#include "../state/stage-registers/EXMEMStageRegisters.h"
#include "../state/stage-registers/MEMWBStageRegisters.h"
#include "../state/Driver.h"

class Driver;
class IFIDStageRegisters;
class IDEXStageRegisters;
class EXMEMStageRegisters;
class MEMWBStageRegisters;
class RegisterFile;
class DataMemory;

class StageSynchronizer {
    std::barrier<std::function<void()>> *single_stage_barrier;
    std::barrier<std::function<void()>> *five_stage_barrier;

    PipelineType current_pipeline_type;

    static constexpr int SINGLE_STAGE_THREAD_COUNT = 5;
    static constexpr int FIVE_STAGE_THREAD_COUNT = 17;

    static StageSynchronizer *current_instance;
    static std::mutex initialization_mutex;

    Driver *driver;
    IFIDStageRegisters *if_id_stage_registers;
    IDEXStageRegisters *id_ex_stage_registers;
    EXMEMStageRegisters *ex_mem_stage_registers;
    MEMWBStageRegisters *mem_wb_stage_registers;

    RegisterFile *register_file;
    DataMemory *data_memory;

    int current_cycle;

public:
    StageSynchronizer();

    static StageSynchronizer *init();

    void conditionalArriveSingleStage();
    void conditionalArriveFiveStage();

    void setStage(PipelineType new_pipeline_type);

private:
    void onCompletionSingleStage();
    void onCompletionFiveStage();
};

#endif //RISC_V_SIMULATOR_STAGESYNCHRONIZER_H
