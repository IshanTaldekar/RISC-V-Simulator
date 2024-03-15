#include "../../include/common/BitwiseOperations.h"

std::bitset<WORD_BIT_COUNT> BitwiseOperations::addInputs(std::bitset<WORD_BIT_COUNT> value1,
                                                         std::bitset<WORD_BIT_COUNT> value2) {
    bool carry = false;
    std::bitset<WORD_BIT_COUNT> result;

    for (int i = 0; i < WORD_BIT_COUNT; ++i) {
        bool input_1_bit = value1.test(i);
        bool input_2_bit = value2.test(i);

        result[i] = input_1_bit ^ input_2_bit ^ carry;
        carry = (input_1_bit && input_2_bit) || (carry && (input_1_bit ^ input_2_bit));
    }

    return result;
}

std::bitset<WORD_BIT_COUNT> BitwiseOperations::subtractInputs(std::bitset<WORD_BIT_COUNT> value1,
                                                              std::bitset<WORD_BIT_COUNT> value2) {
    std::bitset<WORD_BIT_COUNT> inverted_input_2 = ~value2;
    std::bitset<WORD_BIT_COUNT> one;
    one.set(0);

    std::bitset<WORD_BIT_COUNT> complement_input_2 = BitwiseOperations::addInputs(inverted_input_2, one);
    return addInputs(value1, complement_input_2);
}

std::bitset<WORD_BIT_COUNT> BitwiseOperations::bitwiseXorInputs(std::bitset<WORD_BIT_COUNT> value1,
                                                                std::bitset<WORD_BIT_COUNT> value2) {
    return value1 ^ value2;
}

std::bitset<WORD_BIT_COUNT> BitwiseOperations::bitwiseOrInputs(std::bitset<WORD_BIT_COUNT> value1,
                                                               std::bitset<WORD_BIT_COUNT> value2) {
    return value1 | value2;
}

std::bitset<WORD_BIT_COUNT> BitwiseOperations::bitwiseAndInputs(std::bitset<WORD_BIT_COUNT> value1,
                                                                std::bitset<WORD_BIT_COUNT> value2) {
    return value1 & value2;
}