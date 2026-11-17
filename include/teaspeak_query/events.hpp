#pragma once

namespace teaspeak_query {
namespace events {

// ------------------------------------------------------------------
// Event table (X-macro)
//
// Each row is: (TagName, notify_category, event_name, targetmode_filter)
//
// To add a new event: append one row. Nothing else.
// ------------------------------------------------------------------

#define TQ_EVENT_TABLE(X) \
    X(TextMessagePrivate,     "textprivate", "textmessage",         "1")     \
    X(TextMessageChannel,     "textchannel", "textmessage",         "2")     \
    X(TextMessageServer,      "textserver",  "textmessage",         "3")     \
    X(ClientEnterView,        "server",      "cliententerview",     nullptr) \
    X(ClientLeftView,         "server",      "clientleftview",      nullptr) \
    X(ClientMoved,            "server",      "clientmoved",         nullptr) \
    X(ClientUpdated,          "server",      "clientupdated",       nullptr) \
    X(ChannelCreated,         "server",      "channelcreated",      nullptr) \
    X(ChannelDeleted,         "server",      "channeldeleted",      nullptr) \
    X(ChannelEdited,          "server",      "channeledited",       nullptr) \
    X(ChannelMoved,           "server",      "channelmoved",        nullptr) \
    X(ChannelPasswordChanged, "server",      "channelpasswordchanged", nullptr) \
    X(ServerEdited,           "server",      "serveredited",        nullptr) \
    X(ServerStart,            "server",      "serverstart",         nullptr) \
    X(ServerStop,             "server",      "serverstop",          nullptr) \
    X(TokenUsed,              "server",      "tokenused",           nullptr)

#define TQ_DEFINE_EVENT_TAG(NAME, CAT, EVENT, FILTER) \
    struct NAME { \
        static constexpr const char* category = CAT;   \
        static constexpr const char* event    = EVENT; \
        static constexpr const char* filter   = FILTER; \
    };

TQ_EVENT_TABLE(TQ_DEFINE_EVENT_TAG)
#undef TQ_DEFINE_EVENT_TAG

} // namespace events
} // namespace teaspeak_query