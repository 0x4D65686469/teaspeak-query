#include "teaspeak_query/types.hpp"
#include "parser.hpp"

namespace teaspeak_query {

bool QueryResponse::ok() const noexcept {
    for (auto& e : errors) if (e.id != 0) return false;
    return true;
}

QueryErrorInfo QueryResponse::error() const noexcept {
    for (auto& e : errors) if (e.id != 0) return e;
    return QueryErrorInfo{0, "ok", {}};
}

KeyValueMap QueryResponse::as_dict() const {
    if (data.empty()) return {};
    return detail::parse_kv(data[0]);
}

ItemList QueryResponse::as_list() const {
    ItemList out;
    for (auto& line : data) {
        auto items = detail::parse_items(line);
        out.insert(out.end(), items.begin(), items.end());
    }
    return out;
}

std::string QueryResponse::as_text() const {
    std::string out;
    for (std::size_t i = 0; i < data.size(); ++i) {
        if (i) out += '\n';
        out += data[i];
    }
    return out;
}

QueryResponse& QueryResponse::raise_for_error() {
    auto e = error();
    if (e.id != 0)
        throw CommandError("Error " + std::to_string(e.id) + ": " + e.msg, e);
    return *this;
}

std::string QueryEvent::get(const std::string& key,
                            const std::string& def) const {
    auto it = data.find(key);
    return it != data.end() ? it->second : def;
}

} // namespace teaspeak_query