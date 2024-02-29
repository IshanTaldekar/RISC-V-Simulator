#include "../../../../include/combinational/mux/forwarding/ALUInput2ForwardingMux.h"
#include "../../../../include/combinational/ALU.h"

ALUInput2ForwardingMux *ALUInput2ForwardingMux::current_instance = nullptr;
std::mutex ALUInput2ForwardingMux::initialization_mutex;

ALUInput2ForwardingMux *ALUInput2ForwardingMux::init() {
    std::lock_guard<std::mutex> alu_input_2_forwarding_mux_lock (ALUInput2ForwardingMux::initialization_mutex);

    if (ALUInput2ForwardingMux::current_instance == nullptr) {
        ALUInput2ForwardingMux::current_instance = new ALUInput2ForwardingMux();
    }

    return ALUInput2ForwardingMux::current_instance;
}

void ALUInput2ForwardingMux::passOutput() {
    this->logger->log(Stage::EX, "[" + this->getModuleTag() + "] Passing value to ALU Input 2.");

    if (this->control_signal == ALUInputMuxControlSignals::IDEXStageRegisters) {
        this->alu->setInput2(this->id_ex_stage_registers_value);
    } else if (this->control_signal == ALUInputMuxControlSignals::EXMEMStageRegisters) {
        this->alu->setInput2(this->ex_mem_stage_registers_value);
    } else if (this->control_signal == ALUInputMuxControlSignals::MEMWBStageRegisters) {
        this->alu->setInput2(this->mem_wb_stage_registers_value);
    } else {
        throw std::runtime_error("[ALUInput2ForwardingMux] control signal type did not match any existing type.");
    }

    this->logger->log(Stage::EX, "[" + this->getModuleTag() + "] Passed value to ALU Input 2.");
}

std::string ALUInput2ForwardingMux::getModuleTag() {
    return "ALUInput2ForwardingMux";
}
