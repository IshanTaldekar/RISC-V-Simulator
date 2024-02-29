#include "../../../include/state/stage-registers/IFIDStageRegisters.h"

IFIDStageRegisters *IFIDStageRegisters::current_instance = nullptr;
std::mutex IFIDStageRegisters::initialization_mutex;

IFIDStageRegisters::IFIDStageRegisters() {
    this->program_counter = 0UL;
    this->instruction_bits = std::string(32, '0');

    this->is_program_counter_set = false;
    this->is_instruction_set = false;
    this->is_nop_asserted = false;
    this->is_reset_flag_set = false;
    this->is_pause_flag_set = false;

    this->instruction = nullptr;
    this->control = nullptr;

    this->register_file = nullptr;
    this->id_ex_stage_registers = nullptr;
    this->immediate_generator = nullptr;
    this->stage_synchronizer = nullptr;
    this->logger = nullptr;
}

void IFIDStageRegisters::changeStageAndReset(PipelineType new_stage) {
    {  // Limit lock guard scope to avoid deadlock
        std::lock_guard<std::mutex> if_id_stage_registers_lock (this->getModuleMutex());

        this->logger->log(Stage::IF, "[IFIDStageRegisters] PipelineType change.");
        this->setPipelineType(new_stage);
    }

    this->reset();
}

void IFIDStageRegisters::reset() {
    std::lock_guard<std::mutex> if_id_stage_registers_lock (this->getModuleMutex());

    this->logger->log(Stage::IF, "[IFIDStageRegisters] Reset flag set.");

    this->is_reset_flag_set = true;
    this->notifyModuleConditionVariable();
}

void IFIDStageRegisters::resetStage() {
    if (this->getPipelineType() == PipelineType::Single) {
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
    this->logger->log(Stage::IF, "[IFIDStageRegisters] Paused.");
    this->is_pause_flag_set = true;
}

void IFIDStageRegisters::resume() {
    this->logger->log(Stage::IF, "[IFIDStageRegisters] Resumed.");
    this->is_pause_flag_set = false;
    this->notifyModuleConditionVariable();
}

IFIDStageRegisters *IFIDStageRegisters::init() {
    std::lock_guard<std::mutex> if_id_stage_registers_lock (IFIDStageRegisters::initialization_mutex);

    if (IFIDStageRegisters::current_instance == nullptr) {
        IFIDStageRegisters::current_instance = new IFIDStageRegisters();
    }

    return IFIDStageRegisters::current_instance;
}

void IFIDStageRegisters::initDependencies() {
    this->instruction = new Instruction(std::string(32, '0'));
    this->control = new Control(this->instruction);

    this->register_file = RegisterFile::init();
    this->id_ex_stage_registers = IDEXStageRegisters::init();
    this->immediate_generator = ImmediateGenerator::init();
    this->stage_synchronizer = StageSynchronizer::init();
    this->logger = Logger::init();
}

void IFIDStageRegisters::run() {
    this->initDependencies();

    while (this->isAlive()) {
        this->logger->log(Stage::IF, "[IFIDStageRegisters] Waiting to be woken up and acquire lock.");

        std::unique_lock<std::mutex> if_id_stage_registers_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                if_id_stage_registers_lock,
                [this] {
                    return (this->is_instruction_set && this->is_program_counter_set && !this->is_pause_flag_set) ||
                            this->is_reset_flag_set || this->isKilled();
                }
        );

        if (this->isKilled()) {
            this->logger->log(Stage::IF, "[IFIDStageRegisters] Killed.");
            break;
        }

        if (this->is_reset_flag_set) {
            this->logger->log(Stage::IF, "[IFIDStageRegisters] Resetting stage.");

            this->resetStage();
            this->is_reset_flag_set = false;

            this->logger->log(Stage::IF, "[IFIDStageRegisters] Resetting stage.");
            continue;
        }

        this->logger->log(Stage::IF, "[IFIDStageRegisters] Woken up and acquired lock.");

        this->instruction = new Instruction(this->instruction_bits);
        this->control = new Control(this->instruction);

        // Popup threads to pass data because barrier will have them sleep until synchronization condition is met
        std::thread pass_control_thread (&IFIDStageRegisters::passControlToIDEXStageRegisters, this);
        std::thread pass_program_counter_thread (&IFIDStageRegisters::passProgramCounterToIDEXStageRegisters, this);
        std::thread pass_read_registers_thread (&IFIDStageRegisters::passReadRegistersToRegisterFile, this);
        std::thread pass_instruction_immediate_generator_thread (&IFIDStageRegisters::passInstructionToImmediateGenerator, this);
        std::thread pass_register_destination_thread (&IFIDStageRegisters::passRegisterDestinationToIDEXStageRegisters, this);
        std::thread pass_register_source1_thread (&IFIDStageRegisters::passRegisterSource1ToIDEXStageRegisters, this);
        std::thread pass_register_source2_thread (&IFIDStageRegisters::passRegisterSource2ToIDEXStageRegisters, this);
        std::thread pass_instruction_if_id_stage_registers_thread (&IFIDStageRegisters::passInstructionToIDEXStageRegisters, this);

        // Join to avoid the values being overridden before the threads have a chance to execute.
        pass_control_thread.join();
        pass_program_counter_thread.join();
        pass_read_registers_thread.join();
        pass_instruction_immediate_generator_thread.join();
        pass_register_destination_thread.join();
        pass_register_source1_thread.join();
        pass_register_source2_thread.join();
        pass_instruction_if_id_stage_registers_thread.join();

        this->is_instruction_set = false;
        this->is_program_counter_set = false;
        this->is_nop_asserted = false;

        this->stage_synchronizer->conditionalArriveSingleStage();
    }
}

void IFIDStageRegisters::setInput(const std::variant<unsigned long, std::string> &input) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    this->logger->log(Stage::IF, "[IFIDStageRegisters] setInput waiting to acquire lock.");

    std::unique_lock<std::mutex> if_id_stage_registers_lock (this->getModuleMutex());

    this->logger->log(Stage::IF, "[IFIDStageRegisters] setInput acquired lock. Updating value.");

    if (std::holds_alternative<unsigned long>(input)) {
        if (!this->is_nop_asserted) {
            this->program_counter = std::get<unsigned long>(input);
            this->logger->log(Stage::IF, "[IFIDStageRegisters] setInput updated value.");
        } else {
            this->logger->log(Stage::IF, "[IFIDStageRegisters] setInput update skipped. NOP asserted.");
        }

        this->is_program_counter_set = true;
    } else if (std::holds_alternative<std::string>(input)) {
        if (!this->is_nop_asserted) {
            this->instruction_bits = std::get<std::string>(input);
            this->logger->log(Stage::IF, "[IFIDStageRegisters] setInput updated value.");
        } else {
            this->logger->log(Stage::IF, "[IFIDStageRegisters] setInput update skipped. NOP asserted.");
        }

        this->is_instruction_set = true;
    } else {
        throw std::runtime_error("IFStageRegisters::setInput for IFIDStageRegisters passed an unsupported value");
    }

    this->notifyModuleConditionVariable();
}

void IFIDStageRegisters::passProgramCounterToIDEXStageRegisters() {
    this->logger->log(Stage::IF, "[IFIDStageRegisters] Passing program counter to IDEXStageRegisters.");
    this->id_ex_stage_registers->setProgramCounter(this->program_counter);
    this->logger->log(Stage::IF, "[IFIDStageRegisters] Passed program counter to IDEXStageRegisters.");
}

void IFIDStageRegisters::passControlToIDEXStageRegisters() {
    this->logger->log(Stage::IF, "[IFIDStageRegisters] Passing control to IDEXStageRegisters.");
    this->id_ex_stage_registers->setControlModule(this->control);
    this->logger->log(Stage::IF, "[IFIDStageRegisters] Passed control to IDEXStageRegisters.");
}

void IFIDStageRegisters::passReadRegistersToRegisterFile() {
    this->logger->log(Stage::IF, "[IFIDStageRegisters] Passing read registers to RegisterFile.");

    InstructionType type = this->instruction->getType();

    if (type == InstructionType::I || type == InstructionType::S) {
        this->register_file->setReadRegister(this->instruction->getRs1().to_ulong());
    } else {
        this->register_file->setReadRegisters(this->instruction->getRs1().to_ulong(), this->instruction->getRs2().to_ulong());
    }

    this->logger->log(Stage::IF, "[IFIDStageRegisters] Passed read registers to RegisterFile.");
}

void IFIDStageRegisters::passInstructionToImmediateGenerator() {
    this->logger->log(Stage::IF, "[IFIDStageRegisters] Passing instruction to ImmediateGenerator.");
    this->immediate_generator->setInstruction(this->instruction);
    this->logger->log(Stage::IF, "[IFIDStageRegisters] Passed instruction to ImmediateGenerator.");
}

void IFIDStageRegisters::passRegisterDestinationToIDEXStageRegisters() {
    this->logger->log(Stage::IF, "[IFIDStageRegisters] Passing register destination to IDEXStageRegisters.");
    this->id_ex_stage_registers->setRegisterDestination(this->instruction->getRd().to_ulong());
    this->logger->log(Stage::IF, "[IFIDStageRegisters] Passed register destination to IDEXStageRegisters.");
}

void IFIDStageRegisters::assertNop() {
    this->logger->log(Stage::IF, "[IFIDStageRegisters] NOP asserted.");
    this->is_nop_asserted = true;
}

void IFIDStageRegisters::passRegisterSource1ToIDEXStageRegisters() {
    this->logger->log(Stage::IF, "[IFIDStageRegisters] Passing register source 1 to IDEXStageRegisters.");
    this->id_ex_stage_registers->setRegisterSource1(this->instruction->getRs1().to_ulong());
    this->logger->log(Stage::IF, "[IFIDStageRegisters] Passed register source 1 to IDEXStageRegisters.");
}

void IFIDStageRegisters::passRegisterSource2ToIDEXStageRegisters() {
    this->logger->log(Stage::IF, "[IFIDStageRegisters] Passing register source 2 to IDEXStageRegisters.");
    this->id_ex_stage_registers->setRegisterSource2(this->instruction->getRs2().to_ulong());
    this->logger->log(Stage::IF, "[IFIDStageRegisters] Passed register source 2 to IDEXStageRegisters.");
}

void IFIDStageRegisters::passInstructionToIDEXStageRegisters() {
    this->logger->log(Stage::IF, "[IFIDStageRegisters] Passing instruction to IDEXStageRegisters.");
    this->id_ex_stage_registers->setInstruction(this->instruction);
    this->logger->log(Stage::IF, "[IFIDStageRegisters] Passed instruction to IDEXStageRegisters.");
}