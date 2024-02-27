#include "../../../include/combinational/mux/MuxBase.h"

void MuxBase::assertControlSignal(bool is_asserted) {
    this->is_control_signal_set = true;
}