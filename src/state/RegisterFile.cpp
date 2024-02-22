#include "../../include/state/RegisterFile.h"

RegisterFile *RegisterFile::current_instance = nullptr;

RegisterFile::RegisterFile() {
    this->is_single_read_register_set = false;
    this->is_double_read_register_set = false;
    this->is_write_register_set = false;
    this->is_reg_write_signal_set = false;
    this->is_write_thread_finished = false;
    this->is_write_data_set = false;
    this->is_reset_flag_set = false;

    this->register_source1 = 0UL;
    this->register_source2 = 0UL;

    this->register_destination = 0UL;

    this->logger = IDLogger::init();
    this->id_ex_stage_registers = IDEXStageRegisters::init();

    this->resetRegisterFileContents();
}

RegisterFile *RegisterFile::init() {
    if (RegisterFile::current_instance == nullptr) {
        RegisterFile::current_instance = new RegisterFile();
    }

    return RegisterFile::current_instance;
}

void RegisterFile::run() {
    while (this->isAlive()) {
        this->logger->log("[RegisterFile] Waiting to be woken up and acquire lock.");

        std::unique_lock<std::mutex> register_file_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                register_file_lock,
                [this] {
                    return (this->is_write_register_set && this->is_write_data_set &&
                           (this->is_single_read_register_set || this->is_double_read_register_set)) ||
                           this->is_reset_flag_set;
                }
        );

        if (this->isKilled()) {
            break;
        }

        if (this->is_reset_flag_set) {
            this->resetRegisterFileContents();
            continue;
        }

        this->logger->log("[RegisterFile] Woken up and acquired lock. Writing value and passing data.");

        std::thread write_register_thread (&RegisterFile::writeDataToRegisterFile, this);
        std::thread pass_data_thread (&RegisterFile::passReadRegisterDataToIDEXStageRegister, this);

        this->getModuleConditionVariable().wait(
                register_file_lock,
                [this] {
                    return !this->is_write_register_set && !this->is_write_data_set &&
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

void RegisterFile::reset() {
    this->logger->log("[RegisterFile] Resetting register file contents.");

    std::lock_guard<std::mutex> register_file_lock (this->getModuleMutex());

    this->is_reset_flag_set = true;
    this->notifyModuleConditionVariable();
}

void RegisterFile::setReadRegister(unsigned long rs1) {
    this->logger->log("[RegisterFile] Setting rs1.");

    std::lock_guard<std::mutex> register_file_lock (this->getModuleMutex());

    this->register_source1 = rs1;
    this->register_source2 = -1;

    this->is_single_read_register_set = true;

    this->logger->log("[RegisterFile] rs1 set.");

    this->notifyModuleConditionVariable();
}

void RegisterFile::setReadRegisters(unsigned long rs1, unsigned long rs2) {
    this->logger->log("[RegisterFile] Setting rs1 and rs2.");

    std::lock_guard<std::mutex> register_file_lock (this->getModuleMutex());

    this->register_source1 = rs1;
    this->register_source2 = rs2;

    this->is_double_read_register_set = true;

    this->logger->log("[RegisterFile] rs1 and rs2 set.");

    this->notifyModuleConditionVariable();
}

void RegisterFile::setWriteRegister(unsigned long rd) {
    this->logger->log("[RegisterFile] Setting write register.");

    std::lock_guard<std::mutex> register_file_lock (this->getModuleMutex());

    this->register_destination = rd;
    this->is_write_register_set = true;

    this->logger->log("[RegisterFile] Write register set.");

    this->notifyModuleConditionVariable();
}

void RegisterFile::setRegWriteSignal(bool is_asserted) {
    this->logger->log("[RegisterFile] Setting reg_write signal.");

    std::lock_guard<std::mutex> register_file_lock (this->getModuleMutex());
    this->is_reg_write_signal_set = true;

    this->logger->log("[RegisterFile] reg_write signal set.");

    this->notifyModuleConditionVariable();
}

void RegisterFile::notifyModuleConditionVariable() {
    this->getModuleConditionVariable().notify_one();
}

void RegisterFile::passReadRegisterDataToIDEXStageRegister() {
    this->logger->log("[RegisterFile] Waiting for write to finish to pass contents to IDEXStageRegister.");

    std::unique_lock<std::mutex> load_lock (this->write_load_mutex);
    this->load_condition_variable.wait(
            load_lock,
            [this] { return this->is_write_thread_finished; }
    );


    std::lock_guard<std::mutex> register_file_lock (this->getModuleMutex());

    if (!this->is_single_read_register_set && !this->is_double_read_register_set) {
        throw std::runtime_error("[RegisterFile] no register read set for a cycle.");
    }

    this->logger->log("[RegisterFile] Passing values to IDEXStageRegisters.");

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

    this->notifyModuleConditionVariable();
}

void RegisterFile::writeDataToRegisterFile() {
    this->logger->log("[RegisterFile] Waiting to acquire lock to write to register file.");

    std::lock_guard<std::mutex> register_file_lock (this->getModuleMutex());

    if (this->is_reg_write_signal_set && this->register_destination != 0) {
        this->registers.at(this->register_destination) = this->write_data;
    }

    this->logger->log("[RegisterFile] Write complete. Waking up thread to pass values to IDEXStageRegister.");

    std::lock_guard<std::mutex> load_lock (this->write_load_mutex);
    this->is_write_thread_finished = true;

    this->load_condition_variable.notify_one();

    this->is_write_register_set = false;
    this->is_write_data_set = false;
    this->is_reg_write_signal_set = false;

    this->notifyModuleConditionVariable();
}

void RegisterFile::setWriteData(unsigned long value) {
    this->logger->log("[RegisterFile] Waiting to set write data.");

    std::lock_guard<std::mutex> register_file_lock (this->getModuleMutex());

    this->write_data = std::bitset<WORD_BIT_COUNT>(value);
    this->is_write_data_set = true;

    this->logger->log("[RegisterFile] Write data set.");

    this->notifyModuleConditionVariable();
}

void RegisterFile::resetRegisterFileContents() {
    this->registers.clear();

    std::string empty_word = std::string(32, '0');

    for (int i = 0; i < REGISTERS_COUNT; ++i) {
        this->registers.emplace_back(empty_word);
    }

    this->logger->log("[RegisterFile] Registers cleared.");
}

