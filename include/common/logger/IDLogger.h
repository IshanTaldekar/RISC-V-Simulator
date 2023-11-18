//
// Created by wasp on 10/31/23.
//

#ifndef RISC_V_SIMULATOR_IDLOGGER_H
#define RISC_V_SIMULATOR_IDLOGGER_H

#include "Logger.h"
#include "../Config.h"

class IDLogger: protected Logger {
    static IDLogger *current_instance;

public:
    IDLogger();
    static IDLogger *init();
    void log(const std::string &text);
};

#endif //RISC_V_SIMULATOR_IDLOGGER_H
