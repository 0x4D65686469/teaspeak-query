#include "teaspeak_query/params.hpp"

namespace teaspeak_query {

Params::Params(std::initializer_list<std::pair<std::string, std::string>> init) {
    for (auto& p : init) set(p.first, p.second);
}

Params& Params::set(const std::string& k, const std::string& v) {
    items_.emplace_back(k, escape(v));
    return *this;
}
Params& Params::set(const std::string& k, const char* v) {
    return set(k, std::string(v ? v : ""));
}
Params& Params::set(const std::string& k, int v) {
    items_.emplace_back(k, std::to_string(v));
    return *this;
}
Params& Params::set(const std::string& k, long long v) {
    items_.emplace_back(k, std::to_string(v));
    return *this;
}
Params& Params::set(const std::string& k, bool v) {
    items_.emplace_back(k, v ? "1" : "0");
    return *this;
}
Params& Params::set_list(const std::string& k, const std::vector<std::string>& v) {
    std::string joined;
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (i) joined += '|';
        joined += escape(v[i]);
    }
    items_.emplace_back(k, std::move(joined));
    return *this;
}
Params& Params::set_raw(const std::string& k, const std::string& v) {
    items_.emplace_back(k, v);
    return *this;
}

std::string Params::build() const {
    std::string out;
    for (auto& [k, v] : items_) {
        if (!out.empty()) out += ' ';
        out += k;
        out += '=';
        out += v;
    }
    return out;
}

} // namespace teaspeak_query