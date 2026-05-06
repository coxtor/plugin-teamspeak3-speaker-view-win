#pragma once

#include <QtCore/QObject>

class QSystemTrayIcon;
class QMenu;

// Windows notification-area icon — Qt equivalent to the Mac
// MenuBarController's NSStatusItem. Provides one-click access to the same
// three actions (Toggle overlay, Settings, Quit TeamSpeak) without going
// through the TS3 Tools menu.
class TrayController : public QObject {
    Q_OBJECT
public:
    explicit TrayController(QObject* parent = nullptr);
    ~TrayController() override;

    // Show / hide the tray icon. Called from PluginContext when the user
    // flips the corresponding checkbox in the settings dialog.
    void setVisible(bool visible);
    bool isVisible() const;

private slots:
    void onToggleOverlay();
    void onOpenSettings();
    void onQuitTeamSpeak();

private:
    void buildTray();

    QSystemTrayIcon* m_tray = nullptr;
    QMenu* m_menu = nullptr;
};
