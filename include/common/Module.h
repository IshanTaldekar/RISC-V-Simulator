#ifndef RISC_V_SIMULATOR_MODULE_H
#define RISC_V_SIMULATOR_MODULE_H

#include <mutex>
#include <condition_variable>
#include <thread>
#include <string>

#include "Config.h"
#include "Logger.h"

class Logger;

class Module {
    bool is_alive;

    std::mutex module_mutex;
    std::condition_variable module_condition_variable;

    std::mutex log_mutex;
    std::mutex dependency_mutex;

    PipelineType pipeline_type;

public:
    Module();

    virtual void run() = 0;
    void notifyModuleConditionVariable();

    void kill();
    [[nodiscard]] bool isAlive() const;
    [[nodiscard]] bool isKilled() const;

    std::mutex &getModuleMutex();
    std::mutex &getModuleDependencyMutex();

    std::condition_variable &getModuleConditionVariable();

    void setPipelineType(PipelineType current_type);
    PipelineType getPipelineType();

protected:
    Logger *logger;

    virtual void initDependencies() = 0;
    virtual void log(const std::string &message);
    virtual std::string getModuleTag() = 0;
    virtual Stage getModuleStage() = 0;
};

#endif //RISC_V_SIMULATOR_MODULE_H
