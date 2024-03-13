#ifndef RISC_V_SIMULATOR_BITWISEOPERATIONS_H
#define RISC_V_SIMULATOR_BITWISEOPERATIONS_H

#include "Config.h"

class BitwiseOperations {
public:
    static std::bitset<WORD_BIT_COUNT> addInputs(const std::bitset<WORD_BIT_COUNT> &value1,
                                                 const std::bitset<WORD_BIT_COUNT> &value2);
    static std::bitset<WORD_BIT_COUNT> subtractInputs(const std::bitset<WORD_BIT_COUNT> &value1,
                                                      const std::bitset<WORD_BIT_COUNT> &value2);
    static std::bitset<WORD_BIT_COUNT> bitwiseXorInputs(const std::bitset<WORD_BIT_COUNT> &value1,
                                                        const std::bitset<WORD_BIT_COUNT> &value2);
    static std::bitset<WORD_BIT_COUNT> bitwiseOrInputs(const std::bitset<WORD_BIT_COUNT> &value1,
                                                       const std::bitset<WORD_BIT_COUNT> &value2);
    static std::bitset<WORD_BIT_COUNT> bitwiseAndInputs(const std::bitset<WORD_BIT_COUNT> &value1,
                                                        const std::bitset<WORD_BIT_COUNT> &value2);
};

#endif //RISC_V_SIMULATOR_BITWISEOPERATIONS_H
