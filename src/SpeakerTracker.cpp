#include "SpeakerTracker.h"

#include "ConfigModel.h"
#include "PluginContext.h"
#include "TS3Bridge.h"

#include <QtCore/QDateTime>
#include <QtCore/QMutexLocker>
#include <QtCore/QTimer>
#include <algorithm>

extern "C" {
#include "plugin_definitions.h"
#include "teamspeak/public_definitions.h"
}

namespace {
constexpr int kMaxRows = 10;

// TS3 emits NOT_TALKING during voice-activity-detection gaps between words,
// sentences, and breaths. 2 s covers typical breath pauses and mid-sentence
// hesitation without making a truly finished turn feel sluggish. Must match
// the macOS reference implementation.
constexpr double kGracePeriodSeconds = 2.0;

double nowSeconds() {
    return QDateTime::currentMSecsSinceEpoch() / 1000.0;
}
}

SpeakerTracker::SpeakerTracker(QObject* parent) : QObject(parent) {
    qRegisterMetaType<QList<SpeakerSnapshot>>("QList<SpeakerSnapshot>");

    m_timer = new QTimer(this);
    m_timer->setInterval(100);  // 100 ms tick
    connect(m_timer, &QTimer::timeout, this, &SpeakerTracker::onFadeTick);
}

SpeakerTracker::~SpeakerTracker() = default;

void SpeakerTracker::teardown() {
    QMutexLocker lock(&m_mutex);
    if (m_timer) m_timer->stop();
    m_entries.clear();
}

void SpeakerTracker::resumeTimerIfNeeded() {
    if (m_timer && !m_timer->isActive()) {
        QMetaObject::invokeMethod(m_timer, QOverload<>::of(&QTimer::start),
                                  Qt::QueuedConnection);
    }
}

void SpeakerTracker::suspendTimerIfIdle() {
    if (m_timer && m_timer->isActive() && m_entries.empty()) {
        QMetaObject::invokeMethod(m_timer, &QTimer::stop, Qt::QueuedConnection);
    }
}

void SpeakerTracker::handleTalkStatus(int status, uint16_t clientID,
                                      uint64_t schid, bool /*isWhisper*/) {
    QMutexLocker lock(&m_mutex);

    ConfigModel* cfg = PluginContext::instance().config();
    TS3Bridge* bridge = PluginContext::instance().bridge();
    if (!cfg || !bridge) return;

    uint16_t localID = bridge->localClientIDOnServer(schid);
    const bool isSelf = (clientID == localID);
    if (isSelf && !cfg->showSelf()) return;

    const uint64_t myChannel = localID ? bridge->channelForClient(localID, schid) : 0;
    const uint64_t theirChannel = bridge->channelForClient(clientID, schid);
    if (myChannel == 0 || theirChannel == 0 || myChannel != theirChannel) return;

    m_lastKnownLocalChannel = myChannel;

    // Phantom-entry guard: if NOT_TALKING arrives for a client we no longer
    // track, ignore it — otherwise we'd resurrect the row briefly.
    if (status != STATUS_TALKING && m_entries.find(clientID) == m_entries.end()) {
        return;
    }

    Entry& entry = m_entries[clientID];
    entry.clientID = clientID;
    entry.channelID = theirChannel;
    if (entry.nickname.isEmpty()) {
        entry.nickname = bridge->nicknameForClient(clientID, schid);
    }
    if (cfg->showChannel() && entry.channelName.isEmpty()) {
        entry.channelName = bridge->channelNameForChannel(theirChannel, schid);
    }

    if (status == STATUS_TALKING) {
        entry.talking = true;
        entry.stoppedAt = 0;
        m_lastTalker = clientID;
    } else {
        // Fade-clock integrity: only stamp on the first talking->not-talking
        // transition; subsequent NOT_TALKINGs during fade are no-ops.
        if (entry.talking) {
            entry.talking = false;
            entry.stoppedAt = nowSeconds();
        }
        resumeTimerIfNeeded();
    }

    emitSnapshotLocked();
}

void SpeakerTracker::handleClientMoved(uint64_t fromChannel, uint64_t toChannel,
                                       uint16_t clientID, uint64_t schid) {
    QMutexLocker lock(&m_mutex);
    TS3Bridge* bridge = PluginContext::instance().bridge();
    if (!bridge) return;
    uint16_t localID = bridge->localClientIDOnServer(schid);

    if (clientID == localID) {
        m_entries.clear();
        m_lastKnownLocalChannel = toChannel;
        m_lastTalker = 0;
        emitSnapshotLocked();
        suspendTimerIfIdle();
        return;
    }

    const uint64_t myChannel = localID ? bridge->channelForClient(localID, schid) : 0;
    if (fromChannel == myChannel && toChannel != myChannel) {
        m_entries.erase(clientID);
        if (m_lastTalker == clientID) m_lastTalker = 0;
        emitSnapshotLocked();
        suspendTimerIfIdle();
    }
}

void SpeakerTracker::handleConnectStatus(int status, uint64_t /*schid*/) {
    if (status != STATUS_DISCONNECTED) return;
    QMutexLocker lock(&m_mutex);
    m_entries.clear();
    m_lastKnownLocalChannel = 0;
    m_lastTalker = 0;
    emitSnapshotLocked();
    suspendTimerIfIdle();
}

void SpeakerTracker::handleClientUpdate(uint16_t clientID, uint64_t schid) {
    QMutexLocker lock(&m_mutex);
    auto it = m_entries.find(clientID);
    if (it == m_entries.end()) return;
    TS3Bridge* bridge = PluginContext::instance().bridge();
    if (!bridge) return;
    QString newNick = bridge->nicknameForClient(clientID, schid);
    if (!newNick.isEmpty() && newNick != it->second.nickname) {
        it->second.nickname = newNick;
        emitSnapshotLocked();
    }
}

void SpeakerTracker::configChanged() {
    QMutexLocker lock(&m_mutex);
    emitSnapshotLocked();
}

void SpeakerTracker::onFadeTick() {
    QMutexLocker lock(&m_mutex);
    ConfigModel* cfg = PluginContext::instance().config();
    if (!cfg) return;
    const double now = nowSeconds();
    const double totalHold = kGracePeriodSeconds + cfg->fadeOutSeconds();

    for (auto it = m_entries.begin(); it != m_entries.end(); ) {
        if (!it->second.talking && (now - it->second.stoppedAt) >= totalHold) {
            if (m_lastTalker == it->first) m_lastTalker = 0;
            it = m_entries.erase(it);
        } else {
            ++it;
        }
    }
    // Always re-emit while anything is in grace/fade so the UI can animate
    // opacity continuously and flip its dot colour at the grace boundary.
    emitSnapshotLocked();
    suspendTimerIfIdle();
}

void SpeakerTracker::emitSnapshotLocked() {
    ConfigModel* cfg = PluginContext::instance().config();
    if (!cfg) return;
    const double now = nowSeconds();

    QList<SpeakerSnapshot> all;
    all.reserve(static_cast<int>(m_entries.size()));
    for (auto& kv : m_entries) {
        SpeakerSnapshot s;
        s.clientID = kv.second.clientID;
        s.nickname = kv.second.nickname;
        s.channelName = cfg->showChannel() ? kv.second.channelName : QString();

        const bool inGrace = !kv.second.talking
                             && (now - kv.second.stoppedAt) < kGracePeriodSeconds;
        s.talking = kv.second.talking || inGrace;

        if (s.talking) {
            s.remainingFadeSeconds = 0.0;
        } else {
            const double elapsedSinceFadeStart =
                (now - kv.second.stoppedAt) - kGracePeriodSeconds;
            s.remainingFadeSeconds =
                std::max(0.0, cfg->fadeOutSeconds() - elapsedSinceFadeStart);
        }
        all.append(s);
    }

    std::sort(all.begin(), all.end(),
              [](const SpeakerSnapshot& a, const SpeakerSnapshot& b) {
        if (a.talking != b.talking) return a.talking;
        if (a.remainingFadeSeconds != b.remainingFadeSeconds)
            return a.remainingFadeSeconds > b.remainingFadeSeconds;
        return a.nickname.compare(b.nickname, Qt::CaseInsensitive) < 0;
    });

    QList<SpeakerSnapshot> finalList;
    if (cfg->displayMode() == DisplayMode::LatestOnly) {
        SpeakerSnapshot pick;
        bool found = false;
        for (const auto& s : all) {
            if (s.clientID == m_lastTalker) { pick = s; found = true; break; }
        }
        if (!found && !all.isEmpty()) { pick = all.first(); found = true; }
        if (found) finalList.append(pick);
    } else if (all.size() > kMaxRows) {
        finalList = all.mid(0, kMaxRows);
    } else {
        finalList = all;
    }

    emit snapshotReady(finalList);
}
