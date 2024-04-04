#include "../../include/state/RegisterFile.h"

RegisterFile *RegisterFile::current_instance = nullptr;
std::mutex RegisterFile::initialization_mutex;

RegisterFile::RegisterFile() {
    this->is_single_read_register_set = false;
    this->is_double_read_register_set = false;
    this->is_reset_flag_set = false;

    this->is_write_register_set = false;
    this->is_write_data_set = false;
    this->is_reg_write_signal_set = false;
    this->is_pause_flag_set = false;

    this->is_reg_write_signal_asserted = false;

    this->register_source1 = 0UL;
    this->register_source2 = 0UL;

    this->register_destination = 0UL;

    this->logger = nullptr;
    this->id_ex_stage_registers = nullptr;
    this->stage_synchronizer = nullptr;

    this->output_file_path = "../output/RegisterFile";
    std::ofstream output_file (this->output_file_path + "-SS.log", std::ios::out);
    output_file.close();
}

RegisterFile *RegisterFile::init() {
    std::lock_guard<std::mutex> register_file_lock (RegisterFile::initialization_mutex);

    if (RegisterFile::current_instance == nullptr) {
        RegisterFile::current_instance = new RegisterFile();
    }

    return RegisterFile::current_instance;
}

void RegisterFile::initDependencies() {
    std::unique_lock<std::mutex> register_file_lock (this->getModuleMutex());

    if (this->logger && this->id_ex_stage_registers && this->stage_synchronizer) {
        return;
    }

    this->logger = Logger::init();
    this->id_ex_stage_registers = IDEXStageRegisters::init();
    this->stage_synchronizer = StageSynchronizer::init();
}

void RegisterFile::pause() {
    this->log("Paused.");
    this->is_pause_flag_set = true;
}

void RegisterFile::resume() {
    this->log("Resumed.");
    this->is_pause_flag_set = false;
    this->notifyModuleConditionVariable();
}


void RegisterFile::run() {
    this->initDependencies();
    this->resetRegisterFileContents();

    while (this->isAlive()) {
        this->log("Waiting to be woken up and acquire lock.");

        std::unique_lock<std::mutex> register_file_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                register_file_lock,
                [this] {
                    return ((this->getPipelineType() == PipelineType::Single || (this->is_write_register_set &&
                            this->is_write_data_set && this->is_reg_write_signal_set)) &&
                            (this->is_single_read_register_set || this->is_double_read_register_set) &&
                            !this->is_pause_flag_set) || this->is_reset_flag_set || this->isKilled();
                }
        );

        if (this->isKilled()) {
            this->log("Killed.");
            break;
        }

        if (this->is_reset_flag_set) {
            this->log("Resetting stage.");

            this->resetState();
            this->resetRegisterFileContents();

            this->is_reset_flag_set = false;

            this->log("Reset.");
            continue;
        }

        this->log("Woken up and acquired lock.");

        if (this->getPipelineType() == PipelineType::Single) {
            this->passReadRegisterDataToIDEXStageRegister();

            this->log("Waiting for write items.");

            this->getModuleConditionVariable().wait(
                    register_file_lock,
                    [this] {
                        return this->is_write_register_set && this->is_write_data_set && this->is_reg_write_signal_set;
                    }
            );

            this->log("Write items set.");

            this->writeDataToRegisterFile();
            this->log("Write items set. Waiting on barrier.");

            this->stage_synchronizer->conditionalArriveSingleStage();
        } else {
            this->writeDataToRegisterFile();
            std::thread pass_data_thread (&RegisterFile::passReadRegisterDataToIDEXStageRegister, this);
            pass_data_thread.join();
        }

        this->is_write_register_set = false;
        this->is_reg_write_signal_set = false;
        this->is_single_read_register_set = false;
        this->is_double_read_register_set = false;
    }
}

void RegisterFile::reset() {
    std::lock_guard<std::mutex> register_file_lock (this->getModuleMutex());

    this->is_reset_flag_set = true;
    this->notifyModuleConditionVariable();
}

void RegisterFile::setReadRegister(unsigned long rs1) {
    this->log("setReadRegister waiting to acquire lock.");

    std::lock_guard<std::mutex> register_file_lock (this->getModuleMutex());

    this->log("setReadRegister acquired lock. Updating value.");

    this->register_source1 = rs1;
    this->register_source2 = 0UL;

    this->is_single_read_register_set = true;

    this->log("setReadRegister updated value.");

    this->notifyModuleConditionVariable();
}

void RegisterFile::setReadRegisters(unsigned long rs1, unsigned long rs2) {
    this->log("setReadRegister waiting to acquire lock.");

    std::lock_guard<std::mutex> register_file_lock (this->getModuleMutex());

    this->log("setReadRegister acquired lock. Updating values.");

    this->register_source1 = rs1;
    this->register_source2 = rs2;

    this->is_double_read_register_set = true;

    this->log("setReadRegister updated values.");
    this->notifyModuleConditionVariable();
}

void RegisterFile::setWriteRegister(unsigned long rd) {
    this->log("setWriteRegister waiting to acquire lock.");

    std::lock_guard<std::mutex> register_file_lock (this->getModuleMutex());

    this->log("setWriteRegister acquired lock. Updating value.");

    this->register_destination = rd;
    this->is_write_register_set = true;

    this->log("setWriteRegister updated value.");
    this->notifyModuleConditionVariable();
}

void RegisterFile::setRegWriteSignal(bool is_asserted) {
    this->log("setRegWriteSignal waiting to acquire lock.");

    std::lock_guard<std::mutex> register_file_lock (this->getModuleMutex());

    this->log("setRegWriteSignal acquired lock. Updating value.");

    this->is_reg_write_signal_asserted = is_asserted;
    this->is_reg_write_signal_set = true;

    this->log("setRegWriteSignal updated value.");
    this->notifyModuleConditionVariable();
}

void RegisterFile::setWriteData(std::bitset<WORD_BIT_COUNT> value) {
    this->log("setWriteData waiting to acquire lock.");

    std::lock_guard<std::mutex> register_file_lock (this->getModuleMutex());

    this->log("setWriteData acquired lock. Updating value.");

    this->write_data = value;
    this->is_write_data_set = true;

    this->log("setWriteData updated value.");
    this->notifyModuleConditionVariable();
}

void RegisterFile::passReadRegisterDataToIDEXStageRegister() {
    this->log("passReadRegisterDataToIDEXStageRegister waiting to pass values to IDEXStageRegisters.");

    if (!this->is_single_read_register_set && !this->is_double_read_register_set) {
        throw std::runtime_error("[RegisterFile] no register read set for a cycle.");
    }

    if (this->is_single_read_register_set) {
        this->id_ex_stage_registers->setRegisterData(this->registers.at(this->register_source1));
    } else {
        this->id_ex_stage_registers->setRegisterData(
                this->registers.at(this->register_source1),
                this->registers.at(this->register_source2)
        );
    }

    this->is_single_read_register_set = false;
    this->is_double_read_register_set = false;

    this->log("passReadRegisterDataToIDEXStageRegister Passed values to IDEXStageRegisters.");
}

void RegisterFile::writeDataToRegisterFile() {
    this->log("writeDataToRegisterFile Waiting to acquire lock to write to register file.");

    if (this->is_reg_write_signal_set && this->is_reg_write_signal_asserted && this->register_destination != 0) {
        this->registers.at(this->register_destination) = this->write_data;
        this->log("writeDataToRegisterFile Write complete.");

    }

    this->is_write_register_set = false;
    this->is_write_data_set = false;
    this->is_reg_write_signal_set = false;

    this->log("writeDataToRegisterFile Waking up thread to pass values to IDEXStageRegister.");
}

void RegisterFile::resetRegisterFileContents() {
    this->registers.clear();

    std::string empty_word = std::string(32, '0');

    for (int i = 0; i < REGISTERS_COUNT; ++i) {
        this->registers.emplace_back(empty_word);
    }

    this->log("Registers cleared.");
}

void RegisterFile::resetState() {
    this->is_write_register_set = false;
    this->is_write_data_set = false;
    this->is_reg_write_signal_set = false;
    this->is_single_read_register_set = false;
    this->is_double_read_register_set = false;
    this->is_reg_write_signal_asserted = false;

    std::ofstream output_file (
            this->output_file_path + (this->getPipelineType() == PipelineType::Single ?  "-SS.log" : "-FS.log"),
            std::ios::out
    );
    output_file.close();
}

void RegisterFile::writeRegisterFileContentsToOutputFile(int cycle_count) {
    std::ofstream output_file (
            this->output_file_path + (this->getPipelineType() == PipelineType::Single ?  "-SS.log" : "-FS.log"),
            std::ios::app
    );

    output_file << "State of RF after executing cycle:\t" << cycle_count << std::endl;

    for (const std::bitset<WORD_BIT_COUNT> &data: this->registers) {
        output_file << data.to_string() << std::endl;
    }

    output_file.close();
}

std::string RegisterFile::getModuleTag() {
    return "RegisterFile";
}

Stage RegisterFile::getModuleStage() {
    return Stage::ID;
}