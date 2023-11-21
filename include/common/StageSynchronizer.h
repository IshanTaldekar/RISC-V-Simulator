#ifndef RISC_V_SIMULATOR_STAGESYNCHRONIZER_H
#define RISC_V_SIMULATOR_STAGESYNCHRONIZER_H

#include <barrier>
#include <functional>

#include "Config.h"

class StageSynchronizer {
    std::barrier<std::function<void()>> *single_stage_barrier;
    std::barrier<std::function<void()>> *five_stage_barrier;

    bool is_single_stage_mode;

    static constexpr int SINGLE_STAGE_THREAD_COUNT = 5;
    static constexpr int FIVE_STAGE_THREAD_COUNT = 17;

    static StageSynchronizer *current_instance;

public:
    StageSynchronizer();

    static StageSynchronizer *init();

    void conditionalArriveSingleStage();
    void conditionalArriveFiveStage();

    void setStage(Stage current_stage);

private:
    void onCompletionSingleStage();
    void onCompletionFiveStage();
};

#endif //RISC_V_SIMULATOR_STAGESYNCHRONIZER_H
