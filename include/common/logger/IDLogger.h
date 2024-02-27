#ifndef RISC_V_SIMULATOR_IDLOGGER_H
#define RISC_V_SIMULATOR_IDLOGGER_H

#include "LoggerBase.h"
#include "../Config.h"

class IDLogger: protected LoggerBase {
    static IDLogger *current_instance;

public:
    IDLogger();
    static IDLogger *init();
    void log(const std::string &text);
};

#endif //RISC_V_SIMULATOR_IDLOGGER_H
