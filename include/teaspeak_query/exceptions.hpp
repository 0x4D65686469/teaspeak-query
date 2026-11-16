#pragma once
#include <map>
#include <stdexcept>
#include <string>

namespace teaspeak_query {

struct QueryErrorInfo {
    int         id = 0;
    std::string msg;
    std::map<std::string, std::string> raw;
};

// Error codes (from the TeamSpeak / TeaSpeak protocol docs)
namespace ErrorCode {
    // normal
    constexpr int OK = 0;
    constexpr int UNDEFINED = 1;
    constexpr int NOT_IMPLEMENTED = 2;
    constexpr int COMMAND_NOT_FOUND = 256;

    // client
    constexpr int CLIENT_INVALID_ID = 512;
    constexpr int CLIENT_NICKNAME_INUSE = 513;
    constexpr int CLIENT_PROTOCOL_LIMIT_REACHED = 515;
    constexpr int CLIENT_INVALID_PASSWORD = 520;

    // channel
    constexpr int CHANNEL_INVALID_ID = 768;
    constexpr int CHANNEL_NAME_INUSE = 771;
    constexpr int CHANNEL_NOT_EMPTY = 772;

    // server
    constexpr int SERVER_INVALID_ID = 1024;
    constexpr int SERVER_MAXCLIENTS_REACHED = 1027;
    constexpr int SERVER_INVALID_PASSWORD = 1028;

    // database
    constexpr int DATABASE_ERROR = 1280;
    constexpr int DATABASE_EMPTY_RESULT = 1281;
    constexpr int DATABASE_DUPLICATE_ENTRY = 1282;

    // parameter
    constexpr int PARAMETER_INVALID = 1538;
    constexpr int PARAMETER_MISSING = 1542;

    // connection / TeaSpeak special
    constexpr int VS_CRITICAL = 1792;
    constexpr int CONNECTION_LOST = 1793;
    constexpr int COULD_NOT_RESOLVE_HOSTNAME = 1798;

    // file
    constexpr int FILE_NOT_FOUND = 2051;

    // permission
    constexpr int PERMISSION_INVALID_GROUP_ID = 2560;
    constexpr int PERMISSIONS_CLIENT_INSUFFICIENT = 2568;

    // ban
    constexpr int CONNECT_FAILED_BANNED = 3329;
    constexpr int RENAME_FAILED_BANNED = 3330;

    // token
    constexpr int PRIVILEGE_KEY_INVALID = 3840;
}

class QueryError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class ConnectionLost    : public QueryError { using QueryError::QueryError; };
class AuthenticationError : public QueryError { using QueryError::QueryError; };
class QueryTimeout      : public QueryError { using QueryError::QueryError; };

class CommandError : public QueryError {
public:
    CommandError(const std::string& what, QueryErrorInfo info)
        : QueryError(what), info_(std::move(info)) {}

    const QueryErrorInfo& error()     const noexcept { return info_; }
    int                    error_id() const noexcept { return info_.id; }
    const std::string&     error_msg()const noexcept { return info_.msg; }

    bool is_permission_denied()  const noexcept {
        return info_.id == ErrorCode::PERMISSIONS_CLIENT_INSUFFICIENT;
    }
    bool is_nickname_in_use()    const noexcept {
        return info_.id == ErrorCode::CLIENT_NICKNAME_INUSE;
    }
    bool is_teaspeak_guest_bug() const noexcept {
        return info_.id == ErrorCode::VS_CRITICAL;
    }
    bool is_not_found()          const noexcept {
        return info_.id == ErrorCode::FILE_NOT_FOUND ||
               info_.id == ErrorCode::DATABASE_EMPTY_RESULT;
    }

private:
    QueryErrorInfo info_;
};

} // namespace teaspeak_query