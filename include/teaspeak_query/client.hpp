#pragma once
#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>
#include <vector>
#include <map>

#include "params.hpp"
#include "types.hpp"

namespace teaspeak_query {

namespace detail { class Socket; }

class ServerQuery {
public:
    using EventCallback = std::function<void(const QueryEvent&)>;

    explicit ServerQuery(std::string host,
                         int    port          = 10101,
                         double timeout       = 15.0,
                         bool   auto_dispatch = true);
    ~ServerQuery();

    ServerQuery(const ServerQuery&)            = delete;
    ServerQuery& operator=(const ServerQuery&) = delete;

    // ---------------- lifecycle ----------------
    ServerQuery& connect();
    void         disconnect();
    bool         connected() const noexcept;

    // ---------------- config ------------------
    void set_warning_handler(std::function<void(const std::string&)> h);

    // ---------------- auth --------------------
    ServerQuery& login(const std::string& user,
                       const std::string& password,
                       bool               strict = true);
    ServerQuery& apikey(const std::string& key);
    ServerQuery& select_server(int sid);
    ServerQuery& nickname(const std::string& name);
    bool         logged_in();
    KeyValueMap  whoami();

    int  client_id() const noexcept;
    int  server_id() const noexcept;

    // ---------------- events ------------------
    ServerQuery& register_notify(const std::string& event,
                                 const std::string& specifier = {});
    ServerQuery& unregister_notify(const std::string& event,
                                   const std::string& specifier = {});
    ServerQuery& on(const std::string& event_name, EventCallback cb);
    ServerQuery& on_any(EventCallback cb);

    std::optional<QueryEvent> poll(int timeout_ms = 500);
    void listen(std::atomic<bool>* stop = nullptr);

    // ---------------- raw commands ------------
    QueryResponse execute(const std::string& command,
                          double timeout = -1.0);
    QueryResponse command(const std::string& name,
                          const Params&     params = {});

    // ---------------- common queries ----------
    KeyValueMap serverinfo();
    KeyValueMap instanceinfo();
    KeyValueMap hostinfo();
    KeyValueMap version();

    ItemList      clientlist();
    KeyValueMap   clientinfo(int clid);
    ItemList      channellist();
    KeyValueMap   channelinfo(int cid);
    ItemList      servergrouplist();
    ItemList      channelgrouplist();
    ItemList      clientdblist(int start = 0, int duration = 100);
    ItemList      clientfind(const std::string& pattern);

    std::optional<KeyValueMap> find_channel_by_name(const std::string& name);
    int          get_client_database_id(int clid);
    std::vector<int> get_client_server_groups(int cldbid);

    // ---------------- messages / pokes --------
    ServerQuery& sendtextmessage(int targetmode, int target, const std::string& msg);
    ServerQuery& send_private_message(int clid, const std::string& msg);
    ServerQuery& send_channel_message(int cid,  const std::string& msg);
    ServerQuery& send_server_message (const std::string& msg);
    ServerQuery& clientpoke (int clid, const std::string& msg);
    ServerQuery& clientmove (int clid, int cid);
    ServerQuery& clientkick (int clid,
                             int reasonid  = ReasonID::KICK_SERVER,
                             const std::string& reasonmsg = {});

    // ---------------- edits -------------------
    ServerQuery& channeledit(int cid, const Params& props);
    int          channelcreate(const std::string& name, const Params& props = {});
    ServerQuery& channeldelete(int cid, bool force = true);
    ServerQuery& clientedit(int clid, const Params& props);
    ServerQuery& serveredit(const Params& props);

    // ---------------- permissions -------------
    ServerQuery& servergroupaddclient(int sgid, int cldbid);
    ServerQuery& servergroupdelclient(int sgid, int cldbid);
    ServerQuery& clientaddperm(int cldbid, const std::string& permsid,
                               int permvalue = 1, int permskip = 0);
    ServerQuery& clientdelperm(int cldbid, const std::string& permsid);
    int          clientgetdbidfromuid(const std::string& uid);

    // ---------------- bans --------------------
    ServerQuery& banadd(const std::string& ip,
                        const std::string& reason = {},
                        int                time_seconds = 0);
    ItemList     banlist();
    ServerQuery& banclient(int clid, int time_seconds = 0,
                           const std::string& reason = {});

    // ---------------- logs --------------------
    ItemList logquery(bool instance = true, bool reverse = false,
                      int limit = 100, int offset = 0,
                      std::optional<int> begin  = std::nullopt,
                      std::optional<int> end    = std::nullopt,
                      std::optional<int> timestamp = std::nullopt);
    ServerQuery& logadd(int loglevel, const std::string& logmessage);

    // ---------------- advanced chat -----------
    ItemList     conversationhistory(int cid, const Params& params = {});
    ItemList     conversationfetch  (int cid, const Params& params = {});
    ServerQuery& conversationmessagedelete(int cid, const Params& params = {});

    // ---------------- features ----------------
    ItemList    listfeaturesupport(const Params& filters = {});
    KeyValueMap feature(const std::string& name);

private:
    void _read_loop();
    void _dispatch_loop();
    void _read_greeting();
    void _handle_notification(const std::string& line);
    void _dispatch_event(const QueryEvent& ev);
    void _ensure_dispatcher();
    std::optional<KeyValueMap> _locate_self();

    std::string host_;
    int         port_;
    double      timeout_;
    bool        auto_dispatch_;

    std::unique_ptr<detail::Socket> sock_;
    std::atomic<bool>               connected_{false};
    std::atomic<bool>               stop_flag_{false};

    std::mutex send_mutex_;

    std::mutex              response_mutex_;
    std::condition_variable response_cv_;
    std::vector<std::string>    response_data_;
    std::vector<QueryErrorInfo> response_errors_;
    bool                        response_ready_ = false;

    std::queue<QueryEvent> event_queue_;
    std::mutex             event_mutex_;
    std::condition_variable event_cv_;

    std::mutex                                handlers_mutex_;
    std::map<std::string, std::vector<EventCallback>> handlers_;
    std::vector<EventCallback>                any_handlers_;

    std::thread reader_thread_;
    std::thread dispatcher_thread_;

    std::atomic<int> client_id_{-1};
    std::atomic<int> server_id_{0};
    std::string      self_nickname_;
    std::mutex       nickname_mutex_;

    std::function<void(const std::string&)> warn_;
};

// ---------------------------------------------------------------- //
// Convenience helper
// ---------------------------------------------------------------- //
std::unique_ptr<ServerQuery> quick_connect(
    const std::string& host,
    int port = 10101,
    const std::string& username  = "serveradmin",
    const std::string& password  = {},
    int                server_id = 1,
    const std::string& nickname  = {},
    double             timeout   = 15.0);

} // namespace teaspeak_query