#include "PluginContext.h"

#include "ConfigModel.h"
#include "OverlayController.h"
#include "SpeakerTracker.h"
#include "TS3Bridge.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QTimer>

extern "C" {
#include "plugin_definitions.h"
#include "teamspeak/public_definitions.h"
#include "ts3_functions.h"
}

static void sv_log(const char* msg);

PluginContext& PluginContext::instance() {
    static PluginContext ctx;
    return ctx;
}

void PluginContext::setTS3Functions(const TS3Functions* fns) {
    m_fns = fns;
}

void PluginContext::setupOnInit() {
    m_config = new ConfigModel(this);
    m_config->load();

    m_bridge = new TS3Bridge(this);
    m_tracker = new SpeakerTracker(this);

    // UI must live on the main (Qt GUI) thread. TS3 runs our code on the
    // main thread for init, but we still defer overlay construction to the
    // event loop so the host's QApplication is fully wired up.
    QTimer::singleShot(0, this, [this]() {
        m_overlay = new OverlayController(this);
        m_overlay->applyConfig(*m_config);

        QObject::connect(m_tracker, &SpeakerTracker::snapshotReady,
                         m_overlay, &OverlayController::applySnapshot,
                         Qt::QueuedConnection);
        QObject::connect(m_config, &ConfigModel::changed,
                         m_overlay, [this]() { m_overlay->applyConfig(*m_config); });
        QObject::connect(m_config, &ConfigModel::changed,
                         m_tracker, &SpeakerTracker::configChanged);
    });

    sv_log("Plugin initialised");
}

void PluginContext::teardownOnShutdown() {
    if (m_overlay) {
        m_overlay->teardown();
        m_overlay->deleteLater();
        m_overlay = nullptr;
    }
    if (m_tracker) {
        m_tracker->teardown();
        m_tracker->deleteLater();
        m_tracker = nullptr;
    }
    if (m_config) {
        m_config->deleteLater();
        m_config = nullptr;
    }
    if (m_bridge) {
        m_bridge->deleteLater();
        m_bridge = nullptr;
    }
    m_configDialog = nullptr;
    m_pluginID.clear();
    sv_log("Plugin shut down");
}

static void sv_log(const char* msg) {
    const TS3Functions* fns = PluginContext::instance().ts3Functions();
    if (fns && fns->logMessage) {
        fns->logMessage(msg, LogLevel_INFO, "speakerview", 0);
    }
}
