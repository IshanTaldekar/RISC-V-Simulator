#include "../../include/combinational/ALU.h"

ALU *ALU::current_instance = nullptr;
std::mutex ALU::initialization_mutex;

ALU::ALU() {
    this->input1 = std::bitset<WORD_BIT_COUNT>(std::string(32, '0'));
    this->input2 = std::bitset<WORD_BIT_COUNT>(std::string(32, '0'));
    this->result = std::bitset<WORD_BIT_COUNT>(std::string(32, '0'));

    this->is_result_zero = false;

    this->is_input1_set = false;
    this->is_input2_set = false;
    this->is_alu_op_set = false;

    this->ex_mem_stage_registers = nullptr;
    this->logger = nullptr;
}

void ALU::setInput1(const std::bitset<WORD_BIT_COUNT> &value) {
    this->logger->log(Stage::EX, "[ALU] setInput1 waiting to acquire lock.");

    std::lock_guard<std::mutex> alu_lock (this->getModuleMutex());

    this->logger->log(Stage::EX, "[ALU] setInput1 acquired lock. Updating value.");

    this->input1 = value;
    this->is_input1_set = true;

    this->logger->log(Stage::EX, "[ALU] setInput1 updated value.");
    this->notifyModuleConditionVariable();
}

void ALU::setInput2(const std::bitset<WORD_BIT_COUNT> &value) {
    this->logger->log(Stage::EX, "[ALU] setInput2 waiting to acquire lock.");

    std::lock_guard<std::mutex> alu_lock (this->getModuleMutex());

    this->logger->log(Stage::EX, "[ALU] setInput2 acquired lock. Updating value.");

    this->input2 = value;
    this->is_input2_set = true;

    this->logger->log(Stage::EX, "[ALU] setInput2 updated value.");
    this->notifyModuleConditionVariable();
}

void ALU::setALUOp(const std::bitset<ALU_OP_BIT_COUNT> &value) {
    this->logger->log(Stage::EX, "[ALU] setALUOp waiting to acquire lock.");

    std::lock_guard<std::mutex> alu_lock (this->getModuleMutex());

    this->logger->log(Stage::EX, "[ALU] setALUOp acquired lock. Updating value.");

    this->alu_op = value;
    this->is_alu_op_set = true;

    this->logger->log(Stage::EX, "[ALU] setALUOp updated value.");
    this->notifyModuleConditionVariable();
}

void ALU::run() {
    this->initDependencies();

    while (this->isAlive()) {
        this->logger->log(Stage::EX, "[ALU] Waiting to be woken up and acquire lock.");

        std::unique_lock<std::mutex> alu_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                alu_lock,
                [this] {
                    return (this->is_input1_set && this->is_input2_set && this->is_alu_op_set) || this->isKilled();
                }
        );

        if (this->isKilled()) {
            this->logger->log(Stage::EX, "[ALU] Killed.");
            break;
        }

        this->logger->log(Stage::EX, "[ALU] Woken up and acquired lock.");

        this->computeResult();

        std::thread pass_result_thread(&ALU::passResultToEXMEMStageRegisters, this);
        std::thread pass_zero_flag_thread (&ALU::passZeroFlagToEXMEMStageRegisters, this);

        pass_result_thread.join();
        pass_zero_flag_thread.join();

        this->is_alu_op_set = false;
        this->is_input1_set = false;
        this->is_input2_set = false;
        this->is_result_zero = false;
    }
}

ALU *ALU::init() {
    std::lock_guard<std::mutex> alu_lock (ALU::initialization_mutex);

    if (ALU::current_instance == nullptr) {
        ALU::current_instance = new ALU();
    }

    return ALU::current_instance;
}

void ALU::initDependencies() {
    this->ex_mem_stage_registers = EXMEMStageRegisters::init();
    this->logger = Logger::init();
}

void ALU::computeResult() {
    this->logger->log(Stage::EX, "[ALU] Computing result.");

    if (this->alu_op == std::bitset<ALU_OP_BIT_COUNT>("0000")) {  // Add
        this->result = BitwiseOperations::addInputs(this->input1, this->input2);
    } else if (this->alu_op == std::bitset<ALU_OP_BIT_COUNT>("0001")) {  // subtract
        this->result = BitwiseOperations::subtractInputs(this->input1, this->input2);
    } else if (this->alu_op == std::bitset<ALU_OP_BIT_COUNT>("0010")) {  // Xor
        this->result = BitwiseOperations::bitwiseXorInputs(this->input1, this->input2);
    } else if (this->alu_op == std::bitset<ALU_OP_BIT_COUNT>("0011")) {  // Or
        this->result = BitwiseOperations::bitwiseOrInputs(this->input1, this->input2);
    } else if (this->alu_op == std::bitset<ALU_OP_BIT_COUNT>("0100")) {  // And
        this->result = BitwiseOperations::bitwiseAndInputs(this->input1, this->input2);
    }

    this->is_result_zero = this->result.to_ulong() == 0;

    this->logger->log(Stage::EX, "[ALU] Result computed.");
}

void ALU::passZeroFlagToEXMEMStageRegisters() {
    this->logger->log(Stage::EX, "[ALU] Passing zero flag to EXMEMStageRegisters.");
    this->ex_mem_stage_registers->setIsResultZeroFlag(this->is_result_zero);
    this->logger->log(Stage::EX, "[ALU] Passed zero flag to EXMEMStageRegisters.");
}

void ALU::passResultToEXMEMStageRegisters() {
    this->logger->log(Stage::EX, "[ALU] Passing result to EXMEMStageRegisters.");
    this->ex_mem_stage_registers->setALUResult(this->result);
    this->logger->log(Stage::EX, "[ALU] Passed result to EXMEMStageRegisters.");
}