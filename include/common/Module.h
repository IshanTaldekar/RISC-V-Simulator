#ifndef RISC_V_SIMULATOR_MODULE_H
#define RISC_V_SIMULATOR_MODULE_H

#include <mutex>
#include <condition_variable>
#include <thread>

class Module {
    bool is_alive;
    std::mutex module_mutex;
    std::condition_variable module_condition_variable;

public:
    Module();

    virtual void run() = 0;
    virtual void notifyModuleConditionVariable() = 0;

    void kill();
    [[nodiscard]] bool isAlive() const;

    std::mutex &getModuleMutex();
    std::condition_variable &getModuleConditionVariable();
};

#endif //RISC_V_SIMULATOR_MODULE_H
