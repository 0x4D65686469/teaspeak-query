#include "teaspeak_query/escape.hpp"

namespace teaspeak_query {
namespace {

struct Pair { char from; const char* to; };

constexpr Pair kEscapeTable[] = {
    {'\\', "\\\\"}, {'/', "\\/"}, {' ', "\\s"}, {'|', "\\p"},
    {'\n', "\\n"},  {'\r', "\\r"}, {'\t', "\\t"}, {'\v', "\\v"},
    {'\f', "\\f"},  {'\b', "\\b"}, {'\a', "\\a"},
};

bool decode(char c, char& out) {
    switch (c) {
        case '\\': out = '\\'; return true;
        case '/':  out = '/';  return true;
        case 's':  out = ' ';  return true;
        case 'p':  out = '|';  return true;
        case 'n':  out = '\n'; return true;
        case 'r':  out = '\r'; return true;
        case 't':  out = '\t'; return true;
        case 'v':  out = '\v'; return true;
        case 'f':  out = '\f'; return true;
        case 'b':  out = '\b'; return true;
        case 'a':  out = '\a'; return true;
        default:   return false;
    }
}

} // namespace

std::string escape(const std::string& v) {
    std::string out;
    out.reserve(v.size() + v.size() / 4);
    for (char c : v) {
        bool done = false;
        for (const auto& p : kEscapeTable) {
            if (p.from == c) { out += p.to; done = true; break; }
        }
        if (!done) out += c;
    }
    return out;
}

std::string unescape(const std::string& v) {
    std::string out;
    out.reserve(v.size());
    for (std::size_t i = 0; i < v.size(); ++i) {
        char c;
        if (v[i] == '\\' && i + 1 < v.size() && decode(v[i + 1], c)) {
            out += c;
            ++i;
        } else {
            out += v[i];
        }
    }
    return out;
}

} // namespace teaspeak_query