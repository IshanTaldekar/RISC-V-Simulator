#ifndef RISC_V_SIMULATOR_MEMLOGGER_H
#define RISC_V_SIMULATOR_MEMLOGGER_H

#include "LoggerBase.h"
#include "../Config.h"

class MEMLogger: protected LoggerBase {
    static MEMLogger *current_instance;

public:
    MEMLogger();
    static MEMLogger *init();
    void log(const std::string &text);
};

#endif //RISC_V_SIMULATOR_MEMLOGGER_H
