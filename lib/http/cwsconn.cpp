#include "cwsconn.h"

using namespace inet;

void cwsconnection::WsReadMessage(size_t maxMessageLength) {
	size_t nread{ 0 }, nlength{ 0 };
	ssize_t res{ -1 };

	if (websocket_t::frame_t packet; (res = WsRecv(&packet, sizeof(packet), nread, 0)) == 0) {
		websocket_t::extra_t extra;
		websocket_t::mask_t mask;
		std::shared_ptr<uint8_t[]> message;

		if (auto length = packet.len; length < 126) {
			nlength = length;
		}
		else if (length == 126) {
			if ((res = WsRecv(&extra, sizeof(websocket_t::extra_t::medium), nread, MSG_WAITALL)) == 0) {
				nlength = be16toh(extra.medium.len);
			}
			else {
				OnWsError(res); return;
			}
		}
		else if (length == 127) {
			if ((res = WsRecv(&extra, sizeof(websocket_t::extra_t::large), nread, MSG_WAITALL)) == 0) {
				nlength = be64toh(extra.large.len);
			}
			else {
				OnWsError(res); return;
			}
		}
		else {
			OnWsError(-EBADMSG); return;
		}

		if (packet.mask) {
			if ((res = WsRecv(&mask, sizeof(websocket_t::mask_t), nread, MSG_WAITALL)) != 0) {
				OnWsError(res); return;
			}
		}

		if (nlength) {
			if (nlength < maxMessageLength) {
				message = std::move(std::shared_ptr<uint8_t[]>{ new uint8_t[nlength] });
				if ((res = WsRecv(message.get(), nlength, nread, 0)) == 0) {
					if (packet.mask) { websocket_t::Mask((uint8_t*)message.get(), nlength, mask); }
				}
			}
			else {
				WsClose(websocket_t::close_t::MessageToBig);
				OnWsError(-EMSGSIZE); return;
			}
		}
		if (auto opcode{ (websocket_t::opcode_t)packet.opcode }; opcode == websocket_t::opcode_t::text or opcode == websocket_t::opcode_t::binary) {
			OnWsMessage((websocket_t::opcode_t)packet.opcode, std::move(message), nlength);  return;
		}
		else if (opcode == websocket_t::opcode_t::ping) {
			OnWsPing();  return;
		}
		else if (opcode == websocket_t::opcode_t::close) {
			OnWsClose();  return;
		}
		else {
			WsClose(websocket_t::close_t::UnsupportedData);
			OnWsError(-ENODATA); return;
		}
	}
	OnWsError(res); 
}


void cwsconnection::WsWriteMessage(websocket_t::opcode_t opcode, const std::string_view& data, bool masked) {
	if (auto&& message{ websocket_t::Make(opcode, data, masked) }; !message.empty()) {
		size_t nwrite{ 0 };
		if (auto res = WsSend(message.data(), message.size(), nwrite); res == 0) {
			return;
		}
		else {
			OnWsError(res); return;
		}
	}
	OnWsError(-ENOMEM); return;
}

void cwsconnection::OnWsClose() {
	WsClose(websocket_t::close_t::NormalClosure);
}

void cwsconnection::WsClose(websocket_t::close_t code) {
	uint16_t status{ htobe16((uint16_t)code) };
	WsWriteMessage(websocket_t::opcode_t::close, { (char*)&status,sizeof(status) });
}

void cwsconnection::OnWsPing() {
	WsWriteMessage(websocket_t::opcode_t::pong, {});
}