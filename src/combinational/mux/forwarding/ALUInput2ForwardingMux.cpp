#include "../../../../include/combinational/mux/forwarding/ALUInput2ForwardingMux.h"

ALUInput2ForwardingMux *ALUInput2ForwardingMux::current_instance = nullptr;

ALUInput2ForwardingMux *ALUInput2ForwardingMux::init() {
    if (ALUInput2ForwardingMux::current_instance == nullptr) {
        ALUInput2ForwardingMux::current_instance = new ALUInput2ForwardingMux();
    }

    return ALUInput2ForwardingMux::current_instance;
}

void ALUInput2ForwardingMux::passOutput() {
    if (this->control_signal == ALUInputMuxControlSignals::IDEXStageRegisters) {
        this->alu->setInput2(this->id_ex_stage_registers_value);
    } else if (this->control_signal == ALUInputMuxControlSignals::EXMEMStageRegisters) {
        this->alu->setInput2(this->ex_mem_stage_registers_value);
    } else if (this->control_signal == ALUInputMuxControlSignals::MEMWBStageRegisters) {
        this->alu->setInput2(this->mem_wb_stage_registers_value);
    } else {
        throw std::runtime_error("[ALUInput2ForwardingMux] control signal type did not match any existing type.");
    }
}

std::string ALUInput2ForwardingMux::getModuleTag() {
    return "ALUInput2ForwardingMux";
}
