#include "../../../../include/combinational/mux/forwarding/ALUInput1ForwardingMux.h"
#include "../../../../include/combinational/ALU.h"

ALUInput1ForwardingMux *ALUInput1ForwardingMux::current_instance = nullptr;
std::mutex ALUInput1ForwardingMux::initialization_mutex;

ALUInput1ForwardingMux *ALUInput1ForwardingMux::init() {
    std::lock_guard<std::mutex> alu_input_1_forwarding_mux_lock (ALUInput1ForwardingMux::initialization_mutex);

    if (ALUInput1ForwardingMux::current_instance == nullptr) {
        ALUInput1ForwardingMux::current_instance = new ALUInput1ForwardingMux();
    }

    return ALUInput1ForwardingMux::current_instance;
}

void ALUInput1ForwardingMux::passOutput() {
    this->log("Passing value to ALU Input 1.");

    if (this->control_signal == ALUInputMuxControlSignals::IDEXStageRegisters) {
        this->alu->setInput1(this->id_ex_stage_registers_value);
    } else if (this->control_signal == ALUInputMuxControlSignals::EXMEMStageRegisters) {
        this->alu->setInput1(this->ex_mem_stage_registers_value);
    } else if (this->control_signal == ALUInputMuxControlSignals::MEMWBStageRegisters) {
        this->alu->setInput1(this->mem_wb_stage_registers_value);
    } else {
        throw std::runtime_error("[ALUInput1ForwardingMux] control signal type did not match any existing type.");
    }

    this->log("Passed value to ALU Input 1.");
}

std::string ALUInput1ForwardingMux::getModuleTag() {
    return "ALUInput1ForwardingMux";
}
