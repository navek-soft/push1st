#pragma once
#include "cwebsocket.h"
#include "chttp.h"
#include "../inet/csocket.h"

namespace inet {
	class cwsconnection : public http::cwebsocket
	{
	public:
		cwsconnection() = default;
		virtual ~cwsconnection() { ; }
	protected:
		virtual bool OnWsConnect(const http::uri_t& path, const http::headers_t& headers) = 0;
		virtual void OnWsError(ssize_t err) = 0;
		virtual void OnWsMessage(websocket_t::opcode_t opcode, const std::shared_ptr<uint8_t[]>& message, size_t length) { ; }
		virtual void OnWsPing();
		virtual void OnWsClose();
		virtual inline ssize_t WsRecv(void* data, size_t size, size_t& readed, uint flags = 0) = 0;
		virtual inline ssize_t WsSend(const void* data, size_t size, size_t& writen, uint flags = 0) = 0;
		virtual inline std::string WsSessionId() = 0;
		void WsClose(websocket_t::close_t code);
		void WsError(websocket_t::close_t code, ssize_t err);
		ssize_t WsReadMessage(size_t maxMessageLength);
		ssize_t WsWriteMessage(websocket_t::opcode_t opcode, std::string&& data, bool masked = false);
		ssize_t WsSendMessage(websocket_t::opcode_t opcode, std::string&& data, bool masked = false, uint flags = 0);
	};
}

