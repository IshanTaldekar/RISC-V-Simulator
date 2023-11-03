#include "../../include/state/RegisterFile.h"

RegisterFile::RegisterFile() {
    this->is_single_read_register_set = false;
    this->is_double_read_register_set = false;
    this->is_write_register_set = false;
    this->is_reg_write_signal_set = false;
    this->is_write_thread_finished = false;

    this->register_source1 = -1;
    this->register_source2 = -1;

    this->register_destination = -1;

    std::string empty_word = std::string(32, '0');

    for (int i = 0; i < REGISTERS_COUNT; ++i) {
        this->registers.emplace_back(empty_word);
    }

    this->logger = IDLogger::init();
    this->id_ex_stage_registers = IDEXStageRegisters::init();
}

RegisterFile *RegisterFile::init() {
    if (RegisterFile::current_instance == nullptr) {
        RegisterFile::current_instance = new RegisterFile();
    }

    return RegisterFile::current_instance;
}

void RegisterFile::run() {
    while (this->isAlive()) {
        std::unique_lock<std::mutex> register_file_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                register_file_lock,
                [this] {
                    return this->is_write_register_set &&
                           (this->is_single_read_register_set || this->is_double_read_register_set);
                }
        );

        std::thread write_register_thread (&RegisterFile::writeDataToRegisterFile, this);
        std::thread pass_data_thread (&RegisterFile::passReadRegisterDataToIDEXStageRegister, this);

        this->getModuleConditionVariable().wait(
                register_file_lock,
                [this] {
                    return !this->is_write_register_set &&
                            !(this->is_single_read_register_set || this->is_double_read_register_set);
                }
        );

        this->is_write_register_set = false;
        this->is_reg_write_signal_set = false;
        this->is_single_read_register_set = false;
        this->is_double_read_register_set = false;
        this->is_write_thread_finished = false;
    }
}

void RegisterFile::setReadRegister(unsigned long rs1) {
    std::lock_guard<std::mutex> register_file_lock (this->getModuleMutex());

    this->register_source1 = rs1;
    this->register_source2 = -1;

    this->is_single_read_register_set = true;
}

void RegisterFile::setReadRegisters(unsigned long rs1, unsigned long rs2) {
    std::lock_guard<std::mutex> register_file_lock (this->getModuleMutex());

    this->register_source1 = rs1;
    this->register_source2 = rs2;

    this->is_double_read_register_set = true;
}

void RegisterFile::setWriteRegister(unsigned long rd, const std::bitset<WORD_BIT_COUNT> &data) {
    std::lock_guard<std::mutex> register_file_lock (this->getModuleMutex());

    this->register_destination = rd;
    this->write_data = data;

    this->is_write_register_set = true;
}

void RegisterFile::setRegWriteSignal() {
    std::lock_guard<std::mutex> register_file_lock (this->getModuleMutex());
    this->is_reg_write_signal_set = true;
}

void RegisterFile::notifyModuleConditionVariable() {
    this->getModuleConditionVariable().notify_one();
}

void RegisterFile::passReadRegisterDataToIDEXStageRegister() {
    std::unique_lock<std::mutex> load_lock (this->write_load_mutex);
    this->load_condition_variable.wait(
            load_lock,
            [this] { return this->is_write_thread_finished; }
    );

    std::lock_guard<std::mutex> register_file_lock (this->getModuleMutex());

    if (!this->is_single_read_register_set && !this->is_double_read_register_set) {
        std::cerr << "[RegisterFile] no register read set for a cycle." << std::endl;
    }

    if (this->is_single_read_register_set) {
        this->id_ex_stage_registers->setRegisterData(this->registers.at(this->register_source1));
    } else if (this->is_double_read_register_set) {
        this->id_ex_stage_registers->setRegisterData(
                this->registers.at(this->register_source1),
                this->registers.at(this->register_source2)
        );
    }

    this->is_single_read_register_set = false;
    this->is_double_read_register_set = false;

    this->notifyModuleConditionVariable();
}

void RegisterFile::writeDataToRegisterFile() {
    std::unique_lock<std::mutex> register_file_lock (this->getModuleMutex());

    if (this->is_reg_write_signal_set) {
        this->registers.at(this->register_destination) = this->write_data;
    }

    register_file_lock.unlock();

    std::lock_guard<std::mutex> load_lock (this->write_load_mutex);
    this->is_write_thread_finished = true;

    this->load_condition_variable.notify_one();

    this->is_write_register_set = false;
    this->is_reg_write_signal_set = false;

    this->notifyModuleConditionVariable();
}


