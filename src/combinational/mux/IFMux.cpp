#include "../../../include/combinational/mux/IFMux.h"

IFMux *IFMux::current_instance = nullptr;
std::mutex IFMux::initialization_mutex;

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
    std::lock_guard<std::mutex> if_mux_lock (IFMux::initialization_mutex);

    if (IFMux::current_instance == nullptr) {
        IFMux::current_instance = new IFMux();
    }

    return IFMux::current_instance;
}

void IFMux::initDependencies() {
    std::unique_lock<std::mutex> mux_lock (this->getModuleDependencyMutex());

    if (this->driver && this->logger) {
        return;
    }

    this->driver = Driver::init();
    this->logger = Logger::init();
}

void IFMux::run() {
    this->initDependencies();

    while (this->isAlive()) {
        this->log("Waiting to be woken up and acquire lock.");

        std::unique_lock<std::mutex> mux_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                mux_lock,
                [this] {
                    return (this->is_branched_pc_set && this->is_incremented_pc_set && this->is_control_signal_set) ||
                    this->isKilled();
                }
        );

        if (this->isKilled()) {
            this->log("Killed.");
            break;
        }

        this->log("Woken up and acquired lock.");

        this->passOutput();

        this->is_pc_src_signal_asserted = false;
        this->is_branched_pc_set = false;
        this->is_incremented_pc_set = false;
        this->is_control_signal_set = false;
    }
}

void IFMux::setInput(MuxInputType type, MuxInputDataType value) {
    if (!std::holds_alternative<IFStageMuxInputType>(type)) {
        throw std::runtime_error("IFMux::setInput: incompatible data types passed.");
    }

    this->log("setInput waiting to acquire lock");

    std::unique_lock<std::mutex> mux_lock (this->getModuleMutex());

    this->log("setInput acquired lock. Updating values.");

    if (std::get<IFStageMuxInputType>(type) == IFStageMuxInputType::IncrementedPc) {
        this->incremented_pc = std::get<unsigned long>(value);
        this->is_incremented_pc_set = true;

        this->log("incremented PC value set");
    } else if (std::get<IFStageMuxInputType>(type) == IFStageMuxInputType::BranchedPc) {
        this->branched_pc = std::get<unsigned long>(value);
        this->is_branched_pc_set = true;

        this->log("branched PC value set");
    } else {
        throw std::runtime_error("IFMux::setInput did not match any input type.");
    }

    this->notifyModuleConditionVariable();
}

void IFMux::assertControlSignal(bool is_asserted) {
    this->log("assertControlSignal waiting to acquire lock.");

    std::lock_guard<std::mutex> if_mux_lock (this->getModuleMutex());

    this->is_pc_src_signal_asserted = is_asserted;
    this->is_control_signal_set = true;

    this->log("PCSrc asserted: " + std::to_string(this->is_pc_src_signal_asserted) + ".");
    this->notifyModuleConditionVariable();
}

/**
 * Loading output to the Driver.
 */
void IFMux::passOutput() {
    this->log("Passing output to Driver.");

    if (!this->is_pc_src_signal_asserted) {
        this->driver->setProgramCounter(this->incremented_pc);
        this->log("Passed PC to Driver.");
    } else {
        this->driver->setProgramCounter(this->branched_pc);
        this->log("Passed Branch Address to Driver.");
    }
}

std::string IFMux::getModuleTag() {
    return "IFMux";
}

Stage IFMux::getModuleStage() {
    return Stage::IF;
}