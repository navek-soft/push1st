#include "chttp.h"
#include <regex>

using namespace http;

std::string http::RangeHeader(ssize_t from, ssize_t to, size_t ContentLength, bool withoutLength) {
	if (to == 0) {
		to = ContentLength - 1;
	}
	else if (from < 0) {
		from = ContentLength + from;
		to = ContentLength - from;
	}

	to = to - from + 1;

	return "bytes " + std::to_string(from) + "-" + std::to_string(from + to - 1) + "/" + (!withoutLength ? std::to_string(ContentLength) : "*");
}

bool http::IsGetRange(const headers_t& headers, ssize_t& from, ssize_t& to) {
	if (auto&& hIt{ headers.find("range") }; hIt != headers.end()) {
		std::string_view range{ hIt->second };
		if (range.compare(0, 6, "bytes=") == 0) {
			char ch;
			from = 0; to = -1;
			range.remove_prefix(6);
			if (range.front() != '-') /* bytes=1-9,... */ {
				while (!range.empty()) { ch = range.front(); if (std::isdigit(ch)) { from = (from * 10) + (ch - '0'); range.remove_prefix(1); } else if (ch == '-') { range.remove_prefix(1); break; } else { return false; } }
				while (!range.empty()) { ch = range.front(); if (std::isdigit(ch)) { to = (to * 10) + (ch - '0'); range.remove_prefix(1); } else if (ch == ',') { range.remove_prefix(1); break; } else { return false; } }
			}
			else /* bytes=-2 get last 2 bytes*/ {
				range.remove_prefix(1);
				while (!range.empty()) { ch = range.front(); if (std::isdigit(ch)) { from = (from * 10) + (ch - '0'); range.remove_prefix(1); } else if (ch == ',') { range.remove_prefix(1); break; } else { return false; } }
			}
			return true;
		}
	}
	return false;
}

bool cauthchallenge::Authorize(const std::string_view& authdata, const std::string_view& method, const std::string_view& uri) {
	switch (Type) {
	case auth_t::none:
		return true;
	case auth_t::basic:
		if (!authdata.empty() and strncasecmp(authdata.data(), "Basic ", 6) == 0) {
			return http::FromBase64(authdata.substr(6)).compare(User + ":" + Pwd) == 0;
		}
		break;
	case auth_t::digest:
		if (!authdata.empty() and strncasecmp(authdata.data(), "Digest ", 7) == 0) {
			FromDigestHeader(authdata);
			return !Response.empty() and Digest(User, Pwd, method, uri) == Response;
		}
		break;
	default:
		break;
	}
	return false;
}

std::pair<std::string, std::string> cauthchallenge::GetAuthenticateHeader(const std::string& nonce) {
	switch (Type) {
	case auth_t::basic:
		return { {"WWW-Authenticate"},{"Basic realm=" + Realm} };
	case auth_t::digest:
		return { {"WWW-Authenticate"},{"Digest realm=" + Realm + ", nonce=" + nonce} };
	default:
		break;
	}
	return { {},{} };
}

std::string cauthchallenge::Digest(const std::string_view& user, const std::string_view& pwd, const std::string_view& method, const std::string_view& uri) {
	return Md5(
		Md5(Unquote(user) + ":" + Unquote(Realm) + ":" + Unquote(pwd)) + ":" +
		Nonce + ":" +
		Md5(std::string{ method } + ":" + std::string{ uri }));
}

void cauthchallenge::FromDigestHeader(const std::string_view& authenticate_header) {
	static const std::regex re_keyvalue(R"(([\w-]+)=("|)(?!\")(.*?)\2(,|$))", std::regex_constants::icase);
	std::string_view OptStale;
	std::cregex_iterator next((const char*)authenticate_header.begin() + 7, (const char*)authenticate_header.end(), re_keyvalue), end;
	while (next != end) {
		std::cmatch match = *next;
		if (std::string_view opt{ match[1].first, (size_t)match.length(1) }; opt.compare(0, 5, "realm") == 0) {
			Realm.assign(match[3].first, (size_t)match.length(3));
		}
		else if (opt.compare(0, 6, "domain") == 0) {
			Domain.assign(match[3].first, (size_t)match.length(3));
		}
		else if (opt.compare(0, 5, "nonce") == 0) {
			Nonce.assign(match[3].first, (size_t)match.length(3));
		}
		else if (opt.compare(0, 6, "opaque") == 0) {
			Opaque.assign(match[3].first, (size_t)match.length(3));
		}
		else if (opt.compare(0, 5, "stale") == 0) {
			OptStale = { match[3].first, (size_t)match.length(3) };
		}
		else if (opt.compare(0, 8, "response") == 0) {
			Response = { match[3].first, (size_t)match.length(3) };
		}
		else if (opt.compare(0, 9, "algorithm") == 0) {
			Algorithm.assign(match[3].first, (size_t)match.length(3));
		}
		else if (opt.compare(0, 3, "qop") == 0) {
			Qop.assign(match[3].first, (size_t)match.length(3));
		}
		next++;
	}
	Stale = !OptStale.empty() && (OptStale.front() == 't' || OptStale.front() == 'T');
}

std::string cauthchallenge::GetAuthorizationHeader(const std::string_view& user, const std::string_view& pwd, const std::string_view& method, const std::string_view& uri) {
	static constexpr const char* format_basic{ "Authorization: Basic %s\r\n" };
	static constexpr const char* format_no_opaque{ "Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\n" };
	static constexpr const char* format_wh_opaque{ "Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\", opaque=\"%s\"\r\n" };
	auto buffer{ (char*)alloca(1024) };
	ssize_t nwrite{ 0 };
	switch (Type) {
	case auth_t::digest:
		nwrite = snprintf(buffer, 1024, Opaque.empty() ? format_no_opaque : format_wh_opaque, std::string{ user }.c_str(), Realm.c_str(), Nonce.c_str(), std::string{ uri }.c_str(),
			Digest(user, pwd, method, uri).c_str(), Opaque.c_str());
		return { buffer, buffer + nwrite };
	case auth_t::basic:
		nwrite = snprintf(buffer, 1024, format_basic, ToBase64(std::string{ user } + ":" + std::string{ pwd }).c_str());
		return { buffer, buffer + nwrite };
	case auth_t::none:
	default:
		return { "" };
	}
}

void cauthchallenge::SetFromHeader(std::string_view authenticate_header) {

	Reset();

	if (!authenticate_header.empty()) {
		if (strncasecmp(authenticate_header.data(), "Digest ", 7) == 0) {
			FromDigestHeader(authenticate_header);
			Type = auth_t::digest;
		}
		else if (strncasecmp(authenticate_header.data(), "Basic ", 6) == 0) {
			Type = auth_t::basic;
		}
	}
}

std::string cauthchallenge::GenNonce(size_t length) {
	std::string nonce; nonce.reserve(length);
	static constexpr char alphaNonce[]{ "ABCDEFGHIJKLMNOPQRSTUVWXYZ" "abcdefghijklmnopqrstuvwxyz" "0123456789" };
	static constexpr size_t alphaSize{ sizeof(alphaNonce) };
	while (length--) { nonce.push_back(alphaNonce[mrand48() % alphaSize]); }
	return nonce;
}

cauthchallenge::cauthchallenge(auth_t type, const std::string& realm, const std::string& user, const std::string& pwd) :
	Type{ type }, Realm{ realm }, User{ user }, Pwd{ pwd }
{
	;
}

static inline size_t parseNumber(std::string_view value);
static inline std::string_view parserNext(std::string_view& data, char symbol);
static inline char splitBy(std::string_view& data, std::string_view& result, const char* symbol);
static inline char splitBy(std::string_view& data, std::string_view& result, char symbol);
static inline std::string_view toLower(std::string_view& data);

ssize_t http::ParseRequest(std::string_view request, std::string_view& method, uri_t& path, headers_t& headers, std::string_view& content, size_t& contentLength) {
	static const std::string_view eod{ "\r\n\r\n", 4 };

	const char* begin{ request.data() }; // for length calculation

	method = parserNext(request, ' ');
	std::string_view uri{ parserNext(request, ' ') };

	if (auto proto = parserNext(request, '\n'); proto.compare(0, 5, "HTTP/") == 0 or proto.compare(0, 7, "RTSP/1.") == 0) {

		while (!request.empty()) {
			if (auto hk{ parserNext(request, ':') }; !hk.empty()) {
				request.remove_prefix(1);
				headers.emplace(toLower(hk), parserNext(request, '\n'));
				if (eod != std::string_view{ request.data() - 4, 4 }) {
					continue;
				}
				break;
			}
			return -EBADMSG;
		}


		/* uri split */

		if (!uri.empty()) {
			path.uriFull = uri;
			path.uriPath = uri.substr(0, uri.find_first_of("? "));
			if (uri[0] == '/') { path.uriPathList.emplace_back(uri.substr(0, 1)); }
			std::string_view paramslist;
			for (std::string_view uripath{ uri }, path_part; !uripath.empty();) {
				if (auto ch = splitBy(uripath, path_part, "/?"); ch == '/') {
					if (!path_part.empty()) {
						path.uriPathList.emplace_back(path_part);
					}
					continue;
				}
				else if (ch == '?') {
					if (!path_part.empty()) {
						path.uriPathList.emplace_back(path_part);
					}
					paramslist = uripath;
				}
				else if (!uripath.empty()) {
					path.uriPathList.emplace_back(uripath);
				}
				break;
			}

			for (std::string_view param, key; !paramslist.empty();) {
				if (splitBy(paramslist, param, '&') == '&') {
					if (splitBy(param, key, '=') == '=') {
						path.uriArgs.emplace(key, param);
					}
					else {
						path.uriArgs.emplace(param, std::string_view{});
					}
					continue;
				}
				else if (!paramslist.empty()) {
					if (splitBy(paramslist, key, '=') == '=') {
						path.uriArgs.emplace(key, paramslist);
					}
					else {
						path.uriArgs.emplace(paramslist, std::string_view{});
					}
				}
				break;
			}
		}
		content = { request.data(), 0 };

		if (auto&& clIt{ headers.find("content-length") }; clIt != headers.end()) {
			contentLength = parseNumber(clIt->second);
			if (request.length() < contentLength) {
				content = { request.data(), request.length() };
				return -ENODATA;
			}
			content = { request.data(), contentLength };
		}

		return (ssize_t)((ptrdiff_t)(content.data() + content.length()) - (ptrdiff_t)begin);
	}
	return -EPROTONOSUPPORT;
}

ssize_t http::ParseResponse(std::string_view stream, std::string_view& code, std::string_view& msg, headers_t& headers, size_t& content_length, std::string_view& content) {
	static const std::string_view eod{ "\r\n\r\n", 4 };

	const char* begin{ stream.data() }; // for length calculation

	std::string_view&& method{ parserNext(stream, ' ') };
	code = parserNext(stream, ' ');
	msg = parserNext(stream, '\n');

	if (method.compare(0, 5, "HTTP/") == 0 or method.compare(0, 7, "RTSP/1.") == 0) {
		while (!stream.empty()) {
			if (auto hk{ parserNext(stream, ':') }; !hk.empty()) {
				stream.remove_prefix(1);
				headers.emplace(toLower(hk), parserNext(stream, '\n'));
				if (eod != std::string_view{ stream.data() - 4, 4 }) {
					continue;
				}
				break;
			}
			return -ENODATA;
		}
		if (auto&& h_content{ headers.find("content-length") }; h_content == headers.end()) {
			content = { stream.data(), 0 };
		}
		else {
			content_length = parseNumber(h_content->second);
			if (stream.length() < content_length) {
				content = { stream.data(), stream.length() };
				return -ENODATA;
			}
			content = { stream.data(), content_length };
		}

		return (ssize_t)((ptrdiff_t)(content.data() + content.length()) - (ptrdiff_t)begin);
	}
	return -EPROTONOSUPPORT;
}


static inline std::string_view toLower(std::string_view& data) {
	char* ptr{ (char*)data.data() };
	for (size_t n{ data.length() }; n--; *ptr = (char)std::tolower(*ptr), ++ptr) { ; }
	return data;
}

static inline size_t parseNumber(std::string_view value) {
	size_t nvalue{ 0 };
	while (!value.empty() && std::isspace(value.front())) { value.remove_prefix(1); }
	while (!value.empty() and std::isdigit(value.front())) { nvalue = nvalue * 10 + value.front() - '0'; value.remove_prefix(1); }
	return nvalue;
}

static inline std::string_view parserNext(std::string_view& data, char symbol) {
	std::string_view from{ data };
	while (!data.empty() && data.front() != symbol) { data.remove_prefix(1); }
	from = { from.begin(),(size_t)(data.begin() - from.begin()) };
	while (!from.empty() && std::isspace(from.front())) { from.remove_prefix(1); }
	while (!from.empty() && std::isspace(from.back())) { from.remove_suffix(1); }
	while (!data.empty() && std::isspace(data.front())) { data.remove_prefix(1); }
	return from;
}

static inline char splitBy(std::string_view& data, std::string_view& result, const char* symbol) {
	if (!data.empty()) {
		if (auto npos = data.find_first_of(symbol); npos != std::string_view::npos) {
			result = data.substr(0, npos);
			data.remove_prefix(npos + 1);
			return *(data.data() - 1);
		}
	}
	return 0;
}

static inline char splitBy(std::string_view& data, std::string_view& result, char symbol) {
	if (!data.empty()) {
		if (auto npos = data.find_first_of(symbol); npos != std::string_view::npos) {
			result = data.substr(0, npos);
			data.remove_prefix(npos + 1);
			return *(data.data() - 1);
		}
	}
	return 0;
}


std::string_view& http::TrimBlank(std::string_view& str) {
	while (!str.empty() && std::isblank(str.front())) { str.remove_prefix(1); }
	while (!str.empty() && std::isblank(str.back())) { str.remove_suffix(1); }
	return str;
}


std::string http::FromBase64(const std::string_view& value) {
	static constexpr uint8_t base64TableDecode[] = {
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,		//   0..15
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,		//  16..31
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,  62, 255, 255, 255,  63,		//  32..47
		 52,  53,  54,  55,  56,  57,  58,  59,  60,  61, 255, 255, 255, 254, 255, 255,		//  48..63
		255,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,		//  64..79
		 15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25, 255, 255, 255, 255, 255,		//  80..95
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
	std::string result;
	if (!value.empty()) {
		result.reserve(((size_t)(value.length() / 3) << 2) + 1);
		uint8_t* in{ (uint8_t*)value.data() };
		size_t length{ value.length() };

		for (auto len = length & (~3); len; len -= 4, in += 4) {
			result.push_back((uint8_t)(base64TableDecode[in[0]] << 2 | base64TableDecode[in[1]] >> 4));
			result.push_back((uint8_t)(base64TableDecode[in[1]] << 4 | base64TableDecode[in[2]] >> 2));
			result.push_back((uint8_t)(((base64TableDecode[in[2]] << 6) & 0xc0) | base64TableDecode[in[3]]));
		}
		while (length-- && value[length] == '=') { result.pop_back(); }
		result.shrink_to_fit();
	}
	return result;
}


std::string http::ToBase64(const std::string_view& value) {
	static constexpr uint8_t base64TableEncode[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ" "abcdefghijklmnopqrstuvwxyz" "0123456789+/";
	std::string ret;
	size_t length{ value.length() };
	uint8_t* source{ (uint8_t*)value.data() };

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

			ret.push_back(base64TableEncode[char_array_4[0]]);
			ret.push_back(base64TableEncode[char_array_4[1]]);
			ret.push_back(base64TableEncode[char_array_4[2]]);
			ret.push_back(base64TableEncode[char_array_4[3]]);
			i = 0;
		}
	}

	if (i)
	{
		for (j = i; j < 3; char_array_3[j] = '\0', j++);

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = (uint8_t)(((char_array_3[0] & 0x03) << 4) + (uint8_t)((char_array_3[1] & 0xf0) >> 4));
		char_array_4[2] = (uint8_t)(((char_array_3[1] & 0x0f) << 2) + (uint8_t)((char_array_3[2] & 0xc0) >> 6));

		for (j = 0; (j < i + 1); j++) { ret.push_back(base64TableEncode[char_array_4[j]]); }
		while ((i++ < 3)) { ret.push_back('='); }
	}
	ret.shrink_to_fit();
	return ret;
}

size_t http::ToNumber(std::string_view value) {
	size_t num{ 0 };
	while (!value.empty() and std::isdigit(value.front())) {
		num *= 10;
		num += value.front() - '0';
		value.remove_prefix(1);
	}
	return num;
}

std::string http::Md5(const std::string& value) {
	static constexpr uint32_t k[64] = {
		0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee ,
		0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501 ,
		0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be ,
		0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821 ,
		0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa ,
		0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8 ,
		0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed ,
		0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a ,
		0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c ,
		0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70 ,
		0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05 ,
		0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665 ,
		0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039 ,
		0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1 ,
		0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1 ,
		0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391 };

	static constexpr uint32_t r[] = { 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
										5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
										4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
										6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21 };

#define LEFTROTATE(x, c) (((x) << (c)) | ((x) >> (32 - (c))))
#define to_bytes(val, bytes){bytes[0] = (uint8_t)val;bytes[1] = (uint8_t)(val >> 8);bytes[2] = (uint8_t)(val >> 16);bytes[3] = (uint8_t)(val >> 24);}
#define to_int32(bytes) ((uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) | ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24));

	uint32_t h0, h1, h2, h3;
	uint8_t* msg = NULL;

	size_t new_len, offset;
	uint32_t w[16];
	uint32_t a, b, c, d, i, f, g, temp;
	h0 = 0x67452301;
	h1 = 0xefcdab89;
	h2 = 0x98badcfe;
	h3 = 0x10325476;

	for (new_len = value.size() + 1; new_len % (512 / 8) != 448 / 8; new_len++);

	msg = (uint8_t*)alloca(new_len + 8);
	memcpy(msg, value.data(), value.size());
	msg[value.size()] = 0x80;
	for (offset = value.size() + 1; offset < new_len; offset++) { msg[offset] = 0; }

	to_bytes((uint32_t)(value.size() * 8), (msg + new_len));
	to_bytes((uint32_t)(value.size() >> 29), (msg + new_len + 4));
	for (offset = 0; offset < new_len; offset += (512 / 8)) {

		for (i = 0; i < 16; i++) {
			w[i] = to_int32((msg + offset + i * 4));
		}

		a = h0; b = h1; c = h2; d = h3;

		for (i = 0; i < 64; i++) {
			if (i < 16) { f = (b & c) | ((~b) & d); g = i; }
			else if (i < 32) { f = (d & b) | ((~d) & c); g = (5 * i + 1) % 16; }
			else if (i < 48) { f = b ^ c ^ d; g = (3 * i + 5) % 16; }
			else { f = c ^ (b | (~d)); g = (7 * i) % 16; }

			temp = d;
			d = c;
			c = b;
			b = b + LEFTROTATE((a + f + k[i] + w[g]), r[i]);
			a = temp;
		}

		h0 += a;
		h1 += b;
		h2 += c;
		h3 += d;
	}
	static const char dec2hex[16 + 1] = "0123456789abcdef";
	std::string digest;
	digest.reserve(32);

#define to_hex(ch) digest.push_back(dec2hex[((ch) >> 4) & 15]); digest.push_back(dec2hex[(ch) & 15]); 

	to_hex((uint8_t)(h0)); to_hex((uint8_t)(h0 >> 8)); to_hex((uint8_t)(h0 >> 16)); to_hex((uint8_t)(h0 >> 24));
	to_hex((uint8_t)(h1)); to_hex((uint8_t)(h1 >> 8)); to_hex((uint8_t)(h1 >> 16)); to_hex((uint8_t)(h1 >> 24));
	to_hex((uint8_t)(h2)); to_hex((uint8_t)(h2 >> 8)); to_hex((uint8_t)(h2 >> 16)); to_hex((uint8_t)(h2 >> 24));
	to_hex((uint8_t)(h3)); to_hex((uint8_t)(h3 >> 8)); to_hex((uint8_t)(h3 >> 16)); to_hex((uint8_t)(h3 >> 24));

	return digest;
}

std::string http::Unquote(const std::string_view& value) {
	std::string result;
	result.reserve(value.length());
	for (ssize_t n = 0; n < (ssize_t)value.length(); n++) {
		if (value[n] == '\\' && value[n + 1] == '"') { ++n; continue; }
		result.push_back(value[n]);
	}
	result.shrink_to_fit();
	return result;
}