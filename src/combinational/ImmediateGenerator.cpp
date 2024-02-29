#include "../../include/combinational/ImmediateGenerator.h"

ImmediateGenerator *ImmediateGenerator::current_instance = nullptr;
std::mutex ImmediateGenerator::initialization_mutex;

ImmediateGenerator::ImmediateGenerator() {
    this->is_instruction_set = false;
    this->instruction = new Instruction(std::string(32, '0'));

    this->id_ex_stage_registers = nullptr;
    this->logger = nullptr;
}

ImmediateGenerator *ImmediateGenerator::init() {
    std::lock_guard<std::mutex> immediate_generator_lock (ImmediateGenerator::initialization_mutex);

    if (ImmediateGenerator::current_instance == nullptr) {
        ImmediateGenerator::current_instance = new ImmediateGenerator();
    }

    return ImmediateGenerator::current_instance;
}

void ImmediateGenerator::initDependencies() {
    this->id_ex_stage_registers = IDEXStageRegisters::init();
    this->logger = Logger::init();
}

void ImmediateGenerator::run() {
    this->initDependencies();

    while (this->isAlive()) {
        this->logger->log(Stage::ID, "[ImmediateGenerator] Waiting to be woken up and acquire lock.");

        std::unique_lock<std::mutex> immediate_generator_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                immediate_generator_lock,
                [this] { return this->is_instruction_set || this->isKilled(); }
        );

        if (this->isKilled()) {
            this->logger->log(Stage::ID, "[ImmediateGenerator] Killed.");
            break;
        }

        this->logger->log(Stage::ID, "[ImmediateGenerator] Woken up and acquired lock.");

        this->loadImmediateToIDEXStageRegisters();

        this->is_instruction_set = false;
    }
}

void ImmediateGenerator::setInstruction(const Instruction *current_instruction) {
    this->logger->log(Stage::ID, "[ImmediateGenerator] setInstruction waiting to acquire lock.");

    std::lock_guard immediate_generator_lock (this->getModuleMutex());

    this->logger->log(Stage::ID, "[ImmediateGenerator] setInstruction acquired lock. Updating value.");

    this->instruction = current_instruction;
    this->is_instruction_set = true;

    this->logger->log(Stage::ID, "[ImmediateGenerator] setInstruction value updated.");
    this->notifyModuleConditionVariable();
}

void ImmediateGenerator::loadImmediateToIDEXStageRegisters() {
    this->logger->log(Stage::ID, "[ImmediateGenerator] Passing immediate to IDEXStageRegisters.");

    InstructionType type = this->instruction->getType();

    std::bitset<WORD_BIT_COUNT> result;

    if (type == InstructionType::B || type == InstructionType::S || type == InstructionType::I) {
        std::bitset<Instruction::GENERAL_IMMEDIATE_BIT_COUNT> current_immediate = this->instruction->getImmediate();

        if (current_immediate[Instruction::GENERAL_IMMEDIATE_BIT_COUNT - 1]) {
            result.set();

            for (int i = 0; i < Instruction::GENERAL_IMMEDIATE_BIT_COUNT; ++i) {
                result[i] = current_immediate[i];
            }
        }
    } else if (type == InstructionType::J) {
        std::bitset<Instruction::J_TYPE_IMMEDIATE_BIT_COUNT> current_immediate = this->instruction->getImmediateJ();

        if (current_immediate[Instruction::J_TYPE_IMMEDIATE_BIT_COUNT - 1]) {
            result.set();

            for (int i = 0; i < Instruction::J_TYPE_IMMEDIATE_BIT_COUNT; ++i) {
                result[i] = current_immediate[i];
            }
        }
    }

    this->id_ex_stage_registers->setImmediate(result);
    this->logger->log(Stage::ID, "[ImmediateGenerator] Passed immediate to IDEXStageRegisters.");
}