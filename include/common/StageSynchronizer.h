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
#include "../combinational/HazardDetectionUnit.h"

class Driver;
class IFIDStageRegisters;
class IDEXStageRegisters;
class EXMEMStageRegisters;
class MEMWBStageRegisters;
class RegisterFile;
class DataMemory;
class HazardDetectionUnit;

class StageSynchronizer {
    std::barrier<std::function<void()>> *single_stage_barrier;
    std::barrier<std::function<void()>> *five_stage_barrier;
    std::barrier<std::function<void()>> *reset_barrier;

    PipelineType current_pipeline_type;

    static constexpr int SINGLE_STAGE_THREAD_COUNT = 6;
    static constexpr int FIVE_STAGE_THREAD_COUNT = 34;
    static constexpr int RESET_THREAD_COUNT = 10;

    static StageSynchronizer *current_instance;
    static std::mutex initialization_mutex;

    Driver *driver;
    IFIDStageRegisters *if_id_stage_registers;
    IDEXStageRegisters *id_ex_stage_registers;
    EXMEMStageRegisters *ex_mem_stage_registers;
    MEMWBStageRegisters *mem_wb_stage_registers;
    HazardDetectionUnit *hazard_detection_unit;
    RegisterFile *register_file;
    DataMemory *data_memory;

    int current_cycle;
    bool halt_detected;
    bool is_paused;

    bool is_print_driver_state_asserted;
    bool is_print_if_id_state_asserted;
    bool is_print_id_ex_state_asserted;
    bool is_print_ex_mem_state_asserted;
    bool is_print_mem_wb_state_asserted;

    std::mutex module_mutex;
    std::mutex print_mutex;

public:
    StageSynchronizer();

    static StageSynchronizer *init();

    void conditionalArriveSingleStage();
    void conditionalArriveFiveStage();
    void arriveReset();

    void setPipelineType(PipelineType new_pipeline_type);
    PipelineType getPipelineType();

    [[nodiscard]] bool isPaused();
    void reset();

private:
    void onCompletionSingleStage();
    void onCompletionFiveStage();
    static void onCompletionReset();
};

#endif //RISC_V_SIMULATOR_STAGESYNCHRONIZER_H
