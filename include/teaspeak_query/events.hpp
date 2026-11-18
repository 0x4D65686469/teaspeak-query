#pragma once
#include <string>

#include "types.hpp"

namespace teaspeak_query {
namespace events {

// ------------------------------------------------------------------
// Helper macros used inside Event structs
// ------------------------------------------------------------------
#define TQ_FIELD(name) \
    std::string name() const { return raw_.get(#name); }

#define TQ_FIELD_INT(name) \
    int name##_int(int def = 0) const { \
        try { return std::stoi(raw_.get(#name)); } \
        catch (...) { return def; } \
    }

// Shared base for all Event types.
struct BaseEvent {
    const ::teaspeak_query::QueryEvent& raw_;
    explicit BaseEvent(const ::teaspeak_query::QueryEvent& r) : raw_(r) {}

    const ::teaspeak_query::QueryEvent& raw() const { return raw_; }

    std::string get(const std::string& k,
                    const std::string& d = {}) const { return raw_.get(k, d); }

    int as_int(const std::string& k, int def = 0) const {
        try { return std::stoi(raw_.get(k)); }
        catch (...) { return def; }
    }
};

struct TextMessagePrivate {
    static constexpr const char* category = "textprivate";
    static constexpr const char* event    = "textmessage";
    static constexpr const char* filter   = "1";

    struct Event : BaseEvent {
        using BaseEvent::BaseEvent;
        TQ_FIELD(invokerid)
        TQ_FIELD(invokername)
        TQ_FIELD(invokeruid)
        TQ_FIELD(msg)
        TQ_FIELD(targetmode)
    };
};

struct TextMessageChannel {
    static constexpr const char* category = "textchannel";
    static constexpr const char* event    = "textmessage";
    static constexpr const char* filter   = "2";

    struct Event : BaseEvent {
        using BaseEvent::BaseEvent;
        TQ_FIELD(invokerid)
        TQ_FIELD(invokername)
        TQ_FIELD(invokeruid)
        TQ_FIELD(msg)
        TQ_FIELD(targetmode)
    };
};

struct TextMessageServer {
    static constexpr const char* category = "textserver";
    static constexpr const char* event    = "textmessage";
    static constexpr const char* filter   = "3";

    struct Event : BaseEvent {
        using BaseEvent::BaseEvent;
        TQ_FIELD(invokerid)
        TQ_FIELD(invokername)
        TQ_FIELD(invokeruid)
        TQ_FIELD(msg)
        TQ_FIELD(targetmode)
    };
};

struct ClientEnterView {
    static constexpr const char* category = "server";
    static constexpr const char* event    = "cliententerview";
    static constexpr const char* filter   = nullptr;

    struct Event : BaseEvent {
        using BaseEvent::BaseEvent;
        TQ_FIELD(clid)                       TQ_FIELD_INT(clid)
        TQ_FIELD(cid)                        TQ_FIELD_INT(cid)
        TQ_FIELD(client_nickname)
        TQ_FIELD(client_unique_identifier)
        TQ_FIELD(client_database_id)         TQ_FIELD_INT(client_database_id)
        TQ_FIELD(client_type)                TQ_FIELD_INT(client_type)
        TQ_FIELD(client_country)
        TQ_FIELD(client_away)                TQ_FIELD_INT(client_away)
        TQ_FIELD(client_away_message)
        TQ_FIELD(client_input_muted)         TQ_FIELD_INT(client_input_muted)
        TQ_FIELD(client_output_muted)        TQ_FIELD_INT(client_output_muted)
        TQ_FIELD(client_is_recording)        TQ_FIELD_INT(client_is_recording)
        TQ_FIELD(client_is_priority_speaker) TQ_FIELD_INT(client_is_priority_speaker)
        TQ_FIELD(client_is_channel_commander)TQ_FIELD_INT(client_is_channel_commander)
        TQ_FIELD(client_channel_group_id)    TQ_FIELD_INT(client_channel_group_id)
        TQ_FIELD(client_servergroups)
        TQ_FIELD(client_created)             TQ_FIELD_INT(client_created)
        TQ_FIELD(client_lastconnected)       TQ_FIELD_INT(client_lastconnected)
        TQ_FIELD(client_icon_id)             TQ_FIELD_INT(client_icon_id)
        TQ_FIELD(client_platform)
        TQ_FIELD(client_version)
        TQ_FIELD(client_description)
        TQ_FIELD(cfid)                       TQ_FIELD_INT(cfid)
        TQ_FIELD(ctid)                       TQ_FIELD_INT(ctid)
        TQ_FIELD(reasonid)                   TQ_FIELD_INT(reasonid)
    };
};

struct ClientLeftView {
    static constexpr const char* category = "server";
    static constexpr const char* event    = "clientleftview";
    static constexpr const char* filter   = nullptr;

    struct Event : BaseEvent {
        using BaseEvent::BaseEvent;
        TQ_FIELD(clid)                       TQ_FIELD_INT(clid)
        TQ_FIELD(cid)                        TQ_FIELD_INT(cid)
        TQ_FIELD(client_nickname)
        TQ_FIELD(cfid)                       TQ_FIELD_INT(cfid)
        TQ_FIELD(ctid)                       TQ_FIELD_INT(ctid)
        TQ_FIELD(reasonid)                   TQ_FIELD_INT(reasonid)
        TQ_FIELD(reasonmsg)
        TQ_FIELD(invokerid)                  TQ_FIELD_INT(invokerid)
        TQ_FIELD(invokername)
        TQ_FIELD(invokeruid)
    };
};

struct ClientMoved {
    static constexpr const char* category = "server";
    static constexpr const char* event    = "clientmoved";
    static constexpr const char* filter   = nullptr;

    struct Event : BaseEvent {
        using BaseEvent::BaseEvent;
        TQ_FIELD(clid)                       TQ_FIELD_INT(clid)
        TQ_FIELD(cid)                        TQ_FIELD_INT(cid)
        TQ_FIELD(ctid)                       TQ_FIELD_INT(ctid)
        TQ_FIELD(invokerid)                  TQ_FIELD_INT(invokerid)
        TQ_FIELD(invokername)
        TQ_FIELD(invokeruid)
        TQ_FIELD(reasonid)                   TQ_FIELD_INT(reasonid)
    };
};

struct ClientUpdated {
    static constexpr const char* category = "server";
    static constexpr const char* event    = "clientupdated";
    static constexpr const char* filter   = nullptr;

    struct Event : BaseEvent {
        using BaseEvent::BaseEvent;
        // Fields that actually change vary; expose the common ones.
        TQ_FIELD(clid)                       TQ_FIELD_INT(clid)
        TQ_FIELD(cid)                        TQ_FIELD_INT(cid)
        TQ_FIELD(client_nickname)
        TQ_FIELD(client_away)                TQ_FIELD_INT(client_away)
        TQ_FIELD(client_away_message)
        TQ_FIELD(client_input_muted)         TQ_FIELD_INT(client_input_muted)
        TQ_FIELD(client_output_muted)        TQ_FIELD_INT(client_output_muted)
        TQ_FIELD(client_is_recording)        TQ_FIELD_INT(client_is_recording)
        TQ_FIELD(client_is_priority_speaker) TQ_FIELD_INT(client_is_priority_speaker)
        TQ_FIELD(client_is_channel_commander)TQ_FIELD_INT(client_is_channel_commander)
        TQ_FIELD(client_is_talker)           TQ_FIELD_INT(client_is_talker)
        TQ_FIELD(client_talk_power)          TQ_FIELD_INT(client_talk_power)
        TQ_FIELD(client_description)
    };
};

struct ChannelCreated {
    static constexpr const char* category = "server";
    static constexpr const char* event    = "channelcreated";
    static constexpr const char* filter   = nullptr;

    struct Event : BaseEvent {
        using BaseEvent::BaseEvent;
        TQ_FIELD(cid)                        TQ_FIELD_INT(cid)
        TQ_FIELD(cpid)                       TQ_FIELD_INT(cpid)
        TQ_FIELD(channel_name)
        TQ_FIELD(channel_order)              TQ_FIELD_INT(channel_order)
        TQ_FIELD(channel_maxclients)         TQ_FIELD_INT(channel_maxclients)
        TQ_FIELD(channel_maxfamilyclients)   TQ_FIELD_INT(channel_maxfamilyclients)
        TQ_FIELD(channel_topic)
        TQ_FIELD(channel_description)
        TQ_FIELD(channel_flag_default)       TQ_FIELD_INT(channel_flag_default)
        TQ_FIELD(channel_flag_password)      TQ_FIELD_INT(channel_flag_password)
        TQ_FIELD(channel_flag_permanent)     TQ_FIELD_INT(channel_flag_permanent)
        TQ_FIELD(channel_flag_semi_permanent)TQ_FIELD_INT(channel_flag_semi_permanent)
        TQ_FIELD(channel_flag_temporary)     TQ_FIELD_INT(channel_flag_temporary)
        TQ_FIELD(invokerid)                  TQ_FIELD_INT(invokerid)
        TQ_FIELD(invokername)
        TQ_FIELD(invokeruid)
    };
};

struct ChannelDeleted {
    static constexpr const char* category = "server";
    static constexpr const char* event    = "channeldeleted";
    static constexpr const char* filter   = nullptr;

    struct Event : BaseEvent {
        using BaseEvent::BaseEvent;
        TQ_FIELD(cid)                        TQ_FIELD_INT(cid)
        TQ_FIELD(invokerid)                  TQ_FIELD_INT(invokerid)
        TQ_FIELD(invokername)
        TQ_FIELD(invokeruid)
    };
};

struct ChannelEdited {
    static constexpr const char* category = "server";
    static constexpr const char* event    = "channeledited";
    static constexpr const char* filter   = nullptr;

    struct Event : BaseEvent {
        using BaseEvent::BaseEvent;
        TQ_FIELD(cid)                        TQ_FIELD_INT(cid)
        TQ_FIELD(channel_name)
        TQ_FIELD(channel_topic)
        TQ_FIELD(channel_description)
        TQ_FIELD(invokerid)                  TQ_FIELD_INT(invokerid)
        TQ_FIELD(invokername)
        TQ_FIELD(invokeruid)
    };
};

struct ChannelMoved {
    static constexpr const char* category = "server";
    static constexpr const char* event    = "channelmoved";
    static constexpr const char* filter   = nullptr;

    struct Event : BaseEvent {
        using BaseEvent::BaseEvent;
        TQ_FIELD(cid)                        TQ_FIELD_INT(cid)
        TQ_FIELD(cpid)                       TQ_FIELD_INT(cpid)
        TQ_FIELD(order)                      TQ_FIELD_INT(order)
        TQ_FIELD(invokerid)                  TQ_FIELD_INT(invokerid)
        TQ_FIELD(invokername)
        TQ_FIELD(invokeruid)
    };
};

struct ChannelPasswordChanged {
    static constexpr const char* category = "server";
    static constexpr const char* event    = "channelpasswordchanged";
    static constexpr const char* filter   = nullptr;

    struct Event : BaseEvent {
        using BaseEvent::BaseEvent;
        TQ_FIELD(cid)                        TQ_FIELD_INT(cid)
        TQ_FIELD(invokerid)                  TQ_FIELD_INT(invokerid)
        TQ_FIELD(invokername)
        TQ_FIELD(invokeruid)
    };
};

struct ServerEdited {
    static constexpr const char* category = "server";
    static constexpr const char* event    = "serveredited";
    static constexpr const char* filter   = nullptr;

    struct Event : BaseEvent {
        using BaseEvent::BaseEvent;
        // Variable field set depending on what was edited.
        TQ_FIELD(invokerid)                  TQ_FIELD_INT(invokerid)
        TQ_FIELD(invokername)
        TQ_FIELD(invokeruid)
    };
};

struct ServerStart {
    static constexpr const char* category = "server";
    static constexpr const char* event    = "serverstart";
    static constexpr const char* filter   = nullptr;

    struct Event : BaseEvent {
        using BaseEvent::BaseEvent;
        // no common fields
    };
};

struct ServerStop {
    static constexpr const char* category = "server";
    static constexpr const char* event    = "serverstop";
    static constexpr const char* filter   = nullptr;

    struct Event : BaseEvent {
        using BaseEvent::BaseEvent;
        // no common fields
    };
};

struct TokenUsed {
    static constexpr const char* category = "server";
    static constexpr const char* event    = "tokenused";
    static constexpr const char* filter   = nullptr;

    struct Event : BaseEvent {
        using BaseEvent::BaseEvent;
        TQ_FIELD(clid)                       TQ_FIELD_INT(clid)
        TQ_FIELD(cldbid)                     TQ_FIELD_INT(cldbid)
        TQ_FIELD(token)
        TQ_FIELD(tokentype)                  TQ_FIELD_INT(tokentype)
        TQ_FIELD(tokenid1)                   TQ_FIELD_INT(tokenid1)
        TQ_FIELD(tokenid2)                   TQ_FIELD_INT(tokenid2)
        TQ_FIELD(tokendescription)
        TQ_FIELD(invokerid)                  TQ_FIELD_INT(invokerid)
        TQ_FIELD(invokername)
        TQ_FIELD(invokeruid)
    };
};

#undef TQ_FIELD
#undef TQ_FIELD_INT

} // namespace events
} // namespace teaspeak_query