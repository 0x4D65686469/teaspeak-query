#include "parser.hpp"
#include "teaspeak_query/escape.hpp"
#include <cctype>
#include <sstream>

namespace teaspeak_query::detail {

KeyValueMap parse_kv(const std::string& line) {
    KeyValueMap out;
    if (line.empty()) return out;
    std::istringstream iss(line);
    std::string tok;
    while (iss >> tok) {
        auto eq = tok.find('=');
        if (eq != std::string::npos)
            out[tok.substr(0, eq)] = unescape(tok.substr(eq + 1));
    }
    return out;
}

ItemList parse_items(const std::string& text) {
    ItemList out;
    if (text.empty()) return out;
    std::size_t start = 0;
    while (start < text.size()) {
        auto bar = text.find('|', start);
        if (bar == std::string::npos) {
            out.push_back(parse_kv(text.substr(start)));
            break;
        }
        out.push_back(parse_kv(text.substr(start, bar - start)));
        start = bar + 1;
    }
    return out;
}

std::vector<QueryErrorInfo> parse_error_line(const std::string& line) {
    std::string body = line;
    if (body.rfind("error", 0) == 0) body = body.substr(5);
    while (!body.empty() && std::isspace(static_cast<unsigned char>(body.front()))) body.erase(body.begin());
    while (!body.empty() && std::isspace(static_cast<unsigned char>(body.back())))  body.pop_back();

    if (body.empty()) return { QueryErrorInfo{0, "ok", {}} };

    std::vector<QueryErrorInfo> out;
    std::size_t start = 0;
    while (true) {
        auto bar = body.find('|', start);
        std::string part = (bar == std::string::npos)
            ? body.substr(start)
            : body.substr(start, bar - start);
        auto kv = parse_kv(part);
        QueryErrorInfo info;
        info.raw = kv;
        info.id  = -1;
        auto it = kv.find("id");
        if (it != kv.end()) {
            try { info.id = std::stoi(it->second); } catch (...) { info.id = -1; }
        }
        auto mt = kv.find("msg");
        if (mt != kv.end()) info.msg = mt->second;
        out.push_back(std::move(info));
        if (bar == std::string::npos) break;
        start = bar + 1;
    }
    if (out.empty()) out.push_back({0, "ok", {}});
    return out;
}

} // namespace teaspeak_query::detail