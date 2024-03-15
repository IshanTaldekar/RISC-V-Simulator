#ifndef RISC_V_SIMULATOR_HAZARDDETECTIONUNIT_H
#define RISC_V_SIMULATOR_HAZARDDETECTIONUNIT_H

#include "../common/Module.h"
#include "../common/Logger.h"
#include "../state/Driver.h"
#include "../state/stage-registers/IFIDStageRegisters.h"
#include "../state/stage-registers/IDEXStageRegisters.h"
#include "../common/StageSynchronizer.h"

class Logger;
class Driver;
class IFIDStageRegisters;
class IDEXStageRegisters;

class HazardDetectionUnit: public Module {
    unsigned long id_ex_register_destination;
    Instruction *instruction;
    bool is_id_ex_mem_read_asserted;
    bool is_hazard_detected_flag_asserted;

    bool is_id_ex_register_destination_set;
    bool is_id_ex_mem_read_set;
    bool is_instruction_set;

    bool is_reset_flag_set;
    bool is_pause_flag_set;

    static HazardDetectionUnit *current_instance;
    static std::mutex initialization_mutex;

    Logger *logger;
    Driver *driver;
    IFIDStageRegisters *if_id_stage_registers;
    IDEXStageRegisters *id_ex_stage_registers;

public:
    HazardDetectionUnit();

    static HazardDetectionUnit *init();

    void run() override;
    void reset();
    void pause();
    void resume();

    void setIDEXRegisterDestination(unsigned  long rd);
    void setInstruction(Instruction *new_instruction);
    void setIDEXMemRead(bool is_asserted);

private:
    void initDependencies() override;

    void passHazardDetectedFlagToDriver();
    void passHazardDetectedFlagToIFIDStageRegisters();
    void passHazardDetectedFlagToIDEXStageRegisters();

    void resetState();
};

#endif //RISC_V_SIMULATOR_HAZARDDETECTIONUNIT_H
