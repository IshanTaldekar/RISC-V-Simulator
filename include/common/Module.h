#ifndef RISC_V_SIMULATOR_MODULE_H
#define RISC_V_SIMULATOR_MODULE_H

#include <mutex>
#include <condition_variable>
#include <thread>

#include "Config.h"

class Module {
    bool is_alive;

    std::mutex module_mutex;
    std::condition_variable module_condition_variable;

    PipelineType pipeline_type;

public:
    Module();

    virtual void run() = 0;
    void notifyModuleConditionVariable();

    void kill();
    [[nodiscard]] bool isAlive() const;
    [[nodiscard]] bool isKilled() const;

    std::mutex &getModuleMutex();
    std::condition_variable &getModuleConditionVariable();

    void setPipelineType(PipelineType current_type);
    PipelineType getPipelineType();
};

#endif //RISC_V_SIMULATOR_MODULE_H
