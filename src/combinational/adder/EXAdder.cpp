#include "../../../include/combinational/adder/EXAdder.h"

EXAdder *EXAdder::current_instance = nullptr;
std::mutex EXAdder::initialization_mutex;

EXAdder::EXAdder() {
    this->program_counter = 0UL;
    this->immediate = 0UL;
    this->result = 0UL;

    this->ex_mem_stage_registers = nullptr;
    this->logger = nullptr;

    this->is_program_counter_set = false;
    this->is_immediate_set = false;
}

EXAdder *EXAdder::init() {
    std::lock_guard<std::mutex> ex_adder_lock (EXAdder::initialization_mutex);

    if (EXAdder::current_instance == nullptr) {
        EXAdder::current_instance = new EXAdder();
    }

    return EXAdder::current_instance;
}

void EXAdder::initDependencies() {
    this->ex_mem_stage_registers = EXMEMStageRegisters::init();
    this->logger = Logger::init();
}

void EXAdder::run() {
    this->initDependencies();

    while (this->isAlive()) {
        this->logger->log(Stage::EX, "[EXAdder] Waiting to be woken up and acquire lock.");

        std::unique_lock<std::mutex> ex_adder_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                ex_adder_lock,
                [this] {
                    return (this->is_program_counter_set && this->is_immediate_set) || this->isKilled();
                }
        );

        if (this->isKilled()) {
            this->logger->log(Stage::EX, "[EXAdder] Killed.");
            break;
        }

        this->logger->log(Stage::EX, "[EXAdder] Woken up and acquired lock. Computing result.");

        this->computeResult();
        this->passBranchAddressToEXMEMStageRegisters();

        this->is_immediate_set = false;
        this->is_program_counter_set = false;
    }
}

void EXAdder::setInput(const AdderInputType &type, const AdderInputDataType &value) {
    if (!std::holds_alternative<EXAdderInputType>(type)) {
        throw std::runtime_error("EXAdder::setInput: incompatible data types passed");
    }

    this->logger->log(Stage::EX, "[EXAdder] setInput waiting to acquire lock.");

    std::lock_guard<std::mutex> ex_adder_lock (this->getModuleMutex());

    this->logger->log(Stage::EX, "[EXAdder] setInput acquired lock and updating value.");

    if (std::get<EXAdderInputType>(type) == EXAdderInputType::PCValue) {
        this->program_counter = std::get<unsigned long>(value);
        this->is_program_counter_set = true;
    } else if (std::get<EXAdderInputType>(type) == EXAdderInputType::ImmediateValue) {
        this->immediate = std::get<std::bitset<WORD_BIT_COUNT>>(value);
        this->is_immediate_set = true;
    }

    this->logger->log(Stage::EX, "[EXAdder] setInput value updated.");
    this->notifyModuleConditionVariable();
}

void EXAdder::computeResult() {
    this->result = BitwiseOperations::addInputs(
            std::bitset<WORD_BIT_COUNT>(this->program_counter),
            this->immediate
    ).to_ulong();

    this->logger->log(Stage::EX, "[EXAdder] Computed adder result.");
}

void EXAdder::passBranchAddressToEXMEMStageRegisters() {
    this->logger->log(Stage::EX, "[EXAdder] Passing adder result to EXMEMStageRegisters.");
    this->ex_mem_stage_registers->setBranchedProgramCounter(this->result);
    this->logger->log(Stage::EX, "[EXAdder] Passed adder result to EXMEMStageRegisters.");
}

