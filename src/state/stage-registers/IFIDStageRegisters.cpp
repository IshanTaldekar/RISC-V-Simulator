#include "../../../include/state/stage-registers/IFIDStageRegisters.h"

IFIDStageRegisters *IFIDStageRegisters::current_instance = nullptr;

IFIDStageRegisters::IFIDStageRegisters() {
    this->program_counter = 0UL;
    this->instruction_bits = std::string(32, '0');

    this->is_program_counter_set = false;
    this->is_instruction_set = false;
    this->is_nop_asserted = false;
    this->is_reset_flag_set = false;
    this->is_pause_flag_set = false;

    this->if_logger = IFLogger::init();
    this->id_logger = IDLogger::init();

    this->instruction = new Instruction(std::string(32, '0'));
    this->control = new Control(this->instruction);

    this->register_file = RegisterFile::init();
    this->id_ex_stage_registers = IDEXStageRegisters::init();
    this->immediate_generator = ImmediateGenerator::init();
    this->stage_synchronizer = StageSynchronizer::init();
}

void IFIDStageRegisters::changeStageAndReset(Stage new_stage) {
    {  // Limit lock guard scope to avoid deadlock
        std::lock_guard<std::mutex> if_id_stage_registers_lock(this->getModuleMutex());

        this->log("[IFIDStageRegisters] Stage change.");
        this->setStage(new_stage);
    }

    this->reset();
}

void IFIDStageRegisters::reset() {
    std::lock_guard<std::mutex> if_id_stage_registers_lock (this->getModuleMutex());

    this->log("[IFIDStageRegisters] Reset flag set.");

    this->is_reset_flag_set = true;
    this->notifyModuleConditionVariable();
}

void IFIDStageRegisters::resetStage() {
    this->log("[IFIDStageRegisters] Reset stage.");

    if (this->getStage() == Stage::Single) {
        this->is_program_counter_set = false;
        this->is_instruction_set = false;
    } else {
        this->is_program_counter_set = true;
        this->is_instruction_set = true;
    }

    this->is_nop_asserted = false;
    this->is_reset_flag_set = false;
    this->is_pause_flag_set = false;

    this->program_counter = 0UL;
    this->instruction = new Instruction(std::string(32, '0'));
    this->control = new Control(this->instruction);
}

void IFIDStageRegisters::pause() {
    this->log("[IFIDStageRegisters] Pausing execution.");

    std::lock_guard<std::mutex> if_id_stage_registers_lock (this->getModuleMutex());

    this->is_pause_flag_set = true;
    this->notifyModuleConditionVariable();
}

void IFIDStageRegisters::resume() {
    this->log("[IFIDStageRegisters] Resume execution.");

    std::lock_guard<std::mutex> if_id_stage_registers_lock (this->getModuleMutex());

    this->is_pause_flag_set = false;
    this->notifyModuleConditionVariable();
}

IFIDStageRegisters *IFIDStageRegisters::init() {
    if (IFIDStageRegisters::current_instance == nullptr) {
        IFIDStageRegisters::current_instance = new IFIDStageRegisters();
    }

    return IFIDStageRegisters::current_instance;
}

void IFIDStageRegisters::run() {
    while (this->isAlive()) {
        this->log("[IFIDStageRegisters] Waiting to be woken up and acquire lock.");

        std::unique_lock<std::mutex> if_id_stage_registers_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                if_id_stage_registers_lock,
                [this] {
                    return (this->is_instruction_set && this->is_program_counter_set) ||
                            this->is_reset_flag_set || !this->is_pause_flag_set;
                }
        );

        if (this->isKilled()) {
            break;
        }

        if (this->is_pause_flag_set) {
            continue;
        }

        if (this->is_reset_flag_set) {
            this->resetStage();
            this->is_reset_flag_set = false;
            continue;
        }

        this->log("[IFIDStageRegisters] Woken up and acquire lock. Passing values.");

        this->instruction = new Instruction(this->instruction_bits);
        this->control = new Control(this->instruction);

        std::thread pass_control_thread (&IFIDStageRegisters::passControlToIDEXStageRegisters, this);
        std::thread pass_program_counter_thread (&IFIDStageRegisters::passProgramCounterToIDEXStageRegisters, this);
        std::thread pass_read_registers_thread (&IFIDStageRegisters::passReadRegistersToRegisterFile, this);
        std::thread pass_instruction_thread (&IFIDStageRegisters::passInstructionToImmediateGenerator, this);
        std::thread pass_register_destination_thread (&IFIDStageRegisters::passRegisterDestinationToIDEXStageRegisters, this);

        // Join to avoid the values being overridden before the threads have a chance to execute.
        pass_control_thread.join();
        pass_program_counter_thread.join();
        pass_read_registers_thread.join();
        pass_instruction_thread.join();
        pass_register_destination_thread.join();

        this->is_instruction_set = false;
        this->is_program_counter_set = false;
        this->is_nop_asserted = false;

        this->stage_synchronizer->conditionalArriveSingleStage();
    }
}

void IFIDStageRegisters::setInput(const std::variant<unsigned long, std::string> &input) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    std::unique_lock<std::mutex> if_id_stage_registers_lock (this->getModuleMutex());

    if (this->is_nop_asserted) {
        this->log("[IFIDStageRegisters] Nop asserted new values dropped.");
    }

    if (std::holds_alternative<unsigned long>(input)) {
        if (!this->is_nop_asserted) {
            this->program_counter = std::get<unsigned long>(input);
            this->log("[IFIDStageRegisters] Program counter set.");
        }

        this->is_program_counter_set = true;
    } else if (std::holds_alternative<std::string>(input)) {
        if (this->is_nop_asserted) {
            this->instruction_bits = std::string(32, '0');
        } else {
            this->instruction_bits = std::get<std::string>(input);
            this->log("[IFIDStageRegisters] Instruction set.");
        }

        this->is_instruction_set = true;
    } else {
        throw std::runtime_error("IFStageRegisters::setInput for IFIDStageRegisters passed an unsupported value");
    }

    this->notifyModuleConditionVariable();
}

void IFIDStageRegisters::notifyModuleConditionVariable() {
    this->getModuleConditionVariable().notify_one();
}

void IFIDStageRegisters::passProgramCounterToIDEXStageRegisters() {
    this->id_ex_stage_registers->setProgramCounter(this->program_counter);
}

void IFIDStageRegisters::passControlToIDEXStageRegisters() {
    this->id_ex_stage_registers->setControlModule(this->control);
}

void IFIDStageRegisters::passReadRegistersToRegisterFile() {
    InstructionType type = this->instruction->getType();

    if (type == InstructionType::I || type == InstructionType::S) {
        this->register_file->setReadRegister(this->instruction->getRs1().to_ulong());
    } else {
        this->register_file->setReadRegisters(this->instruction->getRs1().to_ulong(), this->instruction->getRs2().to_ulong());
    }
}

void IFIDStageRegisters::passInstructionToImmediateGenerator() {
    this->immediate_generator->setInstruction(this->instruction);
}

void IFIDStageRegisters::passRegisterDestinationToIDEXStageRegisters() {
    this->id_ex_stage_registers->setRegisterDestination(this->instruction->getRd().to_ulong());
}

void IFIDStageRegisters::assertNop() {
    std::lock_guard<std::mutex> if_id_stage_registers_lock (this->getModuleMutex());
    this->is_nop_asserted = true;
}

void IFIDStageRegisters::log(const std::string &message) {
    this->id_logger->log(message);
    this->if_logger->log(message);
}