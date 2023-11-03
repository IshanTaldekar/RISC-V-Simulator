#ifndef RISC_V_SIMULATOR_MEMLOGGER_H
#define RISC_V_SIMULATOR_MEMLOGGER_H

#include "Logger.h"
#include "../Config.h"

class MEMLogger: protected Logger {
    static MEMLogger *current_instance;
public:
    MEMLogger();
    static MEMLogger *init();
    void log(const std::string &text);
};

MEMLogger *MEMLogger::current_instance = nullptr;

#endif //RISC_V_SIMULATOR_MEMLOGGER_H
