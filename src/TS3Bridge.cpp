#include "TS3Bridge.h"
#include "PluginContext.h"

extern "C" {
#include "plugin_definitions.h"
#include "teamspeak/public_definitions.h"
#include "teamspeak/public_errors.h"
#include "teamspeak/public_rare_definitions.h"
#include "ts3_functions.h"
}

TS3Bridge::TS3Bridge(QObject* parent) : QObject(parent) {}

QString TS3Bridge::nicknameForClient(uint16_t clientID, uint64_t schid) const {
    const TS3Functions* fns = PluginContext::instance().ts3Functions();
    if (!fns) return {};
    char* name = nullptr;
    if (fns->getClientVariableAsString(schid, clientID, CLIENT_NICKNAME, &name) != ERROR_ok
        || !name) {
        return {};
    }
    QString out = QString::fromUtf8(name);
    fns->freeMemory(name);
    return out;
}

uint64_t TS3Bridge::channelForClient(uint16_t clientID, uint64_t schid) const {
    const TS3Functions* fns = PluginContext::instance().ts3Functions();
    if (!fns) return 0;
    uint64 channelID = 0;
    if (fns->getChannelOfClient(schid, clientID, &channelID) != ERROR_ok) return 0;
    return channelID;
}

QString TS3Bridge::channelNameForChannel(uint64_t channelID, uint64_t schid) const {
    const TS3Functions* fns = PluginContext::instance().ts3Functions();
    if (!fns) return {};
    char* name = nullptr;
    if (fns->getChannelVariableAsString(schid, channelID, CHANNEL_NAME, &name) != ERROR_ok
        || !name) {
        return {};
    }
    QString out = QString::fromUtf8(name);
    fns->freeMemory(name);
    return out;
}

uint16_t TS3Bridge::localClientIDOnServer(uint64_t schid) const {
    const TS3Functions* fns = PluginContext::instance().ts3Functions();
    if (!fns) return 0;
    anyID me = 0;
    if (fns->getClientID(schid, &me) != ERROR_ok) return 0;
    return me;
}

uint64_t TS3Bridge::currentServerHandlerID() const {
    const TS3Functions* fns = PluginContext::instance().ts3Functions();
    if (!fns) return 0;
    uint64 schid = fns->getCurrentServerConnectionHandlerID();
    if (schid == 0) return 0;
    int status = 0;
    if (fns->getConnectionStatus(schid, &status) != ERROR_ok) return 0;
    if (status != STATUS_CONNECTION_ESTABLISHED) return 0;
    return schid;
}

int TS3Bridge::toggleSelfFlag(int flag) {
    const TS3Functions* fns = PluginContext::instance().ts3Functions();
    if (!fns) return -1;
    uint64_t schid = currentServerHandlerID();
    if (schid == 0) return -1;
    int current = 0;
    if (fns->getClientSelfVariableAsInt(schid, static_cast<size_t>(flag), &current) != ERROR_ok) return -1;
    int next = current ? 0 : 1;
    if (fns->setClientSelfVariableAsInt(schid, static_cast<size_t>(flag), next) != ERROR_ok) return -1;
    if (fns->flushClientSelfUpdates(schid, nullptr) != ERROR_ok) return -1;
    return next;
}

int TS3Bridge::readSelfFlag(int flag) const {
    const TS3Functions* fns = PluginContext::instance().ts3Functions();
    if (!fns) return -1;
    uint64_t schid = currentServerHandlerID();
    if (schid == 0) return -1;
    int current = 0;
    if (fns->getClientSelfVariableAsInt(schid, static_cast<size_t>(flag), &current) != ERROR_ok) return -1;
    return current ? 1 : 0;
}

int TS3Bridge::toggleSelfMicMuted()     { return toggleSelfFlag(CLIENT_INPUT_MUTED);  }
int TS3Bridge::toggleSelfSpeakerMuted() { return toggleSelfFlag(CLIENT_OUTPUT_MUTED); }

int TS3Bridge::toggleSelfAway() {
    const TS3Functions* fns = PluginContext::instance().ts3Functions();
    if (!fns) return -1;
    uint64_t schid = currentServerHandlerID();
    if (schid == 0) return -1;
    int current = 0;
    if (fns->getClientSelfVariableAsInt(schid, CLIENT_AWAY, &current) != ERROR_ok) return -1;
    int next = (current == AWAY_ZZZ) ? AWAY_NONE : AWAY_ZZZ;
    if (fns->setClientSelfVariableAsInt(schid, CLIENT_AWAY, next) != ERROR_ok) return -1;
    if (fns->flushClientSelfUpdates(schid, nullptr) != ERROR_ok) return -1;
    return (next == AWAY_ZZZ) ? 1 : 0;
}

int TS3Bridge::toggleSelfSilentMode() {
    const TS3Functions* fns = PluginContext::instance().ts3Functions();
    if (!fns) return -1;
    uint64_t schid = currentServerHandlerID();
    if (schid == 0) return -1;
    int micCurrent = 0;
    if (fns->getClientSelfVariableAsInt(schid, CLIENT_INPUT_MUTED, &micCurrent) != ERROR_ok) return -1;
    int next = micCurrent ? 0 : 1;
    int awayValue = next ? AWAY_ZZZ : AWAY_NONE;
    if (fns->setClientSelfVariableAsInt(schid, CLIENT_INPUT_MUTED,  next)      != ERROR_ok) return -1;
    if (fns->setClientSelfVariableAsInt(schid, CLIENT_OUTPUT_MUTED, next)      != ERROR_ok) return -1;
    if (fns->setClientSelfVariableAsInt(schid, CLIENT_AWAY,         awayValue) != ERROR_ok) return -1;
    if (fns->flushClientSelfUpdates(schid, nullptr) != ERROR_ok) return -1;
    return next;
}

int TS3Bridge::selfMicMuted()     const { return readSelfFlag(CLIENT_INPUT_MUTED);  }
int TS3Bridge::selfSpeakerMuted() const { return readSelfFlag(CLIENT_OUTPUT_MUTED); }

int TS3Bridge::selfAway() const {
    int v = readSelfFlag(CLIENT_AWAY);
    if (v < 0) return -1;
    // readSelfFlag normalises non-zero to 1, but CLIENT_AWAY is an enum, so
    // we re-fetch the raw value to compare against AWAY_ZZZ explicitly.
    const TS3Functions* fns = PluginContext::instance().ts3Functions();
    if (!fns) return -1;
    uint64_t schid = currentServerHandlerID();
    if (schid == 0) return -1;
    int current = 0;
    if (fns->getClientSelfVariableAsInt(schid, CLIENT_AWAY, &current) != ERROR_ok) return -1;
    return (current == AWAY_ZZZ) ? 1 : 0;
}

int TS3Bridge::selfSilentMode() const {
    // Mirror Mac: silent-mode visibility tracks mic-mute; the composite
    // toggle keeps all three in sync, but a user might still hit one of
    // them individually, so report the most user-visible flag.
    return selfMicMuted();
}
