#include "ConfigModel.h"

#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtCore/QStandardPaths>

namespace {
constexpr const char* kKeyFadeOut      = "fadeOutSeconds";
constexpr const char* kKeyDisplayMode  = "displayMode";
constexpr const char* kKeyAlwaysOnTop  = "alwaysOnTop";
constexpr const char* kKeyRememberFrame= "rememberFrame";
constexpr const char* kKeyClickThrough = "clickThrough";
constexpr const char* kKeyShowChannel  = "showChannel";
constexpr const char* kKeyShowSelf     = "showSelf";
constexpr const char* kKeyShowTrayIcon = "showTrayIcon";
constexpr const char* kKeyHttpEnabled  = "httpControlEnabled";
constexpr const char* kKeyHttpPort     = "httpControlPort";
constexpr const char* kKeyFrameX       = "windowFrameX";
constexpr const char* kKeyFrameY       = "windowFrameY";
constexpr const char* kKeyFrameW       = "windowFrameW";
constexpr const char* kKeyFrameH       = "windowFrameH";
}

ConfigModel::ConfigModel(QObject* parent) : QObject(parent) {}
ConfigModel::~ConfigModel() = default;

QString ConfigModel::settingsFilePath() const {
    // %APPDATA%\TS3Client\plugins\speaker-view\settings.ini
    QString roaming = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    // AppDataLocation on Windows resolves under the calling app's subfolder
    // (e.g. TS3Client). Strip the "<AppName>" tail and rebuild under TS3Client.
    QDir appData(roaming);
    appData.cdUp();
    QString ts3Plugins = appData.filePath("TS3Client/plugins/speaker-view");
    QDir().mkpath(ts3Plugins);
    return ts3Plugins + "/settings.ini";
}

void ConfigModel::load() {
    QSettings s(settingsFilePath(), QSettings::IniFormat);
    m_suppressNotifications = true;

    m_fadeOutSeconds = s.value(kKeyFadeOut,      m_fadeOutSeconds).toDouble();
    m_displayMode    = static_cast<DisplayMode>(
                         s.value(kKeyDisplayMode, static_cast<int>(m_displayMode)).toInt());
    m_alwaysOnTop    = s.value(kKeyAlwaysOnTop,  m_alwaysOnTop).toBool();
    m_rememberFrame  = s.value(kKeyRememberFrame,m_rememberFrame).toBool();
    m_clickThrough   = s.value(kKeyClickThrough, m_clickThrough).toBool();
    m_showChannel        = s.value(kKeyShowChannel,  m_showChannel).toBool();
    m_showSelf           = s.value(kKeyShowSelf,     m_showSelf).toBool();
    m_showTrayIcon       = s.value(kKeyShowTrayIcon, m_showTrayIcon).toBool();
    m_httpControlEnabled = s.value(kKeyHttpEnabled,  m_httpControlEnabled).toBool();
    m_httpControlPort    = s.value(kKeyHttpPort,     m_httpControlPort).toInt();

    if (s.contains(kKeyFrameX) && s.contains(kKeyFrameY)
        && s.contains(kKeyFrameW) && s.contains(kKeyFrameH)) {
        m_windowFrame = QRect(s.value(kKeyFrameX).toInt(),
                              s.value(kKeyFrameY).toInt(),
                              s.value(kKeyFrameW).toInt(),
                              s.value(kKeyFrameH).toInt());
    }

    m_suppressNotifications = false;
}

void ConfigModel::save() {
    QSettings s(settingsFilePath(), QSettings::IniFormat);
    s.setValue(kKeyFadeOut,       m_fadeOutSeconds);
    s.setValue(kKeyDisplayMode,   static_cast<int>(m_displayMode));
    s.setValue(kKeyAlwaysOnTop,   m_alwaysOnTop);
    s.setValue(kKeyRememberFrame, m_rememberFrame);
    s.setValue(kKeyClickThrough,  m_clickThrough);
    s.setValue(kKeyShowChannel,   m_showChannel);
    s.setValue(kKeyShowSelf,      m_showSelf);
    s.setValue(kKeyShowTrayIcon,  m_showTrayIcon);
    s.setValue(kKeyHttpEnabled,   m_httpControlEnabled);
    s.setValue(kKeyHttpPort,      m_httpControlPort);
    if (m_windowFrame.isValid()) {
        s.setValue(kKeyFrameX, m_windowFrame.x());
        s.setValue(kKeyFrameY, m_windowFrame.y());
        s.setValue(kKeyFrameW, m_windowFrame.width());
        s.setValue(kKeyFrameH, m_windowFrame.height());
    } else {
        // Actively drop stale frame keys so a later load() doesn't pick
        // up a previously-saved off-screen geometry.
        s.remove(kKeyFrameX);
        s.remove(kKeyFrameY);
        s.remove(kKeyFrameW);
        s.remove(kKeyFrameH);
    }
    s.sync();
}

void ConfigModel::notifyChange() {
    if (m_suppressNotifications) return;
    emit changed();
}

#define SV_SET(Name, Field, Type)                        \
    void ConfigModel::set##Name(Type v) {                \
        if (Field == v) return;                          \
        Field = v;                                       \
        save();                                          \
        notifyChange();                                  \
    }

SV_SET(FadeOutSeconds, m_fadeOutSeconds, double)
SV_SET(DisplayMode,    m_displayMode,    DisplayMode)
SV_SET(AlwaysOnTop,    m_alwaysOnTop,    bool)
SV_SET(RememberFrame,  m_rememberFrame,  bool)
SV_SET(ClickThrough,   m_clickThrough,   bool)
SV_SET(ShowChannel,        m_showChannel,        bool)
SV_SET(ShowSelf,           m_showSelf,           bool)
SV_SET(ShowTrayIcon,       m_showTrayIcon,       bool)
SV_SET(HttpControlEnabled, m_httpControlEnabled, bool)
SV_SET(HttpControlPort,    m_httpControlPort,    int)

// windowFrame is silent — the UI writes it from its own geometry
// notifications and must not get a config-change callback in return.
void ConfigModel::setWindowFrame(QRect r) {
    if (m_windowFrame == r) return;
    m_windowFrame = r;
    save();
}
