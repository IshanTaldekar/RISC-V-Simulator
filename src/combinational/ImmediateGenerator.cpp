#include "../../include/combinational/ImmediateGenerator.h"

ImmediateGenerator::ImmediateGenerator() {
    this->is_instruction_set = false;
    this->instruction = nullptr;

    this->id_ex_stage_registers = IDEXStageRegisters::init();
}

ImmediateGenerator *ImmediateGenerator::init() {
    if (ImmediateGenerator::current_instance == nullptr) {
        ImmediateGenerator::current_instance = new ImmediateGenerator();
    }

    return ImmediateGenerator::current_instance;
}

void ImmediateGenerator::run() {
    while (this->isAlive()) {
        std::unique_lock<std::mutex> immediate_generator_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                immediate_generator_lock,
                [this] { return this->is_instruction_set; }
        );

        this->loadImmediateToIDEXStageRegisters();

        this->is_instruction_set = false;
    }
}

void ImmediateGenerator::setInstruction(const Instruction *current_instruction) {
    std::lock_guard immediate_generator_lock (this->getModuleMutex());

    this->instruction = current_instruction;
}

void ImmediateGenerator::loadImmediateToIDEXStageRegisters() {
    std::lock_guard immediate_generator_lock (this->getModuleMutex());
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
}

void ImmediateGenerator::notifyModuleConditionVariable() {
    this->getModuleConditionVariable().notify_one();
}