#include "../../../include/state/stage-registers/IFIDStageRegisters.h"

IFIDStageRegisters *IFIDStageRegisters::current_instance = nullptr;

IFIDStageRegisters::IFIDStageRegisters() {
    this->program_counter = 0UL;
    this->instruction_bits = std::string(32, '0');

    this->is_program_counter_set = true;
    this->is_instruction_set = true;
    this->is_nop_asserted = false;
    this->is_reset_flag_set = false;

    this->if_logger = IFLogger::init();
    this->id_logger = IDLogger::init();

    this->instruction = new Instruction(std::string(32, '0'));
    this->control = new Control(this->instruction);

    this->register_file = RegisterFile::init();
    this->id_ex_stage_registers = IDEXStageRegisters::init();
    this->immediate_generator = ImmediateGenerator::init();
    this->stage_synchronizer = StageSynchronizer::init();
}

void IFIDStageRegisters::reset() {
    this->is_reset_flag_set = true;
}

void IFIDStageRegisters::resetStage() {
    if (this->getStage() == Stage::Single) {
        this->is_program_counter_set = false;
        this->is_instruction_set = false;
    } else {
        this->is_program_counter_set = true;
        this->is_instruction_set = true;
    }

    this->is_nop_asserted = false;
    this->is_reset_flag_set = false;

    this->program_counter = 0UL;
    this->instruction = new Instruction(std::string(32, '0'));
    this->control = new Control(this->instruction);
}

IFIDStageRegisters *IFIDStageRegisters::init() {
    if (IFIDStageRegisters::current_instance == nullptr) {
        IFIDStageRegisters::current_instance = new IFIDStageRegisters();
    }

    return IFIDStageRegisters::current_instance;
}

void IFIDStageRegisters::run() {
    while (this->isAlive()) {
        std::unique_lock<std::mutex> if_id_stage_registers_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                if_id_stage_registers_lock,
                [this] { return this->is_instruction_set && this->is_program_counter_set; }
        );

        if (this->is_reset_flag_set) {
            this->resetStage();
            this->is_reset_flag_set = false;

            continue;
        }

        this->instruction = new Instruction(this->instruction_bits);
        this->control = new Control(this->instruction);

        std::thread pass_control_thread (&IFIDStageRegisters::passControlToIDEXStageRegisters, this);
        std::thread pass_program_counter_thread (&IFIDStageRegisters::passProgramCounterToIDEXStageRegisters, this);
        std::thread pass_read_registers_thread (&IFIDStageRegisters::passReadRegistersToRegisterFile, this);
        std::thread pass_instruction_thread (&IFIDStageRegisters::passInstructionToImmediateGenerator, this);
        std::thread pass_register_destination_thread (&IFIDStageRegisters::passRegisterDestinationToIDEXStageRegisters, this);

        this->is_instruction_set = false;
        this->is_program_counter_set = false;

        this->stage_synchronizer->conditionalArriveSingleStage();
    }
}

void IFIDStageRegisters::setInput(const std::variant<unsigned long, std::string> &input) {
    this->stage_synchronizer->conditionalArriveFiveStage();

    std::unique_lock<std::mutex> if_id_stage_registers_lock (this->getModuleMutex());

    if (std::holds_alternative<unsigned long>(input)) {
        this->program_counter = std::get<unsigned long>(input);
        this->is_program_counter_set = true;
    } else if (std::holds_alternative<std::string>(input)) {
        if (this->is_nop_asserted) {
            this->instruction_bits = std::string(32, '0');
        } else {
            this->instruction_bits = std::get<std::string>(input);
        }

        this->is_instruction_set = true;
    } else {
        std::cerr << "IFStageRegisters::setInput for IFIDStageRegisters passed an unsupported value" << std::endl;
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

void IFIDStageRegisters::setNop() {
    this->is_nop_asserted = true;
}