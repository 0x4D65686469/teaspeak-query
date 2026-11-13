#pragma once
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

#include "escape.hpp"

namespace teaspeak_query {

/// Builder for command parameters.  String values are escaped automatically.
class Params {
public:
    Params() = default;
    Params(std::initializer_list<std::pair<std::string, std::string>> init);

    Params& set(const std::string& key, const std::string& value);
    Params& set(const std::string& key, const char* value);
    Params& set(const std::string& key, int value);
    Params& set(const std::string& key, long long value);
    Params& set(const std::string& key, bool value);
    Params& set_list(const std::string& key,
                     const std::vector<std::string>& values);

    /// Pre-escaped value (skips escaping).
    Params& set_raw(const std::string& key, const std::string& escaped);

    bool        empty() const noexcept { return items_.empty(); }
    std::string build() const;

private:
    std::vector<std::pair<std::string, std::string>> items_;
};

} // namespace teaspeak_query