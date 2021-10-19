#include "cwsserver.h"

using namespace inet;

void cwsserver::WsData(fd_t fd, uint events, inet::socket_t so, const std::weak_ptr<inet::cpoll>& poll) {
	if (so) {
		if (events == EPOLLIN) {
			so->OnSocketRecv(); 
		}
		else if (events == EPOLLOUT) { 
			so->OnSocketSend(); 
		}
		else { 
			so->OnSocketError((events & (EPOLLERR | EPOLLERR)) ? -EPIPE : ((events & (EPOLLRDHUP)) ? -ESTRPIPE : -EREMOTEIO)); 
		}
	}
	else if (auto&& self{ poll.lock() }; self) {
		self->PollDelete(fd);
	}
}

ssize_t cwsserver::WsUpgrade(const inet::csocket& fd, const http::uri_t& path, const http::headers_t& headers) {
	return HttpWriteResponse(fd, "101", {}, {
			{"Upgrade","WebSocket"},
			{"Connection","Upgrade"},
			{"Sec-WebSocket-Accept", http::cwebsocket::SecKey(http::GetValue(headers,"sec-websocket-key"))},
//			{"Sec-WebSocket-Protocol", std::string{http::GetValue(headers,"sec-websocket-protocol","chat")}},
			{"Sec-WebSocket-Version","13"}
	});
}

ssize_t cwsserver::OnTcpAccept(fd_t fd, const sockaddr_storage& sa, const inet::ssl_t& ssl, const std::weak_ptr<inet::cpoll>& poll) {
	ssize_t res{ -1 };
	if (auto&& self{ poll.lock() }; self) {
		return self->PollAdd(fd, EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLHUP, std::bind(&cwsserver::WsAccept, this, std::placeholders::_1, std::placeholders::_2, sa, ssl, poll),true);
	}
	return res;
}


void cwsserver::WsAccept(fd_t fd, uint events, const sockaddr_storage& sa, const inet::ssl_t& ssl, const std::weak_ptr<inet::cpoll>& poll) {
	/* Read HTTP request */
	ssize_t res{ -1 };
	if (auto&& self{ poll.lock() }; self) {
		if (events == EPOLLIN) {
			std::string_view method, content; http::uri_t path; http::headers_t headers;  std::string request;
			inet::csocket so(fd, sa, ssl, poll);
			if (res = HttpReadRequest(so, method, path, headers, request, content, HttpMaxHeaderSize); res == 0) {
				if (method == "GET" and http::GetValue(headers, "connection") == "Upgrade" and
					http::GetValue(headers, "upgrade") == "websocket" and
					http::ToNumber(http::GetValue(headers, "sec-websocket-version")) >= 12)
				{
					if (res = WsUpgrade(so, path, headers); res == 0) {
						if (auto&& con = OnWsUpgrade(so, path, headers); con) {
							res = self->PollUpdate(fd, EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLHUP, std::bind(&cwsserver::WsData, this, std::placeholders::_1, std::placeholders::_2, con, poll));
							if (res != 0) {
								con->OnSocketError(res); /* Handler must close connection and free resources by itself */
							}
						}
					}
					return;
				}
				else {
					HttpWriteResponse(so, "426");
				}
			}
			else {
				HttpWriteResponse(so, "400");
			}
		}
		self->PollDelete(fd);
	}
	else {
		inet::Close(fd);
	}
}

cwsserver::cwsserver(const std::string& name, const std::string& HostPort, const inet::ssl_ctx_t& SslCtx, size_t httpMaxHeaderSize) :
	inet::ctcpserver{ name, false, 1500, 2000, true }, HttpMaxHeaderSize{ httpMaxHeaderSize }
{
	TcpListen(HostPort, true, true, true, 4096, SslCtx);
}

cwsserver::~cwsserver() {
	TcpClose();
}
