#include "PluginContext.h"

#include "ConfigModel.h"
#include "Log.h"
#include "OverlayController.h"
#include "SpeakerTracker.h"
#include "TS3Bridge.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QThread>
#include <QtWidgets/QApplication>

PluginContext& PluginContext::instance() {
    static PluginContext ctx;
    return ctx;
}

void PluginContext::setTS3Functions(const TS3Functions* fns) {
    m_fns = fns;
}

void PluginContext::setupOnInit() {
    sv_log(QStringLiteral("setupOnInit: enter. qApp=%1 thread=%2 mainThread=%3")
           .arg(qApp ? QStringLiteral("present") : QStringLiteral("NULL"))
           .arg(reinterpret_cast<quintptr>(QThread::currentThread()), 0, 16)
           .arg(qApp ? reinterpret_cast<quintptr>(qApp->thread()) : 0, 0, 16));

    m_config = new ConfigModel(this);
    m_config->load();
    sv_log("setupOnInit: config loaded");

    m_bridge = new TS3Bridge(this);
    m_tracker = new SpeakerTracker(this);
    sv_log("setupOnInit: bridge + tracker created");

    m_overlay = new OverlayController(this);
    sv_log("setupOnInit: OverlayController constructed");
    m_overlay->applyConfig(*m_config);
    sv_log("setupOnInit: overlay applyConfig done");

    QObject::connect(m_tracker, &SpeakerTracker::snapshotReady,
                     m_overlay, &OverlayController::applySnapshot,
                     Qt::QueuedConnection);
    QObject::connect(m_config, &ConfigModel::changed,
                     m_overlay, [this]() { m_overlay->applyConfig(*m_config); });
    QObject::connect(m_config, &ConfigModel::changed,
                     m_tracker, &SpeakerTracker::configChanged);

    sv_log("setupOnInit: exit. Plugin initialised");
}

void PluginContext::teardownOnShutdown() {
    // Synchronous delete only. deleteLater() crashes TS3 on "Reload All":
    // TS3 unloads our DLL right after shutdown(), then Qt's event loop
    // later tries to dispatch the queued delete-events into freed code.
    if (m_overlay) {
        m_overlay->teardown();
        delete m_overlay;
        m_overlay = nullptr;
    }
    if (m_tracker) {
        m_tracker->teardown();
        delete m_tracker;
        m_tracker = nullptr;
    }
    if (m_config) {
        delete m_config;
        m_config = nullptr;
    }
    if (m_bridge) {
        delete m_bridge;
        m_bridge = nullptr;
    }
    m_configDialog = nullptr;
    m_pluginID.clear();
    sv_log("Plugin shut down");
}
