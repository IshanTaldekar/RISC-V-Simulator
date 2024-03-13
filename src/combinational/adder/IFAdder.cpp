#include "../../../include/combinational/adder/IFAdder.h"

IFAdder *IFAdder::current_instance = nullptr;
std::mutex IFAdder::initialization_mutex;

IFAdder::IFAdder() {
    this->program_counter = 0;
    this->is_program_counter_set = false;

    this->if_mux = nullptr;
    this->logger = nullptr;
}

IFAdder *IFAdder::init() {
    std::lock_guard<std::mutex> if_adder_lock (IFAdder::initialization_mutex);

    if (IFAdder::current_instance == nullptr) {
        IFAdder::current_instance = new IFAdder();
    }

    return IFAdder::current_instance;
}

void IFAdder::initDependencies() {
    this->if_mux = IFMux::init();
    this->logger = Logger::init();
}

void IFAdder::run() {
    this->initDependencies();

    while (this->isAlive()) {
        this->logger->log(Stage::IF, "[IFAdder] Waiting to acquire lock and wake up.");

        std::unique_lock<std::mutex> adder_lock(this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                adder_lock,
                [this] { return this->is_program_counter_set; }
        );

        if (this->isKilled()) {
            this->logger->log(Stage::IF, "[IFAdder] Killed.");
            break;
        }

        this->logger->log(Stage::IF, "[IFAdder] Woken up and acquired lock.");

        this->passProgramCounterToIFMux();

        this->is_program_counter_set = false;
    }
}

void IFAdder::setInput(const AdderInputType &type, const AdderInputDataType &value) {
    if (!std::holds_alternative<IFAdderInputType>(type) ||
            !std::holds_alternative<unsigned long>(value)) {
        throw std::runtime_error("IFAdder::setInput: incompatible type passed.");
    }

    this->logger->log(Stage::IF, "[IFAdder] setInput waiting to acquire lock and update values.");

    std::unique_lock<std::mutex> adder_lock(this->getModuleMutex());

    if (std::get<IFAdderInputType>(type) == IFAdderInputType::PCValue) {
        this->program_counter = std::get<unsigned long>(value);
        this->is_program_counter_set = true;

        this->logger->log(Stage::IF, "[IFAdder] setInput program counter set.");
    } else {
        this->logger->log(Stage::IF, "[IFAdder] setInput program counter update failed.");
    }

    this->notifyModuleConditionVariable();
}

void IFAdder::passProgramCounterToIFMux() {
    this->logger->log(Stage::IF, "[IFAdder] Waiting to pass PCValue to IFMux.");
    this->if_mux->setInput(IFStageMuxInputType::IncrementedPc, this->program_counter + 4);
    this->logger->log(Stage::IF, "[IFAdder] PCValue passed to IFMux.");
}