#pragma once
#include <string>

namespace teaspeak_query {

/// Escape a value so it can be embedded inside a ServerQuery command.
std::string escape(const std::string& value);

/// Reverse the ServerQuery escaping (\\\\ \\/ \\s \\p \\n \\r \\t \\v \\f \\b \\a).
std::string unescape(const std::string& value);

} // namespace teaspeak_query