#include "cwsrawserver.h"
#include "../http/cwsconn.h"
#include "../../core/csyslog.h"

class cwsrawconnection : public inet::cwsconnection, public inet::csocket, public std::enable_shared_from_this<cwsrawconnection>
{
public:
	cwsrawconnection(const inet::csocket& fd, const http::path_t& path, const http::params_t& args, const http::headers_t& headers, size_t maxMessageLength);
	virtual ~cwsrawconnection();
protected:
	virtual inline void OnWsError(ssize_t err) override { OnSocketError(err); }
	virtual inline ssize_t WsRecv(void* data, size_t size, size_t& readed, uint flags = 0) override { return SocketRecv(data, size, readed, flags); }
	virtual inline ssize_t WsSend(const void* data, size_t size, size_t& writen, uint flags = 0) override { return SocketSend(data, size, writen, flags); }

	virtual inline void OnSocketRecv() override;
	virtual inline void OnSocketSend() override;
	virtual inline void OnSocketError(ssize_t err) override;
	virtual inline void OnWsMessage(websocket_t::opcode_t opcode, std::shared_ptr<uint8_t[]>&& message, size_t length) override;
	virtual inline void OnWsClose() override;
	virtual inline void OnWsPing() override;
private:
	size_t MaxMessageLength{ 65536 };
};

inline void cwsrawconnection::OnWsMessage(websocket_t::opcode_t opcode, std::shared_ptr<uint8_t[]>&& message, size_t length) {
	printf("msg");
}

inline void cwsrawconnection::OnWsClose() {
	printf("OnWsClose\n");
	SocketClose();
}

inline void cwsrawconnection::OnWsPing() {
	printf("OnWsPing\n");
}


inline void cwsrawconnection::OnSocketRecv() {
	WsReadMessage(MaxMessageLength);
}

inline void cwsrawconnection::OnSocketSend() {

}

inline void cwsrawconnection::OnSocketError(ssize_t err) {
	syslog.print(1, "%s ( %s )\n", __PRETTY_FUNCTION__, std::strerror((int)-err));
	SocketClose();
}


cwsrawconnection::cwsrawconnection(const inet::csocket& fd, const http::path_t& path, const http::params_t& args, const http::headers_t& headers, size_t maxMessageLength) :
	inet::csocket{ std::move(fd) }, MaxMessageLength{ maxMessageLength }
{
	syslog.print(1, "%s\n", __PRETTY_FUNCTION__);
}

cwsrawconnection::~cwsrawconnection() {
	syslog.print(1, "%s\n", __PRETTY_FUNCTION__);
}

inet::socket_t cwsrawserver::OnWsUpgrade(const inet::csocket& fd, const http::path_t& path, const http::params_t& args, const http::headers_t& headers, std::string&& request, std::string&& content) {
	return std::dynamic_pointer_cast<inet::csocket>(std::make_shared<cwsrawconnection>(fd,path,args, headers, MaxMessageLength));
}

cwsrawserver::cwsrawserver(const config::server_t& config) :
	inet::cwsserver{ "ws:srv", std::string{ config.Listen.hostport() }, config.Ssl.Context(), 8192 },
	MaxMessageLength{ config.MaxPayloadSize }
{
	syslog.ob.print("websocket.raw", "...enable %s proto, listen on %s, path %s", config.Ssl.Enable ? "https" : "http", std::string{config.Listen.hostport()}.c_str(), config.Path.c_str());
}

cwsrawserver::~cwsrawserver() {

}
