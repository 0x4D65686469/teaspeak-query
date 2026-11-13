#pragma once
#include <string>
#include <vector>
#include "teaspeak_query/types.hpp"

namespace teaspeak_query::detail {

KeyValueMap                 parse_kv(const std::string& line);
ItemList                    parse_items(const std::string& text);
std::vector<QueryErrorInfo> parse_error_line(const std::string& line);

} // namespace teaspeak_query::detail