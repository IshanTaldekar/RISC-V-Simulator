#ifndef RISC_V_SIMULATOR_BITWISEOPERATIONS_H
#define RISC_V_SIMULATOR_BITWISEOPERATIONS_H

#include "Config.h"

class BitwiseOperations {
public:
    static std::bitset<WORD_BIT_COUNT> addInputs(std::bitset<WORD_BIT_COUNT> value1,
                                                 std::bitset<WORD_BIT_COUNT> value2);

    static std::bitset<WORD_BIT_COUNT> subtractInputs(std::bitset<WORD_BIT_COUNT> value1,
                                                      std::bitset<WORD_BIT_COUNT> value2);

    static std::bitset<WORD_BIT_COUNT> bitwiseXorInputs(std::bitset<WORD_BIT_COUNT> value1,
                                                        std::bitset<WORD_BIT_COUNT> value2);

    static std::bitset<WORD_BIT_COUNT> bitwiseOrInputs(std::bitset<WORD_BIT_COUNT> value1,
                                                       std::bitset<WORD_BIT_COUNT> value2);

    static std::bitset<WORD_BIT_COUNT> bitwiseAndInputs(std::bitset<WORD_BIT_COUNT> value1,
                                                        std::bitset<WORD_BIT_COUNT> value2);
};

#endif //RISC_V_SIMULATOR_BITWISEOPERATIONS_H
