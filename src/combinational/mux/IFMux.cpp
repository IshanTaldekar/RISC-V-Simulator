#include "../../../include/combinational/mux/IFMux.h"

IFMux *IFMux::current_instance = nullptr;

IFMux::IFMux() {
    this->is_pc_src_signal_asserted = false;
    this->is_incremented_pc_set = false;
    this->is_branched_pc_set = false;

    this->incremented_pc = 0UL;
    this->branched_pc = 0UL;

    this->is_control_signal_set = false;

    this->driver = nullptr;
    this->logger = nullptr;
}

IFMux *IFMux::init() {
    if (IFMux::current_instance == nullptr) {
        IFMux::current_instance = new IFMux();
    }

    return IFMux::current_instance;
}

void IFMux::initDependencies() {
    this->driver = Driver::init();
    this->logger = Logger::init();
}

void IFMux::run() {
    this->initDependencies();

    while (this->isAlive()) {
        this->logger->log(Stage::IF, "[IFMux] Waiting to be woken up and acquire lock.");

        std::unique_lock<std::mutex> mux_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                mux_lock,
                [this] {
                    return (this->is_branched_pc_set && this->is_incremented_pc_set && this->is_control_signal_set) ||
                    this->isKilled();
                }
        );

        if (this->isKilled()) {
            this->logger->log(Stage::IF, "[IFMux] Killed.");
            break;
        }

        this->logger->log(Stage::IF, "[IFMux] Woken up and acquired lock.");

        this->passOutput();

        this->is_pc_src_signal_asserted = false;
        this->is_branched_pc_set = false;
        this->is_incremented_pc_set = false;
        this->is_control_signal_set = false;
    }
}

void IFMux::setInput(MuxInputType type, unsigned long value) {
    if (!std::holds_alternative<IFStageMuxInputType>(type)) {
        throw std::runtime_error("MuxInputType passed to IFMux not compatible with IFStageMuxInputTypes");
    }

    this->logger->log(Stage::IF, "[IFMux] setInput waiting to acquire lock");

    std::unique_lock<std::mutex> mux_lock (this->getModuleMutex());

    this->logger->log(Stage::IF, "[IFMux] setInput acquired lock. Updating values.");

    if (std::get<IFStageMuxInputType>(type) == IFStageMuxInputType::IncrementedPc) {
        this->incremented_pc = value;
        this->is_incremented_pc_set = true;

        this->logger->log(Stage::IF, "[IFMux] incremented PC value set");
    } else if (std::get<IFStageMuxInputType>(type) == IFStageMuxInputType::BranchedPc) {
        this->branched_pc = value;
        this->is_branched_pc_set = true;

        this->logger->log(Stage::IF, "[IFMux] branched PC value set");
    } else {
        throw std::runtime_error("IFMux::setInput did not match any input type.");
    }

    this->notifyModuleConditionVariable();
}

void IFMux::assertControlSignal(bool is_asserted) {
    this->logger->log(Stage::IF, "[IFMux] assertControlSignal waiting to acquire lock.");

    std::lock_guard<std::mutex> if_mux_lock (this->getModuleMutex());

    this->is_pc_src_signal_asserted = is_asserted;
    this->is_control_signal_set = true;

    this->logger->log(Stage::IF, "[IFMux] PCSrc asserted: " + std::to_string(this->is_pc_src_signal_asserted) + ".");
    this->notifyModuleConditionVariable();
}

/**
 * Loading output to the Driver.
 */
void IFMux::passOutput() {
    this->logger->log(Stage::IF, "[IFMux] Passing output to Driver.");

    if (this->is_pc_src_signal_asserted) {
        this->driver->setProgramCounter(this->incremented_pc);
        this->logger->log(Stage::IF, "[IFMux] Passed PC to Driver.");
    } else {
        this->driver->setProgramCounter(this->branched_pc);
        this->logger->log(Stage::IF, "[IFMux] Passed Branch Address to Driver.");
    }
}