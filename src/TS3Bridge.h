#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <cstdint>

class TS3Bridge : public QObject {
    Q_OBJECT
public:
    explicit TS3Bridge(QObject* parent = nullptr);

    // All methods read immutable snapshots from TS3; safe from any thread.
    // Return an empty QString / 0 on failure. freeMemory is handled
    // internally.
    QString nicknameForClient(uint16_t clientID, uint64_t schid) const;
    uint64_t channelForClient(uint16_t clientID, uint64_t schid) const;
    QString channelNameForChannel(uint64_t channelID, uint64_t schid) const;
    uint16_t localClientIDOnServer(uint64_t schid) const;

    // Active server connection handler. Returns 0 when no server is
    // connected. Used by HTTP control endpoints to fail-fast.
    uint64_t currentServerHandlerID() const;

    // Self-state controls — drive the HTTP / tray / hotkey paths. Each
    // toggle reads the current value, flips it, writes back and flushes;
    // returns the new state (0/1) or -1 if no server is connected.
    int toggleSelfMicMuted();
    int toggleSelfSpeakerMuted();
    int toggleSelfAway();

    // Combined toggle: mic + speaker mute + away in lockstep, derived from
    // the current mic-mute flag. New state is mirrored to all three.
    int toggleSelfSilentMode();

    // Read-only snapshots. -1 on failure / no server.
    int selfMicMuted() const;
    int selfSpeakerMuted() const;
    int selfAway() const;
    int selfSilentMode() const;

    // Channel-switch primitives. resolveChannelPath accepts a slash-separated
    // path like "Lobby/AFK" and returns the matching channelID, or 0 if not
    // resolvable / no server. moveSelfToChannelID requests the local client
    // be moved into channelID; returns 1 on success, -1 on failure.
    uint64_t resolveChannelPath(const QString& path) const;
    int moveSelfToChannelID(uint64_t channelID);

    // Self-channel info. Fills outID with the channel the local client is in
    // and outName with its display name. Returns 1 on success, 0 when not
    // connected, -1 on internal failure. Either out param may be nullptr.
    int currentChannelID(uint64_t* outID, QString* outName) const;

    // AFK-aware variants. When afkPath is non-empty:
    //   ON-transition  → remember current channel, move to resolved AFK channel.
    //   OFF-transition → move back to remembered channel (if it still resolves).
    // When afkPath is empty these behave identically to toggleSelfAway /
    // toggleSelfSilentMode. Returns the new state (0/1) or -1 on failure.
    int toggleSelfAwayWithAfkPath(const QString& afkPath);
    int toggleSelfSilentModeWithAfkPath(const QString& afkPath);

private:
    // Helpers for the int-flag dance shared by mic / speaker.
    int toggleSelfFlag(int flag);
    int readSelfFlag(int flag) const;

    // Channel the local client was in *before* the AFK-aware toggle moved it.
    // Set on the ON-transition of toggleSelf{Away,SilentMode}WithAfkPath,
    // consumed and cleared on the OFF transition.
    uint64_t m_savedChannelID = 0;
};
