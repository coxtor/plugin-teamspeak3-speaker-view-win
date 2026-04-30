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
};
