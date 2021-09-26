#pragma once
#include "cwebsocket.h"
#include "../inet/csocket.h"

namespace inet {
	class cwsconnection : public http::cwebsocket
	{
	public:
		cwsconnection() = default;
		virtual ~cwsconnection() { printf("%s\n", __PRETTY_FUNCTION__); }
	protected:
		virtual void OnWsError(ssize_t err) = 0;
		virtual void OnWsMessage(websocket_t::opcode_t opcode, std::shared_ptr<uint8_t[]>&& message, size_t length) { ; }
		virtual void OnWsPing();
		virtual void OnWsClose();
		virtual inline ssize_t WsRecv(void* data, size_t size, size_t& readed, uint flags = 0) = 0;
		virtual inline ssize_t WsSend(const void* data, size_t size, size_t& writen, uint flags = 0) = 0;
		void WsClose(websocket_t::close_t code);
		void WsReadMessage(size_t maxMessageLength);
		void WsWriteMessage(websocket_t::opcode_t opcode, const std::string_view& data, bool masked = false);
	};
}

