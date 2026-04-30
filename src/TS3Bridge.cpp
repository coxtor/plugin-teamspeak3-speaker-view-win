#include "TS3Bridge.h"
#include "PluginContext.h"

extern "C" {
#include "plugin_definitions.h"
#include "teamspeak/public_definitions.h"
#include "teamspeak/public_errors.h"
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
