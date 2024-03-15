#include "../../include/combinational/ForwardingUnit.h"

ForwardingUnit *ForwardingUnit::current_instance = nullptr;
std::mutex ForwardingUnit::initialization_mutex;

ForwardingUnit::ForwardingUnit() {
    this->register_source1 = 0UL;
    this->register_source2 = 0UL;
    this->ex_mem_stage_register_destination = 0UL;
    this->mem_wb_stage_register_destination = 0UL;

    this->is_single_register_source_set = false;
    this->is_double_register_source_set = false;
    this->is_ex_mem_stage_register_destination_set = false;
    this->is_mem_wb_stage_register_destination_set = false;

    this->is_ex_mem_reg_write_asserted = false;
    this->is_mem_wb_reg_write_asserted = false;

    this->is_ex_mem_reg_write_set = false;
    this->is_mem_wb_reg_write_set = false;

    this->is_reset_flag_set = false;

    this->alu_input_1_mux_control_signal = ALUInputMuxControlSignals::IDEXStageRegisters;
    this->alu_input_2_mux_control_signal = ALUInputMuxControlSignals::IDEXStageRegisters;

    this->alu_input_1_mux = nullptr;
    this->alu_input_2_mux = nullptr;
    this->logger = nullptr;
}

ForwardingUnit *ForwardingUnit::init() {
    std::lock_guard<std::mutex> forwarding_unit_lock (ForwardingUnit::initialization_mutex);

    if (ForwardingUnit::current_instance == nullptr) {
        ForwardingUnit::current_instance = new ForwardingUnit();
    }

    return ForwardingUnit::current_instance;
}

void ForwardingUnit::initDependencies() {
    this->alu_input_1_mux = ALUInput1ForwardingMux::init();
    this->alu_input_2_mux = ALUInput2ForwardingMux::init();
    this->logger = Logger::init();
}

void ForwardingUnit::run() {
    this->initDependencies();

    while (this->isAlive()) {
        this->logger->log(Stage::EX, "[ForwardingUnit] Waiting to be woken up and acquire lock.");

        std::unique_lock<std::mutex> forwarding_unit_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                forwarding_unit_lock,
                [this] {
                    return ((this->is_single_register_source_set || this->is_double_register_source_set) &&
                        (this->getPipelineType() == PipelineType::Single ||
                        (this->is_ex_mem_stage_register_destination_set && this->is_ex_mem_reg_write_set &&
                        this->is_mem_wb_stage_register_destination_set && this->is_mem_wb_reg_write_set))) ||
                        this->is_reset_flag_set || this->isKilled();
                }
        );

        if (this->isKilled()) {
            this->logger->log(Stage::EX, "[ForwardingUnit] Killed.");
            break;
        }

        if (this->is_reset_flag_set) {
            this->logger->log(Stage::EX, "[ForwardingUnit] Resetting.");

            this->resetState();
            this->is_reset_flag_set = false;

            this->logger->log(Stage::EX, "[ForwardingUnit] Reset.");
            continue;
        }

        this->logger->log(Stage::EX, "[ForwardingUnit] Woken up and acquired lock.");

        this->computeControlSignals();

        std::thread pass_control_signal_to_alu_input_1_forwarding_mux (
                &ForwardingUnit::passControlSignalToALUInput1ForwardingMux,
                this
        );

        std::thread pass_control_signal_to_alu_input_2_forwarding_mux (
                &ForwardingUnit::passControlSignalToALUInput2ForwardingMux,
                this
        );

        pass_control_signal_to_alu_input_1_forwarding_mux.join();
        pass_control_signal_to_alu_input_2_forwarding_mux.join();

        this->is_single_register_source_set = false;
        this->is_double_register_source_set = false;
        this->is_ex_mem_stage_register_destination_set = false;
        this->is_mem_wb_stage_register_destination_set = false;
        this->is_ex_mem_reg_write_set = false;
        this->is_mem_wb_reg_write_set = false;
        this->is_ex_mem_reg_write_asserted = false;
        this->is_mem_wb_reg_write_asserted = false;
    }
}

void ForwardingUnit::setSingleRegisterSource(unsigned long rs1) {
    this->logger->log(Stage::EX, "[ForwardingUnit] setSingleRegisterSource waiting to acquire lock.");

    std::lock_guard<std::mutex> forwarding_unit_lock (this->getModuleMutex());

    this->logger->log(Stage::EX, "[ForwardingUnit] setSingleRegisterSource acquired lock. Updating value.");

    this->register_source1 = rs1;
    this->is_single_register_source_set = true;

    this->logger->log(Stage::EX, "[ForwardingUnit] setSingleRegisterSource value updated.");
    this->notifyModuleConditionVariable();
}

void ForwardingUnit::setDoubleRegisterSource(unsigned long rs1, unsigned long rs2) {
    this->logger->log(Stage::EX, "[ForwardingUnit] setDoubleRegisterSource waiting to acquire lock.");

    std::lock_guard<std::mutex> forwarding_unit_lock (this->getModuleMutex());

    this->logger->log(Stage::EX, "[ForwardingUnit] setDoubleRegisterSource acquired lock. Updating values.");

    this->register_source1 = rs1;
    this->register_source2 = rs2;
    this->is_double_register_source_set = true;

    this->logger->log(Stage::EX, "[ForwardingUnit] setDoubleRegisterSource values updated.");
    this->notifyModuleConditionVariable();

}

void ForwardingUnit::setEXMEMStageRegisterDestination(unsigned long rd) {
    this->logger->log(Stage::EX, "[ForwardingUnit] setEXMEMStageRegisterDestination waiting to acquire lock.");

    std::lock_guard<std::mutex> forwarding_unit_lock (this->getModuleMutex());

    this->logger->log(Stage::EX, "[ForwardingUnit] setEXMEMStageRegisterDestination acquired lock. Updating value.");

    this->ex_mem_stage_register_destination = rd;
    this->is_ex_mem_stage_register_destination_set = true;

    this->logger->log(Stage::EX, "[ForwardingUnit] setEXMEMStageRegisterDestination value updated.");
    this->notifyModuleConditionVariable();
}

void ForwardingUnit::setMEMWBStageRegisterDestination(unsigned long rd) {
    this->logger->log(Stage::EX, "[ForwardingUnit] setMEMWBStageRegisterDestination waiting to acquire lock.");

    std::lock_guard<std::mutex> forwarding_unit_lock (this->getModuleMutex());

    this->logger->log(Stage::EX, "[ForwardingUnit] setMEMWBStageRegisterDestination acquired lock. Updating value.");

    this->mem_wb_stage_register_destination = rd;
    this->is_mem_wb_stage_register_destination_set = true;

    this->logger->log(Stage::EX, "[ForwardingUnit] setMEMWBStageRegisterDestination values updated.");
    this->notifyModuleConditionVariable();
}

void ForwardingUnit::setEXMEMStageRegisterRegWrite(bool is_asserted) {
    this->logger->log(Stage::EX, "[ForwardingUnit] setEXMEMStageRegisterRegWrite waiting to acquire lock.");

    std::lock_guard<std::mutex> forwarding_unit_lock (this->getModuleMutex());

    this->logger->log(Stage::EX, "[ForwardingUnit] setEXMEMStageRegisterRegWrite acquired lock. Updating value.");

    this->is_ex_mem_reg_write_asserted = is_asserted;
    this->is_ex_mem_reg_write_set = true;

    this->logger->log(Stage::EX, "[ForwardingUnit] setEXMEMStageRegisterRegWrite values updated.");
    this->notifyModuleConditionVariable();
}

void ForwardingUnit::setMEMWBStageRegisterRegWrite(bool is_asserted) {
    this->logger->log(Stage::EX, "[ForwardingUnit] setEXMEMStageRegisterRegWrite waiting to acquire lock.");

    std::lock_guard<std::mutex> forwarding_unit_lock (this->getModuleMutex());

    this->logger->log(Stage::EX, "[ForwardingUnit] setEXMEMStageRegisterRegWrite acquired lock. Updating value.");

    this->is_mem_wb_reg_write_asserted = is_asserted;
    this->is_mem_wb_reg_write_set = true;

    this->logger->log(Stage::EX, "[ForwardingUnit] setEXMEMStageRegisterRegWrite values updated.");
    this->notifyModuleConditionVariable();
}

void ForwardingUnit::reset() {
    if (!this->logger) {
        this->initDependencies();
    }

    this->logger->log(Stage::EX, "[ForwardingUnit] reset flag set.");
    this->is_reset_flag_set = true;
    this->notifyModuleConditionVariable();
}

void ForwardingUnit::resetState() {
    this->is_single_register_source_set = false;
    this->is_double_register_source_set = false;
    this->is_ex_mem_stage_register_destination_set = false;
    this->is_mem_wb_stage_register_destination_set = false;
    this->is_ex_mem_reg_write_set = false;
    this->is_mem_wb_reg_write_set = false;

    this->is_ex_mem_reg_write_asserted = false;
    this->is_mem_wb_reg_write_asserted = false;

    this->register_source1 = 0UL;
    this->register_source2 = 0UL;
    this->ex_mem_stage_register_destination = 0UL;
    this->mem_wb_stage_register_destination = 0UL;
}

void ForwardingUnit::computeControlSignals() {
    this->logger->log(Stage::EX, "[ForwardingUnit] Computing control signals.");

    if (this->getPipelineType() == PipelineType::Single) {
        this->alu_input_1_mux_control_signal = ALUInputMuxControlSignals::IDEXStageRegisters;
        this->alu_input_2_mux_control_signal = ALUInputMuxControlSignals::IDEXStageRegisters;
    } else {
        this->alu_input_1_mux_control_signal = ALUInputMuxControlSignals::IDEXStageRegisters;
        this->alu_input_2_mux_control_signal = ALUInputMuxControlSignals::IDEXStageRegisters;

        if (this->register_source1 == this->ex_mem_stage_register_destination && this->is_ex_mem_reg_write_asserted) {
            this->alu_input_1_mux_control_signal = ALUInputMuxControlSignals::EXMEMStageRegisters;
        } else if (this->register_source1 == this->mem_wb_stage_register_destination && this->is_mem_wb_reg_write_set) {
            this->alu_input_1_mux_control_signal = ALUInputMuxControlSignals::MEMWBStageRegisters;
        }

        if (this->is_double_register_source_set) {
            if (this->register_source2 == this->ex_mem_stage_register_destination && this->is_ex_mem_reg_write_asserted) {
                this->alu_input_2_mux_control_signal = ALUInputMuxControlSignals::EXMEMStageRegisters;
            } else if (this->register_source2 == this->mem_wb_stage_register_destination && this->is_mem_wb_reg_write_asserted) {
                this->alu_input_2_mux_control_signal = ALUInputMuxControlSignals::MEMWBStageRegisters;
            }
        }
    }

    this->logger->log(Stage::EX, "[ForwardingUnit] Control signals computed.");
}

void ForwardingUnit::passControlSignalToALUInput1ForwardingMux() {
    this->logger->log(Stage::EX, "[ForwardingUnit] Passing input to ALUInput1ForwardingMux.");
    this->alu_input_1_mux->setMuxControlSignal(this->alu_input_1_mux_control_signal);
    this->logger->log(Stage::EX, "[ForwardingUnit] Passed input to ALUInput1ForwardingMux.");
}

void ForwardingUnit::passControlSignalToALUInput2ForwardingMux() {
    this->logger->log(Stage::EX, "[ForwardingUnit] Passing input to ALUInput2ForwardingMux.");
    this->alu_input_2_mux->setMuxControlSignal(this->alu_input_2_mux_control_signal);
    this->logger->log(Stage::EX, "[ForwardingUnit] Passing input to ALUInput2ForwardingMux.");
}

