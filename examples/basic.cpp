#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <teaspeak_query/teaspeak_query.hpp>

using namespace teaspeak_query;

namespace {

struct Args {
    std::string host        = "127.0.0.1";
    int         port        = 10101;
    std::string username    = "serveradmin";
    std::string password;
    std::string apikey;
    int         server_id   = 1;
    std::string nickname    = "ExampleBot";
    int         target_clid = 0;
    std::string message;
    bool        interactive = true;
    bool        verbose     = false;
};

void usage(const char* exe) {
    std::cout <<
        "usage: " << exe << " [options]\n"
        "\n"
        "  -H, --host <h>          server host     (default 127.0.0.1)\n"
        "  -P, --port <n>          query port      (default 10101)\n"
        "  -u, --user <u>          login name      (default serveradmin)\n"
        "  -p, --pass <p>          login password  (falls back to env TQ_PASS)\n"
        "  -k, --apikey <k>        use query API key instead of user/pass\n"
        "  -s, --sid <n>           virtual server id (default 1)\n"
        "  -n, --nick <n>          bot nickname    (default ExampleBot)\n"
        "  -t, --to <clid>         send a private message to this client id\n"
        "  -m, --msg <text>        message body (required with --to)\n"
        "  -q, --no-interactive    do not wait for Enter; sleep 3s instead\n"
        "  -v, --verbose           print every incoming notification\n"
        "  -h, --help              show this help and exit\n";
}

bool needs_value(const std::string& a) {
    return a == "-H" || a == "--host"   ||
           a == "-P" || a == "--port"   ||
           a == "-u" || a == "--user"   ||
           a == "-p" || a == "--pass"   ||
           a == "-k" || a == "--apikey" ||
           a == "-s" || a == "--sid"    ||
           a == "-n" || a == "--nick"   ||
           a == "-t" || a == "--to"     ||
           a == "-m" || a == "--msg";
}

bool parse(int argc, char** argv, Args& out) {
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];

        if (a == "-h" || a == "--help") { usage(argv[0]); return false; }

        if (needs_value(a) && i + 1 >= argc) {
            std::cerr << "error: missing value for " << a << "\n";
            usage(argv[0]);
            return false;
        }

        auto next = [&]() -> std::string { return argv[++i]; };

        try {
            if      (a == "-H" || a == "--host")   out.host        = next();
            else if (a == "-P" || a == "--port")   out.port        = std::stoi(next());
            else if (a == "-u" || a == "--user")   out.username    = next();
            else if (a == "-p" || a == "--pass")   out.password    = next();
            else if (a == "-k" || a == "--apikey") out.apikey      = next();
            else if (a == "-s" || a == "--sid")    out.server_id   = std::stoi(next());
            else if (a == "-n" || a == "--nick")   out.nickname    = next();
            else if (a == "-t" || a == "--to")     out.target_clid = std::stoi(next());
            else if (a == "-m" || a == "--msg")    out.message     = next();
            else if (a == "-q" || a == "--no-interactive") out.interactive = false;
            else if (a == "-v" || a == "--verbose")        out.verbose     = true;
            else {
                std::cerr << "error: unknown option: " << a << "\n";
                usage(argv[0]);
                return false;
            }
        } catch (const std::exception&) {
            std::cerr << "error: invalid value for " << a << "\n";
            return false;
        }
    }
    return true;
}

} // namespace

int main(int argc, char** argv) {
    Args args;
    if (!parse(argc, argv, args)) return 1;

    if (args.apikey.empty() && args.password.empty()) {
        if (const char* env = std::getenv("TQ_PASS")) args.password = env;
    }
    if (args.apikey.empty() && args.password.empty()) {
        std::cerr << "error: no credentials. Use --apikey, --pass, or set TQ_PASS.\n";
        return 1;
    }
    if (args.target_clid > 0 && args.message.empty()) {
        std::cerr << "error: --to given but --msg is empty.\n";
        return 1;
    }

    try {
        ServerQuery q(args.host, args.port);
        q.set_warning_handler([&](const std::string& w) {
            std::cerr << "[warn] " << w << "\n";
        });

        q.connect();

        if (!args.apikey.empty()) {
            q.apikey(args.apikey);
        } else {
            q.login(args.username, args.password);
        }

        q.select_server(args.server_id);
        q.nickname(args.nickname);

        std::cout << "logged_in = " << std::boolalpha << q.logged_in() << "\n";
        std::cout << "server    = " << q.serverinfo().at("virtualserver_name") << "\n";
        std::cout << "clients:\n";
        for (auto& c : q.clientlist())
            std::cout << "  " << c["clid"] << "  " << c["client_nickname"] << "\n";

        if (args.target_clid > 0) {
            q.send_private_message(args.target_clid, args.message);
            std::cout << "sent message to clid=" << args.target_clid << "\n";
        }

        // Register notifications and handlers.
        q.register_notify("textprivate");
        q.on("textmessage", [](const QueryEvent& ev) {
            std::cout << "[event] " << ev.get("invokername")
                      << " -> " << ev.get("msg") << "\n";
        });
        if (args.verbose) {
            q.on_any([](const QueryEvent& ev) {
                std::cout << "[any] " << ev.raw_name << "\n";
            });
        }

        if (args.interactive) {
            std::cout << "Press Enter to quit...\n";
            std::cin.get();
        } else {
            // Give the server a short window to deliver events.
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }

        q.disconnect();
    } catch (const AuthenticationError& e) {
        std::cerr << "auth error: " << e.what() << "\n";
        return 1;
    } catch (const CommandError& e) {
        std::cerr << "command error " << e.error_id()
                  << ": " << e.error_msg() << "\n";
        return 1;
    } catch (const QueryError& e) {
        std::cerr << "query error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}