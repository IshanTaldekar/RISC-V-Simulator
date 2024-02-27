#include "../../../../include/combinational/mux/forwarding/ALUInput1ForwardingMux.h"

ALUInput1ForwardingMux *ALUInput1ForwardingMux::current_instance = nullptr;


ALUInput1ForwardingMux *ALUInput1ForwardingMux::init() {
    if (ALUInput1ForwardingMux::current_instance == nullptr) {
        ALUInput1ForwardingMux::current_instance = new ALUInput1ForwardingMux();
    }

    return ALUInput1ForwardingMux::current_instance;
}

void ALUInput1ForwardingMux::passOutput() {
    if (this->control_signal == ALUInputMuxControlSignals::IDEXStageRegisters) {
        this->alu->setInput1(this->id_ex_stage_registers_value);
    } else if (this->control_signal == ALUInputMuxControlSignals::EXMEMStageRegisters) {
        this->alu->setInput1(this->ex_mem_stage_registers_value);
    } else if (this->control_signal == ALUInputMuxControlSignals::MEMWBStageRegisters) {
        this->alu->setInput1(this->mem_wb_stage_registers_value);
    } else {
        throw std::runtime_error("[ALUInput1ForwardingMux] control signal type did not match any existing type.");
    }
}

std::string ALUInput1ForwardingMux::getModuleTag() {
    return "ALUInput1ForwardingMux";
}
