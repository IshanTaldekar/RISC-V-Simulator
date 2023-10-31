#ifndef RISC_V_SIMULATOR_MODULE_H
#define RISC_V_SIMULATOR_MODULE_H

#include <mutex>
#include <condition_variable>

class Module {
    bool is_alive;
    std::mutex module_mutex;
    std::condition_variable module_condition_variable;

public:
    Module();

    virtual void run() = 0;
    virtual void notifyConditionVariable() = 0;

    void kill();
    [[nodiscard]] bool isAlive() const;

    std::mutex &getMutex();
    std::condition_variable &getConditionVariable();

};

#endif //RISC_V_SIMULATOR_MODULE_H
