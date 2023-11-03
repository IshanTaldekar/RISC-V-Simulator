#ifndef RISC_V_SIMULATOR_LOGGER_H
#define RISC_V_SIMULATOR_LOGGER_H

#include <string>
#include <mutex>
#include <fstream>
#include <iostream>

class Logger {
protected:
    std::string log_file_path;
    std::ofstream log_file;
    std::mutex log_file_mutex;

    void openFile();
};

#endif //RISC_V_SIMULATOR_LOGGER_H
