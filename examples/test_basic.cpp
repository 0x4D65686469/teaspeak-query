#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "env.hpp"
#include <teaspeak_query/teaspeak_query.hpp>

using namespace teaspeak_query;

namespace {

struct Args {
    std::string host     = "127.0.0.1";
    int         port     = 10101;
    std::string username = "serveradmin";
    std::string password;
    std::string apikey;
    int         sid      = 1;
    int         watch_s  = 20;
};

void usage(const char* exe) {
    std::cout <<
        "usage: " << exe << " [options]\n"
        "\n"
        "  -H, --host <h>      server host     (default 127.0.0.1)\n"
        "  -P, --port <n>      query port      (default 10101)\n"
        "  -u, --user <u>      login name      (default serveradmin)\n"
        "  -p, --pass <p>      login password  (falls back to env TQ_PASS)\n"
        "  -k, --apikey <k>    use query API key instead of user/pass\n"
        "  -s, --sid <n>       virtual server id (default 1)\n"
        "  -w, --watch <sec>   listen for events for N seconds (default 20)\n"
        "  -h, --help          show this help and exit\n";
}

bool needs_value(const std::string& a) {
    return a == "-H" || a == "--host"   ||
           a == "-P" || a == "--port"   ||
           a == "-u" || a == "--user"   ||
           a == "-p" || a == "--pass"   ||
           a == "-k" || a == "--apikey" ||
           a == "-s" || a == "--sid"    ||
           a == "-w" || a == "--watch";
}

bool parse(int argc, char** argv, Args& out) {
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-h" || a == "--help") { usage(argv[0]); return false; }
        if (needs_value(a) && i + 1 >= argc) {
            std::cerr << "error: missing value for " << a << "\n";
            return false;
        }
        auto next = [&]() -> std::string { return argv[++i]; };
        try {
            if      (a == "-H" || a == "--host")   out.host     = next();
            else if (a == "-P" || a == "--port")   out.port     = std::stoi(next());
            else if (a == "-u" || a == "--user")   out.username = next();
            else if (a == "-p" || a == "--pass")   out.password = next();
            else if (a == "-k" || a == "--apikey") out.apikey   = next();
            else if (a == "-s" || a == "--sid")    out.sid      = std::stoi(next());
            else if (a == "-w" || a == "--watch")  out.watch_s  = std::stoi(next());
            else {
                std::cerr << "error: unknown option: " << a << "\n";
                return false;
            }
        } catch (...) {
            std::cerr << "error: invalid value for " << a << "\n";
            return false;
        }
    }
    return true;
}

int g_pass = 0;
int g_fail = 0;

void pass(const std::string& name, const std::string& detail = {}) {
    ++g_pass;
    std::cout << "[PASS] " << name;
    if (!detail.empty()) std::cout << "  (" << detail << ")";
    std::cout << "\n";
}

void fail(const std::string& name, const std::string& detail = {}) {
    ++g_fail;
    std::cout << "[FAIL] " << name;
    if (!detail.empty()) std::cout << "  (" << detail << ")";
    std::cout << "\n";
}

void skip(const std::string& name, const std::string& detail = {}) {
    std::cout << "[SKIP] " << name;
    if (!detail.empty()) std::cout << "  (" << detail << ")";
    std::cout << "\n";
}

} // namespace

int main(int argc, char** argv) {
    Args args;
    if (!parse(argc, argv, args)) return 1;

    if (args.apikey.empty() && args.password.empty()) {
        if (auto v = ex::get_env("TQ_PASS")) args.password = *v;
    }
    if (args.apikey.empty() && args.password.empty()) {
        std::cerr << "error: no credentials. Use --apikey, --pass, or set TQ_PASS.\n";
        return 1;
    }

    std::cout << "=== teaspeak-query basic test ===\n";
    std::cout << "target: " << args.host << ":" << args.port
              << "  sid=" << args.sid << "\n\n";

    ServerQuery q(args.host, args.port);
    q.set_warning_handler([](const std::string& w) {
        std::cout << "  [warn] " << w << "\n";
    });

    // 1. connect
    try {
        q.connect();
        pass("connect");
    } catch (const std::exception& e) {
        fail("connect", e.what());
        std::cout << "\nresult: " << g_pass << " passed, " << g_fail
                  << " failed\n";
        return 1;
    }

    // 2. auth
    try {
        if (!args.apikey.empty()) {
            q.apikey(args.apikey);
            pass("apikey auth");
        } else {
            q.login(args.username, args.password);
            if (q.logged_in()) pass("login");
            else               pass("login", "guest session (TeaSpeak 1792)");
        }
    } catch (const std::exception& e) {
        fail("auth", e.what());
    }

    // 3. select server
    try {
        q.select_server(args.sid);
        pass("select_server", "sid=" + std::to_string(args.sid));
    } catch (const std::exception& e) {
        fail("select_server", e.what());
    }

    // 4. nickname
    const std::string nick = "TestBot-" + std::to_string(std::rand() % 10000);
    try {
        q.nickname(nick);
        pass("nickname", nick);
    } catch (const CommandError& e) {
        if (e.error_id() == 2568) skip("nickname", "no permission (guest)");
        else                      fail("nickname", e.error_msg());
    } catch (const std::exception& e) {
        fail("nickname", e.what());
    }

    // 5. serverinfo
    try {
        auto info = q.serverinfo();
        auto it = info.find("virtualserver_name");
        if (it != info.end() && !it->second.empty())
            pass("serverinfo", it->second);
        else
            fail("serverinfo", "no virtualserver_name in response");
    } catch (const std::exception& e) {
        fail("serverinfo", e.what());
    }

    // 6. clientlist
    try {
        auto clients = q.clientlist();
        pass("clientlist", std::to_string(clients.size()) + " clients");

        int shown = 0;
        for (auto& c : clients) {
            auto id  = c.find("clid");
            auto nm  = c.find("client_nickname");
            if (id == c.end() || nm == c.end()) continue;
            std::cout << "         " << id->second << "  " << nm->second << "\n";
            if (++shown >= 10) {
                if (clients.size() > 10)
                    std::cout << "         ... (" << (clients.size() - 10)
                              << " more)\n";
                break;
            }
        }
    } catch (const std::exception& e) {
        fail("clientlist", e.what());
    }

    // 7. banlist
    try {
        auto bans = q.banlist();
        pass("banlist", std::to_string(bans.size()) + " bans");

        int shown = 0;
        for (auto& b : bans) {
            auto ip = b.find("ip");
            auto id = b.find("banid");
            if (ip == b.end()) continue;
            std::cout << "         banid=" << (id != b.end() ? id->second : "?")
                      << "  ip=" << ip->second << "\n";
            if (++shown >= 5) {
                if (bans.size() > 5)
                    std::cout << "         ... (" << (bans.size() - 5)
                              << " more)\n";
                break;
            }
        }
    } catch (const CommandError& e) {
        if (e.error_id() == 2568) skip("banlist", "no permission (guest)");
        else                      fail("banlist", e.error_msg());
    } catch (const std::exception& e) {
        fail("banlist", e.what());
    }

    // 8. events — register, wait a few seconds, count
    std::atomic<int> events{0};
    std::string      last_raw;

    try {
        q.on_any([&](const QueryEvent& ev) {
            events++;
            last_raw = ev.raw_name;
            std::cout << "         [event] " << ev.raw_name << "\n";
        });

        q.register_notify("textprivate");
        q.register_notify("textchannel");
        q.register_notify("server");
        pass("register_notify", "textprivate + textchannel + server");

        std::cout << "\n  listening for events for " << args.watch_s
                  << "s — send a message from any client now...\n";
        std::this_thread::sleep_for(std::chrono::seconds(args.watch_s));

        if (events.load() > 0)
            pass("events received", std::to_string(events.load()) +
                                    " events, last=" + last_raw);
        else
            skip("events received",
                 "no events in " + std::to_string(args.watch_s) +
                 "s (nobody chatted, or guest cannot subscribe)");
    } catch (const CommandError& e) {
        if (e.error_id() == 2568) skip("register_notify", "no permission (guest)");
        else                      fail("register_notify", e.error_msg());
    } catch (const std::exception& e) {
        fail("events", e.what());
    }

    // 9. disconnect
    try {
        q.disconnect();
        pass("disconnect");
    } catch (const std::exception& e) {
        fail("disconnect", e.what());
    }

    // result
    std::cout << "\n=== result: " << g_pass << " passed, "
              << g_fail << " failed ===\n";
    return g_fail == 0 ? 0 : 1;
}