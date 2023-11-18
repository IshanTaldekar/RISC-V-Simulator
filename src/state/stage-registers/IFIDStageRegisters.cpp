#include "../../../include/state/stage-registers/IFIDStageRegisters.h"

IFIDStageRegisters *IFIDStageRegisters::current_instance = nullptr;

IFIDStageRegisters::IFIDStageRegisters() {
    this->program_counter = -1;
    this->instruction_bits = "";

    this->is_program_counter_set = false;
    this->is_instruction_set = false;

    this->if_logger = IFLogger::init();
    this->id_logger = IDLogger::init();

    this->instruction = nullptr;
    this->control = nullptr;

    this->register_file = RegisterFile::init();
    this->id_ex_stage_registers = IDEXStageRegisters::init();
    this->immediate_generator = ImmediateGenerator::init();
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

        this->instruction = new Instruction(this->instruction_bits);
        this->control = new Control(this->instruction);

        std::thread pass_control_thread (&IFIDStageRegisters::passControlToIDEXStageRegisters, this);
        std::thread pass_program_counter_thread (&IFIDStageRegisters::passProgramCounterToIDEXStageRegisters, this);
        std::thread pass_read_registers_thread (&IFIDStageRegisters::passReadRegistersToRegisterFile, this);
        std::thread pass_instruction_thread (&IFIDStageRegisters::passInstructionToImmediateGenerator, this);
        std::thread pass_register_destination_thread (&IFIDStageRegisters::passRegisterDestinationToIDEXStageRegisters, this);

        this->is_instruction_set = false;
        this->is_program_counter_set = false;
    }
}

void IFIDStageRegisters::setInput(std::variant<int, std::string> input) {
    std::unique_lock<std::mutex> if_id_stage_registers_lock (this->getModuleMutex());

    if (std::holds_alternative<int>(input)) {
        this->program_counter = std::get<int>(input);
        this->is_program_counter_set = true;
    } else if (std::holds_alternative<std::string>(input)) {
        this->instruction_bits = std::get<std::string>(input);
        this->is_instruction_set = true;
    } else {
        std::cerr << "IFStageRegisters::setInput for IFIDStageRegisters passed an unsupported value" << std::endl;
    }
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

    if (type == InstructionType::R || type == InstructionType::B) {
        this->register_file->setReadRegisters(this->instruction->getRs1().to_ulong(), this->instruction->getRs2().to_ulong());
    } else if (type == InstructionType::I || type == InstructionType::S) {
        this->register_file->setReadRegister(this->instruction->getRs1().to_ulong());
    }
}

void IFIDStageRegisters::passInstructionToImmediateGenerator() {
    this->immediate_generator->setInstruction(this->instruction);
}

void IFIDStageRegisters::passRegisterDestinationToIDEXStageRegisters() {
    this->id_ex_stage_registers->setRegisterDestination(this->instruction->getRd().to_ulong());
}