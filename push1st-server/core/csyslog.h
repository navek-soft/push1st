#pragma once
#include <cstdio>
#include <cstdarg>
#include <sstream>
#include <ctime>

namespace core {
	class csyslog {
		static inline size_t logLevel{ 4 };
		static inline FILE* StdOut{ stdout }, * StdErr{ stderr };
	public:
		inline void open(size_t lvl, FILE* fdStdOut = stdout, FILE* fdStdErr = stderr) const {
			logLevel = lvl; StdOut = fdStdOut; StdErr = fdStdErr;
			if (StdOut) { setvbuf(StdOut, NULL, _IOLBF, 0); }
			if (StdErr) { setvbuf(StdErr, NULL, _IOLBF, 0); }
		}
		inline void ts(std::tm& tm) const { std::time_t ts; std::time(&ts); localtime_r(&ts, &tm); }
		inline void ts(FILE* fp) const {
			std::tm tm; ts(tm); fprintf(fp, "%02d/%02d %02d:%02d:%02d ", tm.tm_mday, tm.tm_mon + 1, tm.tm_hour, tm.tm_min, tm.tm_sec);
		}
		inline void print(size_t lvl, const char* fmt, ...) const { if (logLevel >= lvl) { va_list args; ts(StdOut); va_start(args, fmt); vfprintf(StdOut, fmt, args); va_end(args); } }
		inline void trace(const char* fmt, ...) const { if (logLevel >= 4) { va_list args; ts(StdOut); va_start(args, fmt); vfprintf(StdOut, fmt, args); va_end(args); } }
		inline void error(const char* fmt, ...) const { va_list args; va_start(args, fmt); ts(StdErr); vfprintf(StdErr, fmt, args); va_end(args); fflush(StdErr); }
		inline void exit(const char* fmt, ...) const { va_list args; va_start(args, fmt); ts(StdErr); vfprintf(StdErr, fmt, args); va_end(args); fflush(StdErr); ::exit(EXIT_FAILURE); }
		inline bool is(size_t lvl) const { return logLevel >= lvl; }
		void bt(int depth) const;
		struct cob {
		private:
			static inline size_t logObWidth{ 21 };
			static inline std::stringstream logBuffering;
		public:
			inline void print(const std::string_view& section, const char* fmt, ...) const;
			inline void flush(size_t lvl) const { if (logLevel >= lvl and logBuffering.tellp()) { fprintf(StdOut, "%s", logBuffering.str().c_str()); } logBuffering.clear(); }
		} ob;
	};
	inline void csyslog::cob::print(const std::string_view& section, const char* fmt, ...) const {
		char* buffer{ (char*)alloca(512) };
		snprintf(buffer, 511, "%*s | ", logObWidth, section.empty() ? " " : section.data());
		logBuffering << buffer;
		va_list args; va_start(args, fmt);
		auto nwrite = vsnprintf(buffer, 511, fmt, args);
		va_end(args);
		logBuffering << buffer;
		if (buffer[nwrite] != '\n') logBuffering << '\n';
	}
}

static inline constexpr core::csyslog syslog;