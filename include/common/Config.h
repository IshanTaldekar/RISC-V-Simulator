#ifndef RISC_V_SIMULATOR_CONFIG_H
#define RISC_V_SIMULATOR_CONFIG_H

#include <variant>
#include <string>

enum class IFAdderInputType {
    PCValue = 0
};

enum class EXAdderInputType {
    PCValue = 0,
    ImmediateValue
};

using AdderInputType = std::variant<IFAdderInputType, EXAdderInputType>;

enum class IFStageMuxInputType {
    IncrementedPc = 0,
    BranchedPc
};

enum class EXStageMuxInputType {

};

enum class WBStageMuxInputType {

};

/**
 * StageMuxInputType can hold either the value of IFStageMuxInputType, EXStageMuxInputType,
 * WBStageMuxInputType.
 * */
using StageMuxInputType = std::variant<IFStageMuxInputType, EXStageMuxInputType, WBStageMuxInputType>;

#endif //RISC_V_SIMULATOR_CONFIG_H
