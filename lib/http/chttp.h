#pragma once
#include <string>
#include <unordered_map>
#include <vector>

namespace http {
	enum auth_t { none = 0, basic = 1, digest = 2, token = 3, disable = 4 };
	using headers_t = std::unordered_map<std::string_view, std::string_view>;

	class curi {
		using params_t = std::unordered_multimap<std::string_view, std::string_view>;
	public:
		curi() { ; }
		//curi(const curi& u) : uriFull{ u.uriFull }, uriPath{ u.uriPath }, uriPathList{ u.uriPathList } {; }
		inline std::string_view at(size_t idx) const { return uriPathList[idx]; }
		inline std::string_view uri() const { return uriFull; }
		inline std::string_view path() const { return uriPath; }
		inline std::string_view arg(const std::string_view& name) const { return uriArgs.find(name)->second; }
	public:
		std::string_view uriFull, uriPath;
		std::vector<std::string_view> uriPathList;
		params_t uriArgs;
	};

	using uri_t = curi;

	class cauthchallenge
	{
	public:
		cauthchallenge(auth_t type, const std::string& realm, const std::string& user = {}, const std::string& pwd = {});
		cauthchallenge() = default;
		~cauthchallenge() = default;

		inline void Reset() { Stale = false; Type = auth_t::none; Realm = Domain = Nonce = Opaque = Qop = User = Pwd = Response = {}; Algorithm = "md5"; }
		void SetFromHeader(std::string_view authenticate_header);

		virtual std::string GetAuthorizationHeader(const std::string_view& user, const std::string_view& pwd, const std::string_view& method, const std::string_view& uri);
		virtual std::pair<std::string, std::string> GetAuthenticateHeader(const std::string& nonce);

		std::string Digest(const std::string_view& user, const std::string_view& pwd, const std::string_view& method, const std::string_view& uri);

		virtual bool Authorize(const std::string_view& authdata, const std::string_view& method, const std::string_view& uri);

		inline bool Empty() { return Type == auth_t::none; }

		std::string GenNonce(size_t length);
	private:
		void FromDigestHeader(const std::string_view& authenticate_header);
		bool Stale{ false };
		auth_t Type{ auth_t::none };
		std::string Realm, Domain, Nonce, Opaque, Qop, Algorithm, User, Pwd, Response;
	};


	ssize_t ParseRequest(std::string_view stream, std::string_view& method, uri_t& uri, headers_t& headers, std::string_view& content, size_t& contentLength);
	static inline ssize_t ParseRequest(std::string_view stream, std::string_view& method, uri_t& uri, headers_t& headers, std::string_view& content) {
		size_t contentLength{ 0 };
		return ParseRequest(stream, method, uri, headers, content, contentLength);
	}
	ssize_t ParseResponse(std::string_view stream, std::string_view& code, std::string_view& msg, headers_t& headers, size_t& content_length, std::string_view& content);
	template<typename CONTAINER_T>
	inline std::string_view GetValue(const CONTAINER_T& headers, const std::string_view& header) { if (auto&& hIt{ headers.find(header) }; hIt != headers.end()) return hIt->second; return { "" }; }
	template<typename CONTAINER_T>
	inline std::string_view GetValue(const CONTAINER_T& headers, const std::string_view& header, const std::string_view& def) { if (auto&& hIt{ headers.find(header) }; hIt != headers.end()) return hIt->second; return def; }

	std::string RangeHeader(ssize_t from, ssize_t to, size_t ContentLength, bool withoutLength = false);
	bool IsGetRange(const headers_t& headers, ssize_t& from, ssize_t& to);

	std::string_view& TrimBlank(std::string_view& str);
	std::string Md5(const std::string& value);
	size_t ToNumber(std::string_view value);
	std::string Unquote(const std::string_view& value);
	std::string ToBase64(const std::string_view& value);
	std::string FromBase64(const std::string_view& value);

	std::string_view GetMessage(std::string_view code);
}