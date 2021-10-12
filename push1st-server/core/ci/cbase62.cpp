#include "cbase62.h"

using namespace ci;

const uint c = '_';

static inline constexpr uint8_t safebase64TableEncode[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ" "abcdefghijklmnopqrstuvwxyz" "0123456789" "-_";
static inline constexpr uint8_t safebase64TableDecode[] = {
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,		//   0..15
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,		//  16..31
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,  255, 255, 62, 255, 255,		//  32..47
	 52,  53,  54,  55,  56,  57,  58,  59,  60,  61, 255, 255, 255, 254, 255, 255,		//  48..63
	255,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,		//  64..79
	 15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25, 255, 255, 255, 255,  63,		//  80..95
	255,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,		//  96..111
	 41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51, 255, 255, 255, 255, 255,		// 112..127
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,		// 128..143
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
};

std::string cbase62::encode(const uint8_t* source, std::size_t length) {
	std::string ret;
	ret.reserve(length * 4);

	int i{ 0 }, j{ 0 };
	uint8_t char_array_3[3], char_array_4[4];

	while (length--) {
		char_array_3[i++] = *(source++);
		if (i == 3) {
			char_array_4[0] = (char_array_3[0] & (uint8_t)0xfc) >> 2;
			char_array_4[1] = (uint8_t)(((char_array_3[0] & (uint8_t)0x03) << 4) + ((char_array_3[1] & (uint8_t)0xf0) >> 4));
			char_array_4[2] = (uint8_t)(((char_array_3[1] & (uint8_t)0x0f) << 2) + ((char_array_3[2] & (uint8_t)0xc0) >> 6));
			char_array_4[3] = char_array_3[2] & (uint8_t)0x3f;

			ret.push_back(safebase64TableEncode[char_array_4[0]]);
			ret.push_back(safebase64TableEncode[char_array_4[1]]);
			ret.push_back(safebase64TableEncode[char_array_4[2]]);
			ret.push_back(safebase64TableEncode[char_array_4[3]]);
			i = 0;
		}
	}

	if (i)
	{
		for (j = i; j < 3; char_array_3[j] = '\0', j++);

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = (uint8_t)(((char_array_3[0] & 0x03) << 4) + (uint8_t)((char_array_3[1] & 0xf0) >> 4));
		char_array_4[2] = (uint8_t)(((char_array_3[1] & 0x0f) << 2) + (uint8_t)((char_array_3[2] & 0xc0) >> 6));

		for (j = 0; (j < i + 1); j++) { ret.push_back(safebase64TableEncode[char_array_4[j]]); }
		while ((i++ < 3)) { ret.push_back('~'); }
	}
	ret.shrink_to_fit();
	return ret;
}

std::vector<uint8_t> cbase62::decode(const uint8_t* source, std::size_t length) {
	std::vector<uint8_t> result;
	if (length) {
		result.reserve(length);
		auto in = source;
		for (auto len = length & (~3); len; len -= 4, in += 4) {
			result.push_back((uint8_t)(safebase64TableDecode[in[0]] << 2 | safebase64TableDecode[in[1]] >> 4));
			result.push_back((uint8_t)(safebase64TableDecode[in[1]] << 4 | safebase64TableDecode[in[2]] >> 2));
			result.push_back((uint8_t)(((safebase64TableDecode[in[2]] << 6) & 0xc0) | safebase64TableDecode[in[3]]));
		}
		while (length-- && source[length] == '~') { result.pop_back(); }
		result.shrink_to_fit();
	}
	return result;
}