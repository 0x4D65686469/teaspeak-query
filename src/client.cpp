#include "teaspeak_query/client.hpp"

#include <chrono>
#include <sstream>
#include <stdexcept>
#include <cctype>

#include "socket.hpp"
#include "parser.hpp"

namespace teaspeak_query {

// ------------------------------------------------------------------ //
// ctor / dtor
// ------------------------------------------------------------------ //
ServerQuery::ServerQuery(std::string host, int port, double timeout,
                         bool auto_dispatch)
    : host_(std::move(host))
    , port_(port)
    , timeout_(timeout)
    , auto_dispatch_(auto_dispatch) {}

ServerQuery::~ServerQuery() {
    try { disconnect(); } catch (...) {}
}

// ------------------------------------------------------------------ //
// Lifecycle
// ------------------------------------------------------------------ //
ServerQuery& ServerQuery::connect() {
    if (connected_) return *this;

    sock_ = std::make_unique<detail::Socket>();
    sock_->connect_to(host_, port_, timeout_);

    {
        std::lock_guard<std::mutex> lk(response_mutex_);
        response_data_.clear();
        response_errors_.clear();
        response_ready_ = false;
    }

    stop_flag_ = false;
    connected_ = true;

    _read_greeting();

    reader_thread_ = std::thread([this] { _read_loop(); });
    if (auto_dispatch_) _ensure_dispatcher();
    return *this;
}

void ServerQuery::disconnect() {
    if (connected_.exchange(false)) {
        try { sock_->send_all("quit\n"); } catch (...) {}
    }
    stop_flag_ = true;

    if (sock_) sock_->shutdown_both();

    {
        std::lock_guard<std::mutex> lk(response_mutex_);
        response_ready_ = true;
    }
    response_cv_.notify_all();
    event_cv_.notify_all();

    if (reader_thread_.joinable()     && std::this_thread::get_id() != reader_thread_.get_id())
        reader_thread_.join();
    if (dispatcher_thread_.joinable() && std::this_thread::get_id() != dispatcher_thread_.get_id())
        dispatcher_thread_.join();

    if (sock_) { sock_->close(); sock_.reset(); }
}

bool ServerQuery::connected() const noexcept { return connected_; }

void ServerQuery::set_warning_handler(std::function<void(const std::string&)> h) {
    warn_ = std::move(h);
}

// ------------------------------------------------------------------ //
// Greeting
// ------------------------------------------------------------------ //
void ServerQuery::_read_greeting() {
    if (!sock_) return;
    // Wait up to 2s for first byte, then drain for up to 150ms of silence.
    if (!sock_->wait_readable(2000)) return;
    sock_->drain(150);
}

// ------------------------------------------------------------------ //
// Reader loop
// ------------------------------------------------------------------ //
void ServerQuery::_read_loop() {
    while (!stop_flag_ && connected_) {
        std::string line;
        try {
            if (!sock_->recv_line(line, 1000)) continue;
        } catch (const ConnectionLost&) {
            connected_ = false;
            std::lock_guard<std::mutex> lk(response_mutex_);
            response_ready_ = true;
            response_cv_.notify_all();
            break;
        }

        if (line.empty()) continue;

        if (line.rfind("notify", 0) == 0) {
            _handle_notification(line);
        } else if (line.rfind("error", 0) == 0) {
            std::lock_guard<std::mutex> lk(response_mutex_);
            response_errors_ = detail::parse_error_line(line);
            response_ready_ = true;
            response_cv_.notify_all();
        } else {
            std::lock_guard<std::mutex> lk(response_mutex_);
            response_data_.push_back(line);
        }
    }
}

void ServerQuery::_handle_notification(const std::string& line) {
    auto sp = line.find(' ');
    std::string raw_name = (sp == std::string::npos) ? line : line.substr(0, sp);
    std::string rest     = (sp == std::string::npos) ? ""   : line.substr(sp + 1);
    std::string name     = raw_name.rfind("notify", 0) == 0
                           ? raw_name.substr(6) : raw_name;

    QueryEvent ev{name, raw_name, detail::parse_kv(rest), line};
    {
        std::lock_guard<std::mutex> lk(event_mutex_);
        event_queue_.push(std::move(ev));
    }
    event_cv_.notify_all();
}

// ------------------------------------------------------------------ //
// Dispatcher
// ------------------------------------------------------------------ //
void ServerQuery::_ensure_dispatcher() {
    if (!auto_dispatch_) return;
    if (dispatcher_thread_.joinable()) return;
    dispatcher_thread_ = std::thread([this] { _dispatch_loop(); });
}

void ServerQuery::_dispatch_loop() {
    while (!stop_flag_) {
        QueryEvent ev;
        {
            std::unique_lock<std::mutex> lk(event_mutex_);
            event_cv_.wait_for(lk, std::chrono::milliseconds(200),
                [this]{ return !event_queue_.empty() || stop_flag_.load(); });
            if (stop_flag_) break;
            if (event_queue_.empty()) continue;
            ev = std::move(event_queue_.front());
            event_queue_.pop();
        }
        _dispatch_event(ev);
    }
}

void ServerQuery::_dispatch_event(const QueryEvent& ev) {
    std::vector<EventCallback> anys, spec;
    {
        std::lock_guard<std::mutex> lk(handlers_mutex_);
        anys = any_handlers_;
        auto it = handlers_.find(ev.name);
        if (it != handlers_.end()) spec = it->second;
    }
    for (auto& cb : anys) try { cb(ev); } catch (...) {}
    for (auto& cb : spec) try { cb(ev); } catch (...) {}
}

// ------------------------------------------------------------------ //
// Event registration
// ------------------------------------------------------------------ //
ServerQuery& ServerQuery::on(const std::string& name, EventCallback cb) {
    {
        std::lock_guard<std::mutex> lk(handlers_mutex_);
        handlers_[name].push_back(std::move(cb));
    }
    _ensure_dispatcher();
    return *this;
}

ServerQuery& ServerQuery::on_any(EventCallback cb) {
    {
        std::lock_guard<std::mutex> lk(handlers_mutex_);
        any_handlers_.push_back(std::move(cb));
    }
    _ensure_dispatcher();
    return *this;
}

std::optional<QueryEvent> ServerQuery::poll(int timeout_ms) {
    std::unique_lock<std::mutex> lk(event_mutex_);
    if (!event_cv_.wait_for(lk, std::chrono::milliseconds(timeout_ms),
                            [this]{ return !event_queue_.empty(); }))
        return std::nullopt;
    QueryEvent ev = std::move(event_queue_.front());
    event_queue_.pop();
    return ev;
}

void ServerQuery::listen(std::atomic<bool>* stop) {
    if (auto_dispatch_)
        throw std::runtime_error(
            "listen() is not available when auto_dispatch=true");
    while (!stop_flag_ && !(stop && stop->load())) {
        auto ev = poll(500);
        if (ev) _dispatch_event(*ev);
    }
}

// ------------------------------------------------------------------ //
// execute / command
// ------------------------------------------------------------------ //
QueryResponse ServerQuery::execute(const std::string& command, double timeout) {
    if (!connected_)
        throw ConnectionLost("Not connected");
    if (timeout < 0) timeout = timeout_;

    std::unique_lock<std::mutex> send_lk(send_mutex_);
    {
        std::lock_guard<std::mutex> lk(response_mutex_);
        response_data_.clear();
        response_errors_.clear();
        response_ready_ = false;
    }

    try {
        sock_->send_all(command + "\n");
    } catch (const std::exception& e) {
        connected_ = false;
        throw ConnectionLost(std::string("Failed to send: ") + e.what());
    }

    std::unique_lock<std::mutex> lk(response_mutex_);
    auto dur = std::chrono::milliseconds(static_cast<long long>(timeout * 1000));
    if (!response_cv_.wait_for(lk, dur, [this]{ return response_ready_; }))
        throw QueryTimeout(
            "Command timed out after " + std::to_string(timeout) +
            "s: " + command.substr(0, 60));

    if (!connected_ && response_errors_.empty())
        throw ConnectionLost("Connection closed while waiting");

    QueryResponse resp;
    resp.data   = response_data_;
    resp.errors = response_errors_;
    return resp;
}

QueryResponse ServerQuery::command(const std::string& name,
                                    const Params& params) {
    std::string args = params.build();
    return execute(args.empty() ? name : name + " " + args);
}

// ------------------------------------------------------------------ //
// Auth
// ------------------------------------------------------------------ //
ServerQuery& ServerQuery::login(const std::string& user,
                                const std::string& password,
                                bool strict) {
    auto resp = execute("login " + escape(user) + " " + escape(password));
    int id  = resp.error().id;
    auto msg = resp.error().msg;

    if (id == 0) return *this;

    // TeaSpeak bug #1792 – session stays alive as guest.
    if (id == 1792) {
        if (warn_)
            warn_("Login returned TeaSpeak error 1792 (" + msg +
                  "). Session continues as GUEST; admin ops may fail.");
        return *this;
    }

    if (strict)
        throw AuthenticationError("Login failed (id=" + std::to_string(id) +
                                  "): " + msg);
    return *this;
}

ServerQuery& ServerQuery::apikey(const std::string& key) {
    auto resp = execute("apikey " + escape(key));
    if (!resp.ok())
        throw AuthenticationError("API key auth failed: " + resp.error().msg);
    return *this;
}

ServerQuery& ServerQuery::select_server(int sid) {
    auto resp = execute("use sid=" + std::to_string(sid));
    if (!resp.ok())
        throw CommandError("Failed to select server", resp.error());
    server_id_ = sid;
    return *this;
}

ServerQuery& ServerQuery::nickname(const std::string& name) {
    execute("clientupdate client_nickname=" + escape(name)).raise_for_error();
    {
        std::lock_guard<std::mutex> lk(nickname_mutex_);
        self_nickname_ = name;
    }
    return *this;
}

KeyValueMap ServerQuery::whoami() {
    auto d = execute("whoami").raise_for_error().as_dict();

    if (!d.count("clid")) {
        for (auto& alt : {"invokerid", "client_id", "clientid"}) {
            if (d.count(alt)) { d["clid"] = d[alt]; break; }
        }
    }
    if (!d.count("clid")) {
        if (auto loc = _locate_self()) d = *loc;
    }
    try { client_id_ = std::stoi(d.at("clid")); } catch (...) {}
    return d;
}

std::optional<KeyValueMap> ServerQuery::_locate_self() {
    std::string nick;
    { std::lock_guard<std::mutex> lk(nickname_mutex_); nick = self_nickname_; }
    if (nick.empty()) return std::nullopt;
    try {
        for (auto& c : clientlist()) {
            if (c.count("client_type") && c["client_type"] == "1" &&
                c.count("client_nickname") && c["client_nickname"] == nick &&
                c.count("clid")) {
                try { return clientinfo(std::stoi(c["clid"])); }
                catch (...) { return c; }
            }
        }
    } catch (...) {}
    return std::nullopt;
}

bool ServerQuery::logged_in() {
    try {
        auto me = whoami();
        auto it = me.find("client_login_name");
        return it != me.end() && !it->second.empty();
    } catch (const QueryTimeout&)         { throw; }
      catch (const ConnectionLost&)       { throw; }
      catch (...)                         { return false; }
}

int ServerQuery::client_id() const noexcept { return client_id_.load(); }
int ServerQuery::server_id() const noexcept { return server_id_.load(); }

// ------------------------------------------------------------------ //
// Notifications
// ------------------------------------------------------------------ //
ServerQuery& ServerQuery::register_notify(const std::string& event,
                                          const std::string& specifier) {
    std::string cmd = "servernotifyregister event=" + escape(event);
    if (!specifier.empty()) cmd += " specifier=" + escape(specifier);
    execute(cmd).raise_for_error();
    return *this;
}

ServerQuery& ServerQuery::unregister_notify(const std::string& event,
                                            const std::string& specifier) {
    std::string cmd = "servernotifyunregister event=" + escape(event);
    if (!specifier.empty()) cmd += " specifier=" + escape(specifier);
    execute(cmd).raise_for_error();
    return *this;
}

// ------------------------------------------------------------------ //
// Simple queries
// ------------------------------------------------------------------ //
KeyValueMap ServerQuery::serverinfo()   { return execute("serverinfo").as_dict(); }
KeyValueMap ServerQuery::instanceinfo() { return execute("instanceinfo").as_dict(); }
KeyValueMap ServerQuery::hostinfo()     { return execute("hostinfo").as_dict(); }
KeyValueMap ServerQuery::version()      { return execute("version").as_dict(); }

ItemList    ServerQuery::clientlist()   { return execute("clientlist").as_list(); }
ItemList    ServerQuery::channellist()  { return execute("channellist").as_list(); }
ItemList    ServerQuery::servergrouplist(){ return execute("servergrouplist").as_list(); }
ItemList    ServerQuery::channelgrouplist(){ return execute("channelgrouplist").as_list(); }

KeyValueMap ServerQuery::clientinfo(int clid) {
    return execute("clientinfo clid=" + std::to_string(clid)).as_dict();
}
KeyValueMap ServerQuery::channelinfo(int cid) {
    return execute("channelinfo cid=" + std::to_string(cid)).as_dict();
}

ItemList ServerQuery::clientdblist(int start, int duration) {
    return execute("clientdblist start=" + std::to_string(start) +
                   " duration=" + std::to_string(duration)).as_list();
}
ItemList ServerQuery::clientfind(const std::string& pattern) {
    return execute("clientfind pattern=" + escape(pattern)).as_list();
}

std::optional<KeyValueMap>
ServerQuery::find_channel_by_name(const std::string& name) {
    std::string needle = name;
    for (auto& c : needle) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    for (auto& ch : channellist()) {
        std::string cn = ch["channel_name"];
        for (auto& c : cn) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (cn.find(needle) != std::string::npos) return ch;
    }
    return std::nullopt;
}

int ServerQuery::get_client_database_id(int clid) {
    try { return std::stoi(clientinfo(clid).at("client_database_id")); }
    catch (...) { return 0; }
}

std::vector<int> ServerQuery::get_client_server_groups(int cldbid) {
    std::vector<int> out;
    try {
        for (auto& r : execute("servergroupsbyclientid cldbid=" +
                               std::to_string(cldbid)).as_list())
            if (r.count("sgid")) try { out.push_back(std::stoi(r["sgid"])); } catch (...) {}
    } catch (...) {}
    return out;
}

// ------------------------------------------------------------------ //
// Messages
// ------------------------------------------------------------------ //
ServerQuery& ServerQuery::sendtextmessage(int targetmode, int target,
                                          const std::string& msg) {
    execute("sendtextmessage targetmode=" + std::to_string(targetmode) +
            " target=" + std::to_string(target) +
            " msg=" + escape(msg)).raise_for_error();
    return *this;
}
ServerQuery& ServerQuery::send_private_message(int clid, const std::string& msg) {
    return sendtextmessage(TargetMode::PRIVATE, clid, msg);
}
ServerQuery& ServerQuery::send_channel_message(int cid, const std::string& msg) {
    return sendtextmessage(TargetMode::CHANNEL, cid, msg);
}
ServerQuery& ServerQuery::send_server_message(const std::string& msg) {
    return sendtextmessage(TargetMode::SERVER, 0, msg);
}
ServerQuery& ServerQuery::clientpoke(int clid, const std::string& msg) {
    execute("clientpoke clid=" + std::to_string(clid) +
            " msg=" + escape(msg)).raise_for_error();
    return *this;
}
ServerQuery& ServerQuery::clientmove(int clid, int cid) {
    execute("clientmove clid=" + std::to_string(clid) +
            " cid=" + std::to_string(cid)).raise_for_error();
    return *this;
}
ServerQuery& ServerQuery::clientkick(int clid, int reasonid,
                                     const std::string& reasonmsg) {
    std::string cmd = "clientkick clid=" + std::to_string(clid) +
                      " reasonid=" + std::to_string(reasonid);
    if (!reasonmsg.empty()) cmd += " reasonmsg=" + escape(reasonmsg);
    execute(cmd).raise_for_error();
    return *this;
}

// ------------------------------------------------------------------ //
// Edits
// ------------------------------------------------------------------ //
ServerQuery& ServerQuery::channeledit(int cid, const Params& props) {
    execute("channeledit cid=" + std::to_string(cid) +
            " " + props.build()).raise_for_error();
    return *this;
}
int ServerQuery::channelcreate(const std::string& name, const Params& props) {
    auto resp = command("channelcreate",
                        Params().set("channel_name", name)).raise_for_error();
    if (!props.empty()) {
        auto d = command("channelcreate",
                         Params().set("channel_name", name)
                                 .set_raw("_", "")).as_dict();
        (void)d;
    }
    auto d = resp.as_dict();
    try { return std::stoi(d.at("cid")); } catch (...) { return 0; }
}
ServerQuery& ServerQuery::channeldelete(int cid, bool force) {
    command("channeldelete",
            Params().set("cid", cid).set("force", force)).raise_for_error();
    return *this;
}
ServerQuery& ServerQuery::clientedit(int clid, const Params& props) {
    execute("clientedit clid=" + std::to_string(clid) +
            " " + props.build()).raise_for_error();
    return *this;
}
ServerQuery& ServerQuery::serveredit(const Params& props) {
    execute("serveredit " + props.build()).raise_for_error();
    return *this;
}

// ------------------------------------------------------------------ //
// Permissions
// ------------------------------------------------------------------ //
ServerQuery& ServerQuery::servergroupaddclient(int sgid, int cldbid) {
    execute("servergroupaddclient sgid=" + std::to_string(sgid) +
            " cldbid=" + std::to_string(cldbid)).raise_for_error();
    return *this;
}
ServerQuery& ServerQuery::servergroupdelclient(int sgid, int cldbid) {
    execute("servergroupdelclient sgid=" + std::to_string(sgid) +
            " cldbid=" + std::to_string(cldbid)).raise_for_error();
    return *this;
}
ServerQuery& ServerQuery::clientaddperm(int cldbid, const std::string& permsid,
                                         int permvalue, int permskip) {
    execute("clientaddperm cldbid=" + std::to_string(cldbid) +
            " permsid=" + escape(permsid) +
            " permvalue=" + std::to_string(permvalue) +
            " permskip="  + std::to_string(permskip)).raise_for_error();
    return *this;
}
ServerQuery& ServerQuery::clientdelperm(int cldbid, const std::string& permsid) {
    execute("clientdelperm cldbid=" + std::to_string(cldbid) +
            " permsid=" + escape(permsid)).raise_for_error();
    return *this;
}
int ServerQuery::clientgetdbidfromuid(const std::string& uid) {
    try {
        auto d = execute("clientgetdbidfromuid cluid=" +
                         escape(uid)).as_dict();
        return std::stoi(d.at("cldbid"));
    } catch (...) { return 0; }
}

// ------------------------------------------------------------------ //
// Bans
// ------------------------------------------------------------------ //
ServerQuery& ServerQuery::banadd(const std::string& ip,
                                 const std::string& reason,
                                 int time_seconds) {
    std::string cmd = "banadd ip=" + escape(ip);
    if (!reason.empty()) cmd += " banreason=" + escape(reason);
    if (time_seconds)   cmd += " time=" + std::to_string(time_seconds);
    execute(cmd).raise_for_error();
    return *this;
}
ItemList ServerQuery::banlist() { return execute("banlist").as_list(); }
ServerQuery& ServerQuery::banclient(int clid, int time_seconds,
                                     const std::string& reason) {
    std::string cmd = "banclient clid=" + std::to_string(clid);
    if (time_seconds)  cmd += " time=" + std::to_string(time_seconds);
    if (!reason.empty()) cmd += " banreason=" + escape(reason);
    execute(cmd).raise_for_error();
    return *this;
}

// ------------------------------------------------------------------ //
// Logs
// ------------------------------------------------------------------ //
ItemList ServerQuery::logquery(bool instance, bool reverse, int limit,
                                int offset,
                                std::optional<int> begin,
                                std::optional<int> end,
                                std::optional<int> timestamp) {
    std::string cmd = "logquery instance=" + std::to_string(instance ? 1 : 0) +
                      " reverse="  + std::to_string(reverse ? 1 : 0) +
                      " limit="   + std::to_string(limit) +
                      " offset="  + std::to_string(offset);
    if (begin)     cmd += " begin="     + std::to_string(*begin);
    if (end)       cmd += " end="       + std::to_string(*end);
    if (timestamp) cmd += " timestamp=" + std::to_string(*timestamp);
    return execute(cmd).as_list();
}
ServerQuery& ServerQuery::logadd(int loglevel, const std::string& logmessage) {
    execute("logadd loglevel=" + std::to_string(loglevel) +
            " logmessage=" + escape(logmessage)).raise_for_error();
    return *this;
}

// ------------------------------------------------------------------ //
// Advanced chat
// ------------------------------------------------------------------ //
ItemList ServerQuery::conversationhistory(int cid, const Params& p) {
    return execute("conversationhistory cid=" + std::to_string(cid) +
                   (p.empty() ? "" : " " + p.build())).as_list();
}
ItemList ServerQuery::conversationfetch(int cid, const Params& p) {
    return execute("conversationfetch cid=" + std::to_string(cid) +
                   (p.empty() ? "" : " " + p.build())).as_list();
}
ServerQuery& ServerQuery::conversationmessagedelete(int cid, const Params& p) {
    execute("conversationmessagedelete cid=" + std::to_string(cid) +
            (p.empty() ? "" : " " + p.build())).raise_for_error();
    return *this;
}

// ------------------------------------------------------------------ //
// Feature detection
// ------------------------------------------------------------------ //
ItemList ServerQuery::listfeaturesupport(const Params& filters) {
    return execute("listfeaturesupport" +
                   (filters.empty() ? "" : " " + filters.build())).as_list();
}
KeyValueMap ServerQuery::feature(const std::string& name) {
    return execute("feature name=" + escape(name)).as_dict();
}

// ------------------------------------------------------------------ //
// quick_connect
// ------------------------------------------------------------------ //
std::unique_ptr<ServerQuery> quick_connect(const std::string& host, int port,
                                            const std::string& username,
                                            const std::string& password,
                                            int server_id,
                                            const std::string& nickname,
                                            double timeout) {
    auto q = std::make_unique<ServerQuery>(host, port, timeout);
    q->connect();
    try {
        q->login(username, password);
        q->select_server(server_id);
        if (!nickname.empty()) q->nickname(nickname);
        return q;
    } catch (...) {
        q->disconnect();
        throw;
    }
}

} // namespace teaspeak_query