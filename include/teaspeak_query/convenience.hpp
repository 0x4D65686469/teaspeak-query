#pragma once
#include "client.hpp"
#include "events.hpp"

// ------------------------------------------------------------------
// Generic event handler macro
// ------------------------------------------------------------------
#define TQ_ON(TAG, q, ev) \
    (q).on_event<::teaspeak_query::events::TAG>( \
        [&](const ::teaspeak_query::QueryEvent& ev)

// Text messages - filtered by targetmode automatically
#define TQ_ON_TEXTMESSAGE(q, ev)  TQ_ON(TextMessagePrivate, q, ev)
#define TQ_ON_PM(q, ev)           TQ_ON_TEXTMESSAGE(q, ev)  // shortcut
#define TQ_ON_CHANNEL_MESSAGE(q, ev) TQ_ON(TextMessageChannel, q, ev)
#define TQ_ON_SERVER_MESSAGE(q, ev)  TQ_ON(TextMessageServer,  q, ev)

// Client events
#define TQ_ON_CLIENTENTERVIEW(q, ev) TQ_ON(ClientEnterView, q, ev)
#define TQ_ON_CLIENTLEFTVIEW(q, ev)  TQ_ON(ClientLeftView,  q, ev)
#define TQ_ON_CLIENTMOVED(q, ev)     TQ_ON(ClientMoved,     q, ev)
#define TQ_ON_CLIENTUPDATED(q, ev)   TQ_ON(ClientUpdated,   q, ev)

// Channel events
#define TQ_ON_CHANNELCREATED(q, ev)  TQ_ON(ChannelCreated, q, ev)
#define TQ_ON_CHANNELDELETED(q, ev)  TQ_ON(ChannelDeleted, q, ev)
#define TQ_ON_CHANNELEDITED(q, ev)   TQ_ON(ChannelEdited,  q, ev)
#define TQ_ON_CHANNELMOVED(q, ev)    TQ_ON(ChannelMoved,   q, ev)
#define TQ_ON_CHANNELPASSWORDCHANGED(q, ev) \
    TQ_ON(ChannelPasswordChanged, q, ev)

// Server events
#define TQ_ON_SERVEREDITED(q, ev)    TQ_ON(ServerEdited, q, ev)
#define TQ_ON_SERVERSTART(q, ev)     TQ_ON(ServerStart,  q, ev)
#define TQ_ON_SERVERSTOP(q, ev)      TQ_ON(ServerStop,   q, ev)
#define TQ_ON_TOKENUSED(q, ev)       TQ_ON(TokenUsed,    q, ev)