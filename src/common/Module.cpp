#include "../../include/common/Module.h"

Module::Module() {
    this->is_alive = true;
}

void Module::kill() {
    this->is_alive = false;
}

bool Module::isAlive() const {
    return this->is_alive;
}

std::mutex &Module::getMutex() {
    return this->module_mutex;
}

std::condition_variable &Module::getConditionVariable() {
    return this->module_condition_variable;
}