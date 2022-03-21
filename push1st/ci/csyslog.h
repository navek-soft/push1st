#pragma once
#include <cstdarg>
#include <cstring>
#include <string>
#include <ctime>

class csyslog {
public:
	static inline int verbose{ 2 };
	static inline bool pretty{ false };
	csyslog(const std::string& section = "") : logSection{ section + " " } {}
	~csyslog() = default;

	static inline const std::string_view Red{ "\x1B[31m" };
	static inline const std::string_view Green{ "\x1B[32m" };
	static inline const std::string_view Yellow{ "\x1B[33m" };
	static inline const std::string_view Blue{ "\x1B[34m" };
	static inline const std::string_view Magenta{ "\x1B[35m" };
	static inline const std::string_view Cyan{ "\x1B[36m" };
	static inline const std::string_view White{ "\x1B[37m" };
	static inline const std::string_view Grey{ "\x1B[90m" };

protected:
	template<typename LVL>
	inline void log(LVL lvl, const char* fmt, ...) const;
	inline void log_dbg(const char* fmt, ...) const;
	inline void log_extend(const char* fmt, ...) const;
	inline void log_compact(const char* fmt, ...) const;
	inline void log_none(const char* fmt, ...) const;
	inline void err(const char* fmt, ...) const;
	inline const char* err(ssize_t eno) const;
	inline const char* err(int eno) const;
	inline void log_banner(const std::string& banner) const { logSection = banner; }
public:
	inline std::string _c(const std::string_view& txt, std::string_view c) {
		if (!pretty) { return std::string{ txt }; }
		return std::string{ c }.append(txt).append("\033[0m");
	}

private:
	inline void log(int lvl, const char* fmt, va_list args) const;
	mutable std::string logSection;
};

inline void csyslog::log_dbg(const char* fmt, ...) const {
	va_list args; va_start(args, fmt);
	log(4, fmt, args);
}
inline void csyslog::log_extend(const char* fmt, ...) const {
	va_list args; va_start(args, fmt);
	log(3, fmt, args);
}
inline void csyslog::log_compact(const char* fmt, ...) const {
	va_list args; va_start(args, fmt);
	log(2, fmt, args);
}
inline void csyslog::log_none(const char* fmt, ...) const {
	va_list args; va_start(args, fmt);
	log(1, fmt, args);
}

inline void csyslog::log(int lvl, const char* fmt, va_list args) const {
#ifdef DEBUG
	if (1) {
#else
	if (lvl <= verbose) {
#endif
		if (char* strbuf{ (char*)alloca(1024) }; vsnprintf(strbuf, 1024, fmt, args) > 0) {
			auto ts{ std::time(nullptr) };
			std::tm tm;
			localtime_r(&ts, &tm);
			if (!pretty)
				fprintf(stdout, "%4d-%02d-%02d %02d:%02d:%02d %s%s", tm.tm_year + 1900, tm.tm_mday, tm.tm_mon + 1, tm.tm_hour, tm.tm_min, tm.tm_sec, logSection.c_str(), strbuf);
			else
				fprintf(stdout, "\x1B[90m%4d-%02d-%02d %02d:%02d:%02d\033[0m %s%s", tm.tm_year + 1900, tm.tm_mday, tm.tm_mon + 1, tm.tm_hour, tm.tm_min, tm.tm_sec, logSection.c_str(), strbuf);
		}
		va_end(args);
	}
	}

template<typename LVL>
inline void csyslog::log(LVL lvl, const char* fmt, ...) const {
	va_list args; va_start(args, fmt);
	log((int)lvl, fmt, args);
}

inline void csyslog::err(const char* fmt, ...) const {
	va_list args; va_start(args, fmt);
	if (char* strbuf{ (char*)alloca(1024) }; vsnprintf(strbuf, 1024, fmt, args) > 0) {
		auto ts{ std::time(nullptr) };
		std::tm tm;
		localtime_r(&ts, &tm);
		fprintf(stderr, "%4d-%02d-%02d %02d:%02d:%02d %s%s", tm.tm_year + 1900, tm.tm_mday, tm.tm_mon + 1, tm.tm_hour, tm.tm_min, tm.tm_sec, logSection.c_str(), strbuf);
	}
	va_end(args);
}
inline const char* csyslog::err(ssize_t eno) const { return std::strerror(-(int)eno); }
inline const char* csyslog::err(int eno) const { return std::strerror(eno); }