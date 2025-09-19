#pragma once
#include <core/util/cjson.h>
#include <spdlog/fmt/fmt.h>

#include <string>

#if defined(__GNUC__) || defined(__clang__)
#if defined(__x86_64__) || defined(_M_X64)
#define ARCH_X86_64
#elif defined(__i386) || defined(_M_IX86)
#define ARCH_X86
#elif defined(__aarch64__) || defined(_M_ARM64)
#define ARCH_ARM64
#elif defined(__arm__) || defined(_M_ARM)
#define ARCH_ARM
#else
#error "Unsupported architecture"
#endif
#else
#error "Unsupported compiler"
#endif

static inline const class Version {
   public:
    Version() {
        extern const char* VersionInfo[];
        AppName = VersionInfo[0];
        Patch = VersionInfo[1];
        BuildNo = VersionInfo[2];
        Minor = VersionInfo[3];
        Major = VersionInfo[4];
        Commit = VersionInfo[5];
        Revision = VersionInfo[6];
        BuildTimestamp = VersionInfo[7];
        BuildType = VersionInfo[8];
        BuildFlags = VersionInfo[9];
    }

    inline std::string GetJson() const {
        json::value_t val;
        val["version"] = fmt::format("{}.{}.{}.{}", Major, Minor, Patch, BuildNo);
        val["name"] = AppName;
        val["commit"] = Commit;
        val["revision"] = Revision;
        val["timestamp"] = BuildTimestamp;
        val["build"] = BuildType;
        val["flags"] = BuildFlags;
        return json::Serialize(val);
    }

   public:
    const char* AppName;
    const char* Patch;
    const char* BuildNo;
    const char* Minor;
    const char* Major;
    const char* Commit;
    const char* Revision;
    const char* BuildTimestamp;
    const char* BuildType;
    const char* BuildFlags;
} Version;
