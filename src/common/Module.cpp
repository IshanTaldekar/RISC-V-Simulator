#include "../../include/common/Module.h"

Module::Module() {
    this->stage = Stage::Five;
    this->is_alive = true;
}

void Module::kill() {
    this->is_alive = false;
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

void Module::setStage(Stage current_stage) {
    std::lock_guard<std::mutex> module_lock (this->module_mutex);

    stage = current_stage;
}

Stage Module::getStage() {
    return this->stage;
}