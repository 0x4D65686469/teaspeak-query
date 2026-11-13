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

private:
    QueryErrorInfo info_;
};

} // namespace teaspeak_query