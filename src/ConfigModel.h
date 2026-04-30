#pragma once

#include <QtCore/QObject>
#include <QtCore/QRect>
#include <QtCore/QString>

class QSettings;

enum class DisplayMode {
    AllInChannel = 0,
    LatestOnly = 1,
};

class ConfigModel : public QObject {
    Q_OBJECT
public:
    explicit ConfigModel(QObject* parent = nullptr);
    ~ConfigModel() override;

    void load();
    void save();

    // Getters
    double fadeOutSeconds() const { return m_fadeOutSeconds; }
    DisplayMode displayMode() const { return m_displayMode; }
    bool alwaysOnTop() const { return m_alwaysOnTop; }
    bool rememberFrame() const { return m_rememberFrame; }
    bool clickThrough() const { return m_clickThrough; }
    bool borderless() const { return m_borderless; }
    bool showAvatar() const { return m_showAvatar; }
    bool showChannel() const { return m_showChannel; }
    bool showSelf() const { return m_showSelf; }
    QRect windowFrame() const { return m_windowFrame; }

    // Setters: persist immediately and emit changed() (except windowFrame,
    // which is silent to avoid rebuild loops when geometry callbacks fire).
    void setFadeOutSeconds(double v);
    void setDisplayMode(DisplayMode v);
    void setAlwaysOnTop(bool v);
    void setRememberFrame(bool v);
    void setClickThrough(bool v);
    void setBorderless(bool v);
    void setShowAvatar(bool v);
    void setShowChannel(bool v);
    void setShowSelf(bool v);
    void setWindowFrame(QRect r);  // silent

signals:
    void changed();

private:
    QString settingsFilePath() const;
    void notifyChange();

    double m_fadeOutSeconds = 2.5;
    DisplayMode m_displayMode = DisplayMode::AllInChannel;
    bool m_alwaysOnTop = true;
    bool m_rememberFrame = true;
    bool m_clickThrough = false;
    bool m_borderless = false;
    bool m_showAvatar = false;
    bool m_showChannel = false;
    bool m_showSelf = false;
    QRect m_windowFrame;

    bool m_suppressNotifications = false;
};
