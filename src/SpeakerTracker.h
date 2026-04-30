#pragma once

#include <QtCore/QList>
#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <cstdint>
#include <unordered_map>

class QTimer;

struct SpeakerSnapshot {
    uint16_t clientID = 0;
    QString nickname;
    QString channelName;
    bool talking = false;
    double remainingFadeSeconds = 0.0;
};

Q_DECLARE_METATYPE(QList<SpeakerSnapshot>)

class SpeakerTracker : public QObject {
    Q_OBJECT
public:
    explicit SpeakerTracker(QObject* parent = nullptr);
    ~SpeakerTracker() override;

    // Callable from any thread. Internally serialised by a mutex.
    void handleTalkStatus(int status, uint16_t clientID, uint64_t schid, bool isWhisper);
    void handleClientMoved(uint64_t fromChannel, uint64_t toChannel,
                           uint16_t clientID, uint64_t schid);
    void handleConnectStatus(int status, uint64_t schid);
    void handleClientUpdate(uint16_t clientID, uint64_t schid);

    void teardown();

public slots:
    void configChanged();

signals:
    void snapshotReady(QList<SpeakerSnapshot> snapshot);

private slots:
    void onFadeTick();

private:
    struct Entry {
        uint16_t clientID = 0;
        QString nickname;
        QString channelName;
        uint64_t channelID = 0;
        bool talking = false;
        double stoppedAt = 0.0;  // seconds since epoch; 0 while talking
    };

    void emitSnapshotLocked();  // must be called while m_mutex is held
    void resumeTimerIfNeeded();
    void suspendTimerIfIdle();

    QMutex m_mutex;
    QTimer* m_timer = nullptr;
    std::unordered_map<uint16_t, Entry> m_entries;
    uint64_t m_lastKnownLocalChannel = 0;
    uint16_t m_lastTalker = 0;
};
