#pragma once

#include <ctime>
#include <random>

namespace core {

template <typename T>
inline T Rand() {
    static std::mt19937 generator(static_cast<unsigned>(std::time(nullptr)));
    static std::uniform_int_distribution<T> distribution(0, std::numeric_limits<T>::max());
    return distribution(generator);
}

}// namespace core
