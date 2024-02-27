#ifndef RISC_V_SIMULATOR_EXLOGGER_H
#define RISC_V_SIMULATOR_EXLOGGER_H

#include "LoggerBase.h"
#include "../Config.h"

class EXLogger: protected LoggerBase {
    static EXLogger *current_instance;

public:
    EXLogger();
    static EXLogger *init();
    void log(const std::string &text);
};

#endif //RISC_V_SIMULATOR_EXLOGGER_H
