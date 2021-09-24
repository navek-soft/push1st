#pragma once
#include "cinet.h"

namespace inet {
	class cpoll;
	class cserver
	{
	public:
		cserver(const std::string& name) : SrvName{ name } { ; }
		virtual ~cserver() { ; }
		virtual void Listen(const std::shared_ptr<cpoll>& poll) = 0;
	protected:
		virtual std::shared_ptr<cserver> GetInstance() = 0;
		virtual void OnAccept(fd_t, uint, std::weak_ptr<cpoll> poll) = 0;
		virtual void OnAcceptSSL(fd_t, uint, std::weak_ptr<cpoll> poll) = 0;
		inline const char* NameOf() { return SrvName.c_str(); }
	protected:
		std::string SrvName;
	};
}

using server_t = inet::cserver;