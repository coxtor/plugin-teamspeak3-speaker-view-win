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

private:
    // Helpers for the int-flag dance shared by mic / speaker.
    int toggleSelfFlag(int flag);
    int readSelfFlag(int flag) const;
};
