#pragma once
#include <cstdlib>
#include <optional>
#include <string>

namespace ex {

inline std::optional<std::string> get_env(const char* name) {
#ifdef _WIN32
#  ifdef _MSC_VER
#    pragma warning(push)
#    pragma warning(disable: 4996)
#  endif
    const char* v = std::getenv(name);
#  ifdef _MSC_VER
#    pragma warning(pop)
#  endif
#else
    const char* v = std::getenv(name);
#endif
    if (!v || !*v) return std::nullopt;
    return std::string(v);
}

} // namespace ex