#pragma once
#include <string>

namespace core::ci {
	template<typename ENUM_T, typename FLAG_T = uint>
	class cflag {
	public:
		using type = ENUM_T;

		cflag(ENUM_T flag_set = 0) : flagset{ (FLAG_T)flag_set } { ; }
		cflag(FLAG_T flag_set) : flagset{ flag_set } { ; }
		cflag(const cflag& val) : flagset{ val.flagset } { ; }
		template<typename Integer, typename = std::enable_if_t<std::is_integral<Integer>::value>>
		cflag& operator = (Integer val) { flagset = (FLAG_T)val; return *this; }
		inline operator FLAG_T() const { return flagset; }

		inline bool empty () const { return !flagset; }
		inline bool operator == (FLAG_T flag) const { return flagset == flag; }
		inline bool operator != (FLAG_T flag) const { return flagset != flag; }

		inline FLAG_T operator | (ENUM_T flag) const { return flagset | (FLAG_T)flag; }
		inline FLAG_T operator & (ENUM_T flag) const { return flagset & (FLAG_T)flag; }
		
		template<typename T, typename = std::enable_if_t<std::is_integral<T>::value || std::is_enum<T>::value>>
		inline auto operator |= (T flag) { flagset |= (FLAG_T)flag; return *this; }
		

	private:
		FLAG_T flagset{ 0 };
	};
}

template<typename E_T, typename = std::enable_if_t<std::is_integral<E_T>::value || std::is_enum<E_T>::value>>
constexpr static uint operator | (E_T f1, E_T f2) { return (uint)f1 | (uint)f2; }
template<typename E_T, typename = std::enable_if_t<std::is_integral<E_T>::value || std::is_enum<E_T>::value>>
constexpr static uint operator | (uint f1, E_T f2) { return (uint)f1 | (uint)f2; }

template<typename ET>
using flag_t = core::ci::cflag<ET>;