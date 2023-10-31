#include "../../include/state/IFIDStageRegisters.h"

IFIDStageRegisters::IFIDStageRegisters() = default;

IFIDStageRegisters *IFIDStageRegisters::init() {
    if (IFIDStageRegisters::current_instance == nullptr) {
        IFIDStageRegisters::current_instance = new IFIDStageRegisters();
    }

    return IFIDStageRegisters::current_instance;
}

void IFIDStageRegisters::run() {
    while (this->isAlive()) {

    }
}

void IFIDStageRegisters::setInput(std::variant<int, std::string> input) {
    std::unique_lock<std::mutex> if_id_stage_registers_lock (this->getMutex());

    if (std::holds_alternative<int>(input)) {
        this->program_counter = std::get<int>(input);
    } else if (std::holds_alternative<std::string>(input)) {
        this->instruction = std::get<std::string>(input);
    } else {
        std::cerr << "IFStageRegisters::setInput for IFIDStageRegisters passed an unsupported value" << std::endl;
    }
}

void IFIDStageRegisters::notifyConditionVariable() {
    this->getConditionVariable().notify_one();
}