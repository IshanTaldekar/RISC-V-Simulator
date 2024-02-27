#ifndef RISC_V_SIMULATOR_WBLOGGER_H
#define RISC_V_SIMULATOR_WBLOGGER_H

#include "LoggerBase.h"
#include "../Config.h"

class WBLogger: protected LoggerBase {
    static WBLogger *current_instance;

public:
    WBLogger();
    static WBLogger *init();
    void log(const std::string &text);
};

#endif //RISC_V_SIMULATOR_WBLOGGER_H
