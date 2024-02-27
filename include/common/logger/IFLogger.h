#ifndef RISC_V_SIMULATOR_IFLOGGER_H
#define RISC_V_SIMULATOR_IFLOGGER_H

#include "LoggerBase.h"
#include "../Config.h"

class IFLogger: protected LoggerBase {
    static IFLogger *current_instance;

public:
    IFLogger();
    static IFLogger *init();
    void log(const std::string &text);
};

#endif //RISC_V_SIMULATOR_IFLOGGER_H
