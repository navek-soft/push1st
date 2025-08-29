#pragma once
#include <cstdint>
#include <string>
#include <cstdlib>
#include <memory>
#include <vector>

namespace http {
	class cwebsocket {
	public:

		using msg_t = std::vector<uint8_t>;

		enum class opcode_t : uint8_t {
			more = 0x0, text = 0x1, binary = 0x2, close = 0x8, ping = 0x9, pong = 0xa
		};

		enum class close_t : uint16_t {
			NormalClosure = 1000, GoingAway = 1001, ProtoError = 1002, UnsupportedData = 1003,
			NoStatusReceived = 1005, AbnormalClosure = 1006, InvalidPayload = 1007,
			PolicyViolation = 1008, MessageToBig = 1009, MissingExtension = 1010,
			InternalError = 1011, ServiceRestart = 1012, TryAgainLater = 1013,
			BadGateway = 1014, TLSHandshake = 1015
		};

	public:
#pragma pack(push,1)
		union extra_t
		{
			struct small {
			} small;
			struct medium {
				uint16_t len;
			} medium;
			struct large {
				uint64_t len;
			} large;
		};
		using mask_t = uint8_t[4];

		struct frame_t {
			uint8_t opcode : 4;
			uint8_t rsv3 : 1;
			uint8_t rsv2 : 1;
			uint8_t rsv1 : 1;
			uint8_t fin : 1;
			uint8_t len : 7;
			uint8_t mask : 1;
			frame_t() = default;
			frame_t(cwebsocket::opcode_t op, size_t length, bool have_mask = true, bool is_fin = true) :
				opcode{ (uint8_t)((int)op & 0xf) }, rsv3{ 0 }, rsv2{ 0 }, rsv1{ 0 }, fin{ is_fin }, len{ (uint8_t)(length & 0x7f) }, mask{ have_mask }
			{}
		};

		struct small_frame_t : frame_t {
			mask_t key; uint8_t payload[0];
			small_frame_t(cwebsocket::opcode_t op, size_t length, bool is_fin = true) : frame_t{ op,length,true,is_fin } { ; }
		};
		struct small_un_frame_t : frame_t {
			uint8_t payload[0];
			small_un_frame_t(cwebsocket::opcode_t op, size_t length, bool is_fin = true) : frame_t{ op,length,false,is_fin } { ; }
		};
		struct medium_frame_t : frame_t {
			uint16_t elen; mask_t key; uint8_t payload[0];
			medium_frame_t(cwebsocket::opcode_t op, size_t length, bool is_fin = true) : frame_t{ op,126,true,is_fin }, elen{ htobe16((uint16_t)length) } {; }
		};
		struct medium_un_frame_t : frame_t {
			uint16_t elen; uint8_t payload[0];
			medium_un_frame_t(cwebsocket::opcode_t op, size_t length, bool is_fin = true) : frame_t{ op,126,false,is_fin }, elen{ htobe16((uint16_t)length) } {; }
		};
		struct large_frame_t : frame_t {
			uint64_t elen; mask_t key; uint8_t payload[0];
			large_frame_t(cwebsocket::opcode_t op, size_t length, bool is_fin = true) : frame_t{ op,127,true,is_fin }, elen{ htobe64((uint64_t)length) } {; }
		};
		struct large_un_frame_t : frame_t {
			uint64_t elen; uint8_t payload[0];
			large_un_frame_t(cwebsocket::opcode_t op, size_t length, bool is_fin = true) : frame_t{ op,127,false,is_fin }, elen{ htobe64((uint64_t)length) } {; }
		};
#pragma pack(pop)
	public:
		virtual ~cwebsocket() = default;
		static std::string SecKey(const std::string_view& secKey);
		static void Mask(uint8_t* data, size_t length, const uint8_t* mask);
		static inline void RandMask(mask_t& mask);
		static msg_t Make(cwebsocket::opcode_t opcode, const std::string_view& message, bool masked = false);
	};
}

using websocket_t = http::cwebsocket;