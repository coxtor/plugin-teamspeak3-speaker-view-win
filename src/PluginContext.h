#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <cstdint>

struct TS3Functions;
class TS3Bridge;
class SpeakerTracker;
class OverlayController;
class ConfigModel;
class ConfigDialog;

class PluginContext : public QObject {
    Q_OBJECT
public:
    static PluginContext& instance();

    void setTS3Functions(const TS3Functions* fns);
    const TS3Functions* ts3Functions() const { return m_fns; }

    void setPluginID(QString id) { m_pluginID = std::move(id); }
    const QString& pluginID() const { return m_pluginID; }

    uint64_t activeServerHandlerID() const { return m_activeServerHandlerID; }
    void setActiveServerHandlerID(uint64_t id) { m_activeServerHandlerID = id; }

    TS3Bridge* bridge() const { return m_bridge; }
    SpeakerTracker* tracker() const { return m_tracker; }
    OverlayController* overlay() const { return m_overlay; }
    ConfigModel* config() const { return m_config; }

    void setConfigDialog(ConfigDialog* d) { m_configDialog = d; }
    ConfigDialog* configDialog() const { return m_configDialog; }

    void setupOnInit();
    void teardownOnShutdown();

private:
    PluginContext() = default;
    PluginContext(const PluginContext&) = delete;
    PluginContext& operator=(const PluginContext&) = delete;

    const TS3Functions* m_fns = nullptr;
    QString m_pluginID;
    uint64_t m_activeServerHandlerID = 0;

    TS3Bridge* m_bridge = nullptr;
    SpeakerTracker* m_tracker = nullptr;
    OverlayController* m_overlay = nullptr;
    ConfigModel* m_config = nullptr;
    ConfigDialog* m_configDialog = nullptr;
};
