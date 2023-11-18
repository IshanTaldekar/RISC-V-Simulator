#include "../../../include/combinational/mux/IFMux.h"

IFMux *IFMux::current_instance = nullptr;

IFMux::IFMux() {
    this->driver = Driver::init();
    this->logger = IFLogger::init();

    this->is_pc_src_signal_asserted = false;
    this->is_incremented_pc_set = false;
    this->is_branched_pc_set = false;

    this->incremented_pc = -1;
    this->branched_pc = -1;

    this->is_control_signal_set = false;
}

IFMux *IFMux::init() {
    if (IFMux::current_instance == nullptr) {
        IFMux::current_instance = new IFMux();
    }

    return IFMux::current_instance;
}

void IFMux::run() {
    while (this->isAlive()) {
        this->logger->log("[IFMux] Waiting to wakeup and acquire lock.");

        std::unique_lock<std::mutex> mux_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                mux_lock,
                [this] {
                    return this->is_branched_pc_set && this->is_incremented_pc_set && this->is_control_signal_set;
                }
        );

        this->logger->log("[IFMux] Woken up and acquired lock. Loading output");

        this->passOutput();

        this->is_pc_src_signal_asserted = false;
        this->is_branched_pc_set = false;
        this->is_incremented_pc_set = false;
        this->is_control_signal_set = false;

        this->logger->log("[IFMux] Woken up and acquired lock. Loading output");
    }
}

void IFMux::setInput(StageMuxInputType type, unsigned long value) {
    if (!std::holds_alternative<IFStageMuxInputType>(type)) {
        throw std::runtime_error("StageMuxInputType passed to IFMux not compatible with IFStageMuxInputTypes");
    }

    this->logger->log("[IFMux] setInput waiting to acquire lock");

    std::unique_lock<std::mutex> mux_lock (this->getModuleMutex());

    if (std::get<IFStageMuxInputType>(type) == IFStageMuxInputType::IncrementedPc) {
        this->incremented_pc = value;
        this->is_incremented_pc_set = true;

        this->logger->log("[IFMux] incremented PC value set");
    } else if (std::get<IFStageMuxInputType>(type) == IFStageMuxInputType::BranchedPc) {
        this->branched_pc = value;
        this->is_branched_pc_set = true;

        this->logger->log("[IFMux] branched PC value set");
    } else {
        std::cerr << "IFMux::setInput did not match any input type" << std::endl;
    }
}

void IFMux::assertControlSignal(bool is_asserted) {
    this->is_pc_src_signal_asserted = is_asserted;
    this->is_control_signal_set = true;

    this->logger->log("[IFMux] PCSrc asserted: " + std::to_string(this->is_pc_src_signal_asserted) + ".");
}

/**
 * Loading output to the driver
 */
void IFMux::passOutput() {
    this->logger->log("[IFMux] Loading output to Driver.");

    if (this->is_pc_src_signal_asserted) {
        this->driver->setProgramCounter(this->incremented_pc);
        this->logger->log("[IFMux] Loaded PC to Driver.");
    } else {
        this->driver->setProgramCounter(this->branched_pc);
        this->logger->log("[IFMux] Loaded Branch Address to Driver.");
    }
}

void IFMux::notifyModuleConditionVariable() {
    this->getModuleConditionVariable().notify_one();
}