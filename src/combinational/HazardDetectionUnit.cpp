#include "../../include/combinational/HazardDetectionUnit.h"

HazardDetectionUnit *HazardDetectionUnit::current_instance = nullptr;
std::mutex HazardDetectionUnit::initialization_mutex;

HazardDetectionUnit::HazardDetectionUnit() {
    this->id_ex_register_destination = 0UL;
    this->instruction = nullptr;
    this->is_id_ex_mem_read_asserted = false;
    this->is_hazard_detected_flag_asserted = false;

    this->is_id_ex_register_destination_set = false;
    this->is_id_ex_mem_read_set = false;
    this->is_instruction_set = false;
    this->is_pause_flag_set = false;
    this->is_reset_flag_set = false;

    this->logger = nullptr;
    this->driver = nullptr;
    this->if_id_stage_registers = nullptr;
    this->id_ex_stage_registers = nullptr;
}

void HazardDetectionUnit::pause() {
    this->log("Paused.");
    this->is_pause_flag_set = true;
}

void HazardDetectionUnit::resume() {
    this->log("Resumed.");
    this->is_pause_flag_set = false;
    this->notifyModuleConditionVariable();
}


void HazardDetectionUnit::initDependencies() {
    std::unique_lock<std::mutex> hazard_detection_unit_lock (this->getModuleMutex());

    if (this->logger && this->driver && this->if_id_stage_registers && this->id_ex_stage_registers) {
        return;
    }

    this->logger = Logger::init();
    this->driver = Driver::init();
    this->if_id_stage_registers = IFIDStageRegisters::init();
    this->id_ex_stage_registers = IDEXStageRegisters::init();
}

HazardDetectionUnit *HazardDetectionUnit::init() {
    std::lock_guard<std::mutex> hazard_detection_lock (initialization_mutex);

    if (HazardDetectionUnit::current_instance == nullptr) {
        HazardDetectionUnit::current_instance = new HazardDetectionUnit();
    }

    return HazardDetectionUnit::current_instance;
}

void HazardDetectionUnit::run() {
    this->initDependencies();

    while (this->isAlive()) {
        this->log("Waiting to be woken up and acquire lock.");

        std::unique_lock<std::mutex> hazard_detection_unit_lock (this->getModuleMutex());
        this->getModuleConditionVariable().wait(
                hazard_detection_unit_lock,
                [this] {
                    return ((this->is_instruction_set  &&
                        this->is_id_ex_mem_read_set && this->is_id_ex_register_destination_set) &&
                        !this->is_pause_flag_set) || this->is_reset_flag_set || this->isKilled();
                }
        );

        if (this->isKilled()) {
            this->log("Killed.");
            break;
        }

        if (this->is_reset_flag_set) {
            this->log("Resetting state.");

            this->resetState();
            this->is_reset_flag_set = false;

            this->log("Reset.");
            continue;
        }

        this->log("Woken up and acquired lock.");

        if (this->getPipelineType() == PipelineType::Five && this->is_id_ex_mem_read_asserted &&
            (this->instruction->getRs1().to_ulong() == this->id_ex_register_destination ||
            this->instruction->getRs2().to_ulong() == this->id_ex_register_destination) && !this->is_hazard_detected_flag_asserted) {
            this->is_hazard_detected_flag_asserted = true;
        } else {
            this->is_hazard_detected_flag_asserted = false;
        }

        std::thread pass_hazard_detected_driver_thread (&HazardDetectionUnit::passHazardDetectedFlagToDriver, this);
        std::thread pass_hazard_detected_if_id_thread (&HazardDetectionUnit::passHazardDetectedFlagToIFIDStageRegisters, this);
        std::thread pass_hazard_detected_id_ex_thread (&HazardDetectionUnit::passHazardDetectedFlagToIDEXStageRegisters, this);

        pass_hazard_detected_driver_thread.join();
        pass_hazard_detected_id_ex_thread.join();
        pass_hazard_detected_if_id_thread.join();

        this->is_id_ex_register_destination_set = false;
        this->is_id_ex_mem_read_set = false;
        this->is_instruction_set = false;
    }
}

void HazardDetectionUnit::setIDEXRegisterDestination(unsigned long rd) {
    this->log("setIDEXRegisterDestination waiting to acquire lock.");

    std::lock_guard<std::mutex> hazard_detection_unit_lock (this->getModuleMutex());

    this->log("setIDEXRegisterDestination acquired lock. Updating value.");

    this->id_ex_register_destination = rd;
    this->is_id_ex_register_destination_set = true;

    this->log("setIDEXRegisterDestination value updated.");
    this->notifyModuleConditionVariable();
}

void HazardDetectionUnit::setInstruction(Instruction *new_instruction) {
    if (!this->logger) {
        this->initDependencies();
    }

    this->log("setInstruction waiting to acquire lock.");

    std::lock_guard<std::mutex> hazard_detection_unit_lock (this->getModuleMutex());

    this->log("setInstruction acquired lock. Updating value.");

    this->instruction = new_instruction;
    this->is_instruction_set = true;

    this->log("setInstruction value updated.");
    this->notifyModuleConditionVariable();
}

void HazardDetectionUnit::setIDEXMemRead(bool is_asserted) {
    this->log("setIDEXMemRead waiting to acquire lock.");

    std::lock_guard<std::mutex> hazard_detection_unit_lock (this->getModuleMutex());

    this->log("setIDEXMemRead acquired lock. Updating value.");

    this->is_id_ex_mem_read_asserted = is_asserted;
    this->is_id_ex_mem_read_set = true;

    this->log("setIDEXMemRead value updated.");
    this->notifyModuleConditionVariable();
}

void HazardDetectionUnit::passHazardDetectedFlagToDriver() {
    this->log("Passing hazard detected flag to Driver.");
    this->driver->setNop(this->is_hazard_detected_flag_asserted);
    this->log("Passed hazard detected flag to Driver.");
}

void HazardDetectionUnit::passHazardDetectedFlagToIFIDStageRegisters() {
    this->log("Passing hazard detected flag to IFIDStageRegisters.");
    this->if_id_stage_registers->setNop(this->is_hazard_detected_flag_asserted);
    this->log("Passed hazard detected flag to IFIDStageRegisters.");
}

void HazardDetectionUnit::passHazardDetectedFlagToIDEXStageRegisters() {
    this->log("Passing hazard detected flag to IDEXStageRegisters.");
    this->id_ex_stage_registers->setNop(this->is_hazard_detected_flag_asserted);
    this->log("Passed hazard detected flag to IDEXStageRegisters.");
}

void HazardDetectionUnit::reset() {
    std::lock_guard<std::mutex> hazard_detection_lock (this->getModuleMutex());

    this->is_reset_flag_set = true;
    this->notifyModuleConditionVariable();
}

void HazardDetectionUnit::resetState() {
    this->is_id_ex_register_destination_set = false;
    this->is_id_ex_mem_read_set = false;
    this->is_instruction_set = false;

    this->is_id_ex_mem_read_asserted = false;
    this->instruction = new Instruction(std::string(32, '0'));
    this->id_ex_register_destination = 0UL;
}

std::string HazardDetectionUnit::getModuleTag() {
    return "HazardDetectionUnit";
}

Stage HazardDetectionUnit::getModuleStage() {
    return Stage::ID;
}