#include "../../include/common/BitwiseOperations.h"

std::bitset<WORD_BIT_COUNT> BitwiseOperations::addInputs(const std::bitset<WORD_BIT_COUNT> &input1,
                                                         const std::bitset<WORD_BIT_COUNT> &input2) {
    bool carry = false;
    std::bitset<WORD_BIT_COUNT> result;

    for (int i = 0; i < WORD_BIT_COUNT; ++i) {
        bool input_1_bit = input1.test(i);
        bool input_2_bit = input2.test(i);

        result[i] = input_1_bit ^ input_2_bit ^ carry;
        carry = (input_1_bit && input_2_bit) || (carry && (input_1_bit ^ input_2_bit));
    }

    return result;
}

std::bitset<WORD_BIT_COUNT> BitwiseOperations::subtractInputs(const std::bitset<WORD_BIT_COUNT> &input1,
                                                              const std::bitset<WORD_BIT_COUNT> &input2) {
    std::bitset<WORD_BIT_COUNT> inverted_input_2 = ~input2;
    std::bitset<WORD_BIT_COUNT> one;
    one.set(0);

    std::bitset<WORD_BIT_COUNT> complement_input_2 = BitwiseOperations::addInputs(inverted_input_2, one);
    return addInputs(input1, complement_input_2);
}

std::bitset<WORD_BIT_COUNT> BitwiseOperations::bitwiseXorInputs(const std::bitset<WORD_BIT_COUNT> &input1,
                                                  const std::bitset<WORD_BIT_COUNT> &input2) {
    return input1 ^ input2;
}

std::bitset<WORD_BIT_COUNT> BitwiseOperations::bitwiseOrInputs(const std::bitset<WORD_BIT_COUNT> &input1,
                                                 const std::bitset<WORD_BIT_COUNT> &input2) {
    return input1 | input2;
}

std::bitset<WORD_BIT_COUNT> BitwiseOperations::bitwiseAndInputs(const std::bitset<WORD_BIT_COUNT> &input1,
                                                  const std::bitset<WORD_BIT_COUNT> &input2) {
    return input1 & input2;
}