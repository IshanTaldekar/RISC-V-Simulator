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
    this->is_nop_flag_set = true;
    this->is_nop_passed_flag_asserted = false;
    this->is_nop_passed_flag_set = false;

    this->current_nop_set_operations = 0;

    this->instruction = nullptr;
    this->control = nullptr;

    this->register_file = nullptr;
    this->hazard_detection_unit = nullptr;
    this->id_ex_stage_registers = nullptr;
    this->immediate_generator = nullptr;
    this->stage_synchronizer = nullptr;
    this->logger = nullptr;
}

void IFIDStageRegisters::changeStageAndReset(PipelineType new_stage) {
    {  // Limit lock guard scope to avoid deadlock
        std::lock_guard<std::mutex> if_id_stage_registers_lock (this->getModuleMutex());

        this->log("PipelineType change.");
        this->setPipelineType(new_stage);
    }

    this->reset();
}

void IFIDStageRegisters::reset() {
    if (!this->logger) {
        this->initDependencies();
    }

    std::lock_guard<std::mutex> if_id_stage_registers_lock (this->getModuleMutex());

    this->log("Reset flag set.");

    this->is_reset_flag_set = true;
    this->notifyModuleConditionVariable();
}

void IFIDStageRegisters::resetStage() {
    if (this->getPipelineType() == PipelineType::Single) {
        this->is_program_counter_set = false;
        this->is_instruction_set = false;
        this->is_nop_flag_set = true;
        this->is_nop_asserted = false;
        this->is_nop_passed_flag_set = false;
    } else {
        this->is_program_counter_set = true;
        this->is_instruction_set = true;
        this->is_nop_flag_set = true;
        this->is_nop_asserted = true;
        this->is_nop_passed_flag_set = true;
    }

    this->is_reset_flag_set = false;
    this->is_pause_flag_set = false;

    this->program_counter = 0UL;
    this->instruction_bits = std::string(32, '0');
    this->instruction = new Instruction(this->instruction_bits);
    this->control = new Control(this->instruction);
}

void IFIDStageRegisters::pause() {
    this->log("Paused.");
    this->is_pause_flag_set = true;
}

void IFIDStageRegisters::resume() {
    this->log("Resumed.");
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
    std::unique_lock<std::mutex> if_id_stage_registers_lock (this->getModuleMutex());

    if (this->instruction && this->control && this->register_file && this->id_ex_stage_registers &&
        this->immediate_generator && this->stage_synchronizer && this->logger && this->hazard_detection_unit) {
        return;
    }

    this->instruction = new Instruction(std::string(32, '0'));
    this->control = new Control(this->instruction);

    this->register_file = RegisterFile::init();
    this->id_ex_stage_registers = IDEXStageRegisters::init();
    this->immediate_generator = ImmediateGenerator::init();
    this->stage_synchronizer = StageSynchronizer::init();
    this->logger = Logger::init();
    this->hazard_detection_unit = HazardDetectionUnit::init();
}

void IFIDStageRegisters::run() {
    this->initDependencies();

    while (this->isAlive()) {
        this->log("Waiting to be woken up and acquire lock.");

        std::unique_lock<std::mutex> if_id_stage_registers_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                if_id_stage_registers_lock,
                [this] {
                    return (this->is_instruction_set && this->is_program_counter_set && this->is_nop_flag_set
                            && this->is_nop_passed_flag_set && !this->is_pause_flag_set) || this->is_reset_flag_set ||
                            this->isKilled();
                }
        );

        if (this->isKilled()) {
            this->log("Killed.");
            break;
        }

        if (this->is_reset_flag_set) {
            this->log("Resetting stage.");

            this->resetStage();
            this->is_reset_flag_set = false;

            this->log("Resetting stage.");
            continue;
        }

        this->log("Woken up and acquired lock.");

        this->instruction = new Instruction(this->instruction_bits);
        this->control = new Control(this->instruction);
        this->control->setNop(this->is_nop_passed_flag_asserted ||
                                this->instruction->getType() == InstructionType::HALT);

        // Popup threads to pass data because barrier will have them sleep until synchronization condition is met
        std::thread pass_instruction_hazard_detection_unit_thread (
                &IFIDStageRegisters::passInstructionToHazardDetectionUnit,
                this,
                Instruction::deepCopy(this->instruction)
        );

        std::thread pass_instruction_immediate_generator_thread (
                &IFIDStageRegisters::passInstructionToImmediateGenerator,
                this,
                Instruction::deepCopy(this->instruction)
        );

        std::thread pass_control_thread (
                &IFIDStageRegisters::passControlToIDEXStageRegisters,
                this,
                Control::deepCopy(this->control)
        );

        std::thread pass_program_counter_thread (
                &IFIDStageRegisters::passProgramCounterToIDEXStageRegisters,
                this,
                this->program_counter
        );

        std::thread pass_read_registers_thread (
                &IFIDStageRegisters::passReadRegistersToRegisterFile,
                this,
                Instruction::deepCopy(this->instruction)
        );

        std::thread pass_register_destination_thread (
                &IFIDStageRegisters::passRegisterDestinationToIDEXStageRegisters,
                this,
                Instruction::deepCopy(this->instruction)
        );

        std::thread pass_register_source1_thread (
                &IFIDStageRegisters::passRegisterSource1ToIDEXStageRegisters,
                this,
                Instruction::deepCopy(this->instruction)
        );

        std::thread pass_register_source2_thread (
                &IFIDStageRegisters::passRegisterSource2ToIDEXStageRegisters,
                this,
                Instruction::deepCopy(this->instruction)
        );

        std::thread pass_instruction_id_ex_stage_registers_thread (
                &IFIDStageRegisters::passInstructionToIDEXStageRegisters,
                this,
                Instruction::deepCopy(this->instruction)
        );

        std::thread pass_nop_id_ex_stage_registers_thread (
                &IFIDStageRegisters::passNopToIDEXStageRegisters,
                this,
                this->is_nop_passed_flag_asserted
        );

        pass_instruction_hazard_detection_unit_thread.join();
        pass_instruction_immediate_generator_thread.join();
        pass_control_thread.join();
        pass_program_counter_thread.join();
        pass_read_registers_thread.join();
        pass_register_destination_thread.join();
        pass_register_source1_thread.join();
        pass_register_source2_thread.join();
        pass_instruction_id_ex_stage_registers_thread.join();
        pass_nop_id_ex_stage_registers_thread.join();

        this->is_instruction_set = false;
        this->is_program_counter_set = false;
        this->is_nop_asserted = false;
        this->is_nop_flag_set = false;
        this->is_nop_passed_flag_asserted = false;
        this->is_nop_passed_flag_set = false;

        this->current_nop_set_operations = 0;

        this->stage_synchronizer->conditionalArriveSingleStage();
    }
}

void IFIDStageRegisters::setInput(std::variant<unsigned long, std::string> input) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    if (this->getPipelineType() == PipelineType::Five) {
        this->delayUpdateUntilNopFlagSet();
    }

    this->log("setInput waiting to acquire lock.");

    std::unique_lock<std::mutex> if_id_stage_registers_lock (this->getModuleMutex());

    this->log("setInput acquired lock. Updating value.");

    if (std::holds_alternative<unsigned long>(input)) {
        if (!this->is_nop_asserted) {
            this->program_counter = std::get<unsigned long>(input);
            this->log("setInput updated value.");
        } else {
            this->log("setInput update skipped. NOP asserted.");
        }

        this->is_program_counter_set = true;
    } else if (std::holds_alternative<std::string>(input)) {
        if (!this->is_nop_asserted) {
            this->instruction_bits = std::string(std::get<std::string>(input));
            this->log("setInput updated value.");
        } else {
            this->log("setInput update skipped. NOP asserted.");
        }

        this->is_instruction_set = true;
    } else {
        throw std::runtime_error("IFStageRegisters::setInput for IFIDStageRegisters passed an unsupported value");
    }

    this->notifyModuleConditionVariable();
}


void IFIDStageRegisters::setNop(bool is_asserted) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    this->log("setPassedNop waiting to acquire lock.");

    std::lock_guard<std::mutex> if_id_stage_registers_lock (this->getModuleMutex());

    this->log("setPassedNop acquired lock.");

    this->is_nop_asserted |= is_asserted;

    if (++this->current_nop_set_operations == REQUIRED_NOP_FLAG_SET_OPERATIONS) {
        this->is_nop_flag_set = true;
        this->getModuleConditionVariable().notify_all();
    }
}

void IFIDStageRegisters::passProgramCounterToIDEXStageRegisters(unsigned long pc) {
    this->log("Passing program counter to IDEXStageRegisters.");
    this->id_ex_stage_registers->setProgramCounter(pc);
    this->log("Passed program counter to IDEXStageRegisters.");
}

void IFIDStageRegisters::passControlToIDEXStageRegisters(Control *new_control) {
    this->log("Passing control to IDEXStageRegisters.");
    this->id_ex_stage_registers->setControlModule(new_control);
    this->log("Passed control to IDEXStageRegisters.");
}

void IFIDStageRegisters::passReadRegistersToRegisterFile(Instruction *current_instruction) {
    this->log("Passing read registers to RegisterFile.");

    InstructionType type = current_instruction->getType();

    if (type == InstructionType::I || type == InstructionType::S) {
        this->register_file->setReadRegister(current_instruction->getRs1().to_ulong());
    } else {
        this->register_file->setReadRegisters(current_instruction->getRs1().to_ulong(), this->instruction->getRs2().to_ulong());
    }

    this->log("Passed read registers to RegisterFile.");
}

void IFIDStageRegisters::passInstructionToImmediateGenerator(Instruction *current_instruction) {
    this->log("Passing instruction to ImmediateGenerator.");
    this->immediate_generator->setInstruction(current_instruction);
    this->log("Passed instruction to ImmediateGenerator.");
}

void IFIDStageRegisters::passRegisterDestinationToIDEXStageRegisters(Instruction *current_instruction) {
    this->log("Passing register destination to IDEXStageRegisters.");
    this->id_ex_stage_registers->setRegisterDestination(current_instruction->getRd().to_ulong());
    this->log("Passed register destination to IDEXStageRegisters.");
}

void IFIDStageRegisters::passRegisterSource1ToIDEXStageRegisters(Instruction *current_instruction) {
    this->log("Passing register source 1 to IDEXStageRegisters.");
    this->id_ex_stage_registers->setRegisterSource1(current_instruction->getRs1().to_ulong());
    this->log("Passed register source 1 to IDEXStageRegisters.");
}

void IFIDStageRegisters::passRegisterSource2ToIDEXStageRegisters(Instruction *current_instruction) {
    this->log("Passing register source 2 to IDEXStageRegisters.");
    this->id_ex_stage_registers->setRegisterSource2(current_instruction->getRs2().to_ulong());
    this->log("Passed register source 2 to IDEXStageRegisters.");
}

void IFIDStageRegisters::passInstructionToIDEXStageRegisters(Instruction *current_instruction) {
    this->log("Passing instruction to IDEXStageRegisters.");
    this->id_ex_stage_registers->setInstruction(current_instruction);
    this->log("Passed instruction to IDEXStageRegisters.");
}

void IFIDStageRegisters::passNopToIDEXStageRegisters(bool is_asserted) {
    this->log("Passing NOP to IDEXStageRegisters.");
    this->id_ex_stage_registers->setPassedNop(is_asserted);
    this->log("Passed NOP to IDEXStageRegisters.");
}

void IFIDStageRegisters::passInstructionToHazardDetectionUnit(Instruction *current_instruction) {
    this->log("Passing instruction to hazard detection unit.");
    this->hazard_detection_unit->setInstruction(current_instruction);
    this->log("Passed instruction to hazard detection unit.");
}

Instruction *IFIDStageRegisters::getInstruction() {
    return this->instruction;
}

void IFIDStageRegisters::delayUpdateUntilNopFlagSet() {
    std::unique_lock<std::mutex> if_id_stage_registers_lock (this->getModuleMutex());
    this->getModuleConditionVariable().wait(
            if_id_stage_registers_lock,
            [this] {
                return this->is_nop_flag_set;
            }
    );
}

void IFIDStageRegisters::setPassedNop(bool is_asserted) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    this->log("setPassedNop waiting to acquire lock.");

    std::lock_guard<std::mutex> if_id_stage_registers_lock (this->getModuleMutex());

    this->log("setPassedNop acquired lock.");

    this->is_nop_passed_flag_asserted = is_asserted;
    this->is_nop_passed_flag_set = true;

    this->log("setPassedNop updated value.");
    this->notifyModuleConditionVariable();
}

void IFIDStageRegisters::assertSystemEnabledNop() {
    this->is_nop_asserted = true;
}

std::string IFIDStageRegisters::getModuleTag() {
    return "IFIDStageRegisters";
}

Stage IFIDStageRegisters::getModuleStage() {
    return Stage::IF;
}
