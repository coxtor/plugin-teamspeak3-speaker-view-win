#include "TS3Bridge.h"
#include "PluginContext.h"

#include <QtCore/QByteArray>
#include <QtCore/QStringList>

#include <string>
#include <vector>

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

uint64_t TS3Bridge::resolveChannelPath(const QString& path) const {
    if (path.isEmpty()) return 0;
    const TS3Functions* fns = PluginContext::instance().ts3Functions();
    if (!fns) return 0;
    uint64_t schid = currentServerHandlerID();
    if (schid == 0) return 0;

    // getChannelIDFromChannelNames takes a NULL-terminated char** of UTF-8
    // segments, e.g. {"Lobby", "AFK", NULL}.
    QStringList parts = path.split('/', Qt::SkipEmptyParts);
    if (parts.isEmpty()) return 0;

    std::vector<QByteArray> storage;
    storage.reserve(static_cast<size_t>(parts.size()));
    std::vector<char*> argv;
    argv.reserve(static_cast<size_t>(parts.size()) + 1);
    for (const QString& seg : parts) {
        QString trimmed = seg.trimmed();
        if (trimmed.isEmpty()) continue;
        storage.emplace_back(trimmed.toUtf8());
        argv.push_back(storage.back().data());
    }
    if (argv.empty()) return 0;
    argv.push_back(nullptr);

    uint64 result = 0;
    if (fns->getChannelIDFromChannelNames(schid, argv.data(), &result) != ERROR_ok) {
        return 0;
    }
    return result;
}

int TS3Bridge::moveSelfToChannelID(uint64_t channelID) {
    if (channelID == 0) return -1;
    const TS3Functions* fns = PluginContext::instance().ts3Functions();
    if (!fns) return -1;
    uint64_t schid = currentServerHandlerID();
    if (schid == 0) return -1;
    anyID me = 0;
    if (fns->getClientID(schid, &me) != ERROR_ok) return -1;
    // No-op short-circuit: requesting a move into the current channel can
    // return ERROR_channel_already_in on some servers; treat as success so
    // callers don't need to special-case it.
    uint64 cur = 0;
    if (fns->getChannelOfClient(schid, me, &cur) == ERROR_ok && cur == channelID) {
        return 1;
    }
    if (fns->requestClientMove(schid, me, channelID, "", nullptr) != ERROR_ok) {
        return -1;
    }
    return 1;
}

int TS3Bridge::currentChannelID(uint64_t* outID, QString* outName) const {
    const TS3Functions* fns = PluginContext::instance().ts3Functions();
    if (!fns) return -1;
    uint64_t schid = currentServerHandlerID();
    if (schid == 0) {
        if (outID)   *outID = 0;
        if (outName) *outName = QString();
        return 0;
    }
    anyID me = 0;
    if (fns->getClientID(schid, &me) != ERROR_ok) return -1;
    uint64 cid = 0;
    if (fns->getChannelOfClient(schid, me, &cid) != ERROR_ok) return -1;
    if (outID) *outID = cid;
    if (outName) {
        char* name = nullptr;
        if (fns->getChannelVariableAsString(schid, cid, CHANNEL_NAME, &name) == ERROR_ok && name) {
            *outName = QString::fromUtf8(name);
            fns->freeMemory(name);
        } else {
            *outName = QString();
        }
    }
    return 1;
}

int TS3Bridge::toggleSelfAwayWithAfkPath(const QString& afkPath) {
    int next = toggleSelfAway();
    if (next < 0) return next;
    if (afkPath.isEmpty()) return next;

    if (next == 1) {
        // Going AWAY: remember where we are, then hop. If the path doesn't
        // resolve we keep the away state and skip the move silently — the
        // mute/away toggle is the primary contract, the channel hop is the
        // bonus.
        uint64_t cur = 0;
        if (currentChannelID(&cur, nullptr) == 1) {
            m_savedChannelID = cur;
        }
        uint64_t target = resolveChannelPath(afkPath);
        if (target != 0 && target != m_savedChannelID) {
            moveSelfToChannelID(target);
        }
    } else {
        // Coming back: restore previous channel if we still have one.
        uint64_t back = m_savedChannelID;
        m_savedChannelID = 0;
        if (back != 0) moveSelfToChannelID(back);
    }
    return next;
}

int TS3Bridge::toggleSelfSilentModeWithAfkPath(const QString& afkPath) {
    int next = toggleSelfSilentMode();
    if (next < 0) return next;
    if (afkPath.isEmpty()) return next;

    if (next == 1) {
        uint64_t cur = 0;
        if (currentChannelID(&cur, nullptr) == 1) {
            m_savedChannelID = cur;
        }
        uint64_t target = resolveChannelPath(afkPath);
        if (target != 0 && target != m_savedChannelID) {
            moveSelfToChannelID(target);
        }
    } else {
        uint64_t back = m_savedChannelID;
        m_savedChannelID = 0;
        if (back != 0) moveSelfToChannelID(back);
    }
    return next;
}
