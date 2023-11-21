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

    Stage stage;

public:
    Module();

    virtual void run() = 0;
    virtual void notifyModuleConditionVariable() = 0;

    void kill();
    [[nodiscard]] bool isAlive() const;

    std::mutex &getModuleMutex();
    std::condition_variable &getModuleConditionVariable();

    void setStage(Stage current_stage);
    Stage getStage();
};

#endif //RISC_V_SIMULATOR_MODULE_H
