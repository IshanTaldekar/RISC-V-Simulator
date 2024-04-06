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

    this->is_reset_flag_set = false;

    this->ex_mem_stage_registers = nullptr;
    this->logger = nullptr;
    this->stage_synchronizer = nullptr;
}

void ALU::reset() {
    std::lock_guard<std::mutex> alu_lock (this->getModuleMutex());

    this->is_reset_flag_set = true;
    this->notifyModuleConditionVariable();
}

void ALU::resetState() {
    this->input1 = std::bitset<WORD_BIT_COUNT>(std::string(32, '0'));
    this->input2 = std::bitset<WORD_BIT_COUNT>(std::string(32, '0'));
    this->result = std::bitset<WORD_BIT_COUNT>(std::string(32, '0'));
    this->alu_op = std::bitset<ALU_OP_BIT_COUNT>(std::string(ALU_OP_BIT_COUNT, '0'));

    this->is_result_zero = false;
    this->is_input1_set = false;
    this->is_input2_set = false;
    this->is_alu_op_set = false;
}

void ALU::setInput1(std::bitset<WORD_BIT_COUNT> value) {
    this->log("setInput1 waiting to acquire lock.");

    std::lock_guard<std::mutex> alu_lock (this->getModuleMutex());

    this->log("setInput1 acquired lock. Updating value.");

    this->input1 = value;
    this->is_input1_set = true;

    this->log("setInput1 updated value.");
    this->notifyModuleConditionVariable();
}

void ALU::setInput2(std::bitset<WORD_BIT_COUNT> value) {
    this->log("setInput2 waiting to acquire lock.");

    std::lock_guard<std::mutex> alu_lock (this->getModuleMutex());

    this->log("setInput2 acquired lock. Updating value.");

    this->input2 = value;
    this->is_input2_set = true;

    this->log("setInput2 updated value.");
    this->notifyModuleConditionVariable();
}

void ALU::setALUOp(std::bitset<ALU_OP_BIT_COUNT> value) {
    std::lock_guard<std::mutex> alu_lock (this->getModuleMutex());

    this->log("setALUOp acquired lock. Updating value.");

    this->alu_op = value;
    this->is_alu_op_set = true;

    this->log("setALUOp updated value.");
    this->notifyModuleConditionVariable();
}

void ALU::run() {
    this->initDependencies();

    while (this->isAlive()) {
        this->log("Waiting to be woken up and acquire lock.");

        std::unique_lock<std::mutex> alu_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                alu_lock,
                [this] {
                    return (this->is_input1_set && this->is_input2_set && this->is_alu_op_set) ||
                        this->is_reset_flag_set || this->isKilled();
                }
        );

        if (this->is_reset_flag_set) {
            this->log("Resetting stage.");

            this->resetState();
            this->is_reset_flag_set = false;

            this->stage_synchronizer->arriveReset();

            this->log("Reset.");
            continue;
        }

        if (this->isKilled()) {
            this->log("Killed.");
            break;
        }

        this->log("Woken up and acquired lock.");

        this->computeResult();

        std::thread pass_result_thread(
                &ALU::passResultToEXMEMStageRegisters,
                this,
                this->result
        );

        std::thread pass_zero_flag_thread (
                &ALU::passZeroFlagToEXMEMStageRegisters,
                this,
                this->is_result_zero
        );

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
    std::unique_lock<std::mutex> alu_lock (this->getModuleDependencyMutex());

    if (this->ex_mem_stage_registers && this->logger) {
        return;
    }

    this->ex_mem_stage_registers = EXMEMStageRegisters::init();
    this->logger = Logger::init();
    this->stage_synchronizer = StageSynchronizer::init();
}

void ALU::computeResult() {
    this->log("Computing result.");

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
    this->log("Result computed.");
}

void ALU::passZeroFlagToEXMEMStageRegisters(bool is_flag_asserted) {
    this->log("Passing zero flag to EXMEMStageRegisters.");
    this->ex_mem_stage_registers->setIsResultZeroFlag(is_flag_asserted);
    this->log("Passed zero flag to EXMEMStageRegisters.");
}

void ALU::passResultToEXMEMStageRegisters(std::bitset<WORD_BIT_COUNT> data) {
    this->log("Passing result to EXMEMStageRegisters.");
    this->ex_mem_stage_registers->setALUResult(data);
    this->log("Passed result to EXMEMStageRegisters.");
}

std::string ALU::getModuleTag() {
    return "ALU";
}

Stage ALU::getModuleStage() {
    return Stage::EX;
}