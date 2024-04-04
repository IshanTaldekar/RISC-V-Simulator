#include "../../include/common/Module.h"

Module::Module() {
    this->pipeline_type = PipelineType::Single;
    this->is_alive = true;
    this->logger = nullptr;
}

void Module::kill() {
    this->is_alive = false;
    this->notifyModuleConditionVariable();
}

bool Module::isAlive() const {
    return this->is_alive;
}

bool Module::isKilled() const {
    return !this->is_alive;
}

std::mutex &Module::getModuleMutex() {
    return this->module_mutex;
}

std::condition_variable &Module::getModuleConditionVariable() {
    return this->module_condition_variable;
}

void Module::setPipelineType(PipelineType current_type) {
    std::lock_guard<std::mutex> module_lock (this->module_mutex);
    pipeline_type = current_type;
}

PipelineType Module::getPipelineType() {
    return this->pipeline_type;
}

void Module::notifyModuleConditionVariable() {
    this->getModuleConditionVariable().notify_one();
}

void Module::log(const std::string &message) {
    if (!this->logger) {
        this->initDependencies();
    }

    this->logger->log(this->getModuleStage(), "[" + this->getModuleTag() + "] " + message);
}