#include "../../../include/combinational/adder/IFAdder.h"

IFAdder *IFAdder::current_instance = nullptr;

IFAdder::IFAdder() {
    this->program_counter = 0;
    this->is_program_counter_set = false;

    this->if_mux = IFMux::init();
    this->logger = IFLogger::init();
}

IFAdder *IFAdder::init() {
    if (IFAdder::current_instance == nullptr) {
        IFAdder::current_instance = new IFAdder();
    }

    return IFAdder::current_instance;
}

void IFAdder::run() {
    while (this->isAlive()) {
        this->logger->log("[IFAdder] Waiting to acquire lock and wake up.");

        std::unique_lock<std::mutex> adder_lock(this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                adder_lock,
                [this] { return this->is_program_counter_set; }
        );

        if (this->isKilled()) {
            break;
        }

        this->logger->log("[IFAdder] Woken up and acquired lock. Loading PC to IFMux.");

        this->passProgramCounterToIFMux();
        this->if_mux->notifyModuleConditionVariable();

        this->is_program_counter_set = false;

        this->logger->log("[IFAdder] Load into IFMux successful.");
    }
}

void IFAdder::setInput(AdderInputType type, unsigned long value) {
    if (!std::holds_alternative<IFAdderInputType>(type)) {
        throw std::runtime_error("AdderInputType passed in IFAdder not compatible with IFAdderInputTypes");
    }

    this->logger->log("[IFAdder] Waiting to set input.");

    std::unique_lock<std::mutex> adder_lock(this->getModuleMutex());

    if (std::get<IFAdderInputType>(type) == IFAdderInputType::PCValue) {
        this->program_counter = value;
        this->is_program_counter_set = true;

        this->logger->log("[IFAdder] Input PCValue set.");
    }

    this->notifyModuleConditionVariable();
}

void IFAdder::passProgramCounterToIFMux() {
    this->logger->log("[IFAdder] Waiting to pass PCValue to IFMux.");
    this->if_mux->setInput(IFStageMuxInputType::IncrementedPc, this->program_counter);
    this->logger->log("[IFAdder] PCValue passed to IFMux.");
}

void IFAdder::notifyModuleConditionVariable() {
    this->logger->log("[IFAdder] Notifying condition variable.");
    this->getModuleConditionVariable().notify_one();
}