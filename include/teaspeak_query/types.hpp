#pragma once
#include <map>
#include <string>
#include <vector>

#include "exceptions.hpp"

namespace teaspeak_query {

using KeyValueMap = std::map<std::string, std::string>;
using ItemList    = std::vector<KeyValueMap>;

struct QueryResponse {
    std::vector<std::string>     data;
    std::vector<QueryErrorInfo>  errors;

    bool          ok()    const noexcept;
    QueryErrorInfo error()const noexcept;
    KeyValueMap   as_dict() const;
    ItemList      as_list() const;
    std::string   as_text() const;
    QueryResponse& raise_for_error();
};

struct QueryEvent {
    std::string name;      // "textmessage"  (without notify)
    std::string raw_name;  // "notifytextmessage"
    KeyValueMap data;
    std::string raw;

    std::string get(const std::string& key,
                    const std::string& def = {}) const;
};

namespace TargetMode {
    constexpr int PRIVATE = 1;
    constexpr int CHANNEL = 2;
    constexpr int SERVER  = 3;
}
namespace ClientType {
    constexpr int VOICE = 0;
    constexpr int QUERY = 1;
}
namespace ReasonID {
    constexpr int KICK_CHANNEL      = 4;
    constexpr int KICK_SERVER       = 5;
    constexpr int MOVE_SELF         = 0;
    constexpr int MOVE_ADMIN        = 1;
    constexpr int MOVE_KICK_CHANNEL = 4;
}

} // namespace teaspeak_query