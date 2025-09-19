#pragma once
#include <string>

namespace core::ci {
template <typename ENUM_T, typename FLAG_T = uint>
class cflag {
   public:
    using type = ENUM_T;

    cflag(ENUM_T flag_set = (ENUM_T)0) : flagset {(FLAG_T)flag_set} {
        ;
    }
    cflag(FLAG_T flag_set) : flagset {flag_set} {
        ;
    }
    cflag(const cflag& val) : flagset {val.flagset} {
        ;
    }
    template <typename Integer, typename = std::enable_if_t<std::is_integral<Integer>::value>>
    cflag& operator=(Integer val) {
        flagset = (FLAG_T)val;
        return *this;
    }

    cflag& operator=(const cflag&) = default;
    inline operator FLAG_T() const {
        return flagset;
    }
    inline operator ENUM_T() const {
        return (ENUM_T)flagset;
    }

    inline bool Empty() const {
        return !flagset;
    }
    inline bool operator==(FLAG_T flag) const {
        return flagset == flag;
    }
    inline bool operator!=(FLAG_T flag) const {
        return flagset != flag;
    }
    inline bool operator==(ENUM_T flag) const {
        return flagset == (FLAG_T)flag;
    }
    inline bool operator!=(ENUM_T flag) const {
        return flagset != (FLAG_T)flag;
    }

    inline FLAG_T operator|(ENUM_T flag) const {
        return flagset | (FLAG_T)flag;
    }
    inline FLAG_T operator&(ENUM_T flag) const {
        return flagset & (FLAG_T)flag;
    }

    template <typename T, typename = std::enable_if_t<std::is_integral<T>::value || std::is_enum<T>::value>>
    inline auto operator|=(T flag) {
        flagset |= (FLAG_T)flag;
        return *this;
    }

    template <typename T, typename = std::enable_if_t<std::is_integral<T>::value || std::is_enum<T>::value>>
    inline auto operator+=(T flag) {
        flagset |= (FLAG_T)flag;
        return *this;
    }

    template <typename T, typename = std::enable_if_t<std::is_integral<T>::value || std::is_enum<T>::value>>
    inline auto operator-=(T flag) {
        flagset &= (~(FLAG_T)flag);
        return *this;
    }

   private:
    FLAG_T flagset {0};
};
}// namespace core::ci

// #if defined(__clang__)
//     #pragma clang diagnostic push
//     #pragma clang diagnostic ignored "-Werror"
//     #pragma clang diagnostic ignored "-Weverything" // -Woverloaded-operator
// #elif defined(__GNUC__)
//     #pragma GCC diagnostic push
//     #pragma GCC diagnostic ignored "-Wpragmas"
//     #pragma GCC diagnostic ignored "-Werror"
// #endif

// template <typename E_T, typename = std::enable_if_t<std::is_integral<E_T>::value || std::is_enum<E_T>::value>>
// constexpr static uint operator | (E_T f1, E_T f2) {
//     return (uint)f1 | (uint)f2;
// }
// template <typename E_T, typename = std::enable_if_t<std::is_integral<E_T>::value || std::is_enum<E_T>::value>>
// constexpr static uint operator | (uint f1, E_T f2) {
//     return (uint)f1 | (uint)f2;
// }

// #if defined(__clang__)
//     #pragma clang diagnostic pop
// #elif defined(__GNUC__)
//     #pragma GCC diagnostic pop
// #endif

template <typename E_T, typename = std::enable_if_t<std::is_enum<E_T>::value>>
constexpr static unsigned operator|(E_T f1, E_T f2) {
    return static_cast<unsigned>(f1) | static_cast<unsigned>(f2);
}

template <typename E_T, typename = std::enable_if_t<std::is_enum<E_T>::value>>
constexpr static unsigned operator|(unsigned f1, E_T f2) {
    return f1 | static_cast<unsigned>(f2);
}

template <typename ET>
using flag_t = core::ci::cflag<ET>;