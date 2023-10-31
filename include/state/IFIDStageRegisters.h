#ifndef RISC_V_SIMULATOR_IFIDSTAGEREGISTERS_H
#define RISC_V_SIMULATOR_IFIDSTAGEREGISTERS_H

#include "../common/Config.h"
#include "../common/Module.h"

#include <bitset>
#include <iostream>
#include <queue>

class IFIDStageRegisters: public Module {
    static constexpr int WORD_BIT_COUNT = 32;

    int program_counter;
    std::string instruction;

public:
    IFIDStageRegisters();

    static IFIDStageRegisters *current_instance;
    static IFIDStageRegisters *init();

    void run() override;
    void notifyConditionVariable() override;

    void setInput(std::variant<int, std::string> input);
};

#endif //RISC_V_SIMULATOR_IFIDSTAGEREGISTERS_H
