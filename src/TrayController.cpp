#include "TrayController.h"

#include "ConfigDialog.h"
#include "Log.h"
#include "OverlayController.h"
#include "PluginContext.h"

#include <QtCore/QCoreApplication>
#include <QtGui/QCursor>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMenu>
#include <QtWidgets/QSystemTrayIcon>

#include <cstdlib>

TrayController::TrayController(QObject* parent) : QObject(parent) {
    buildTray();
}

TrayController::~TrayController() {
    if (m_tray) {
        m_tray->hide();
    }
}

void TrayController::buildTray() {
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        sv_log("System tray not available on this system; skipping tray icon.");
        return;
    }

    m_menu = new QMenu();
    m_menu->addAction(QStringLiteral("Toggle Speaker View"),
                      this, &TrayController::onToggleOverlay);
    m_menu->addAction(QStringLiteral("Settings\xE2\x80\xA6"),
                      this, &TrayController::onOpenSettings);
    m_menu->addSeparator();
    m_menu->addAction(QStringLiteral("Quit TeamSpeak"),
                      this, &TrayController::onQuitTeamSpeak);

    m_tray = new QSystemTrayIcon(this);
    m_tray->setContextMenu(m_menu);
    m_tray->setToolTip(QStringLiteral("Speaker View"));

    // Use the host application's window icon — that's TS3's own icon when
    // we run inside the TeamSpeak process. If TS3 hasn't set one (rare),
    // fall back to a generic information icon so the tray entry is at
    // least visible.
    QIcon icon = QApplication::windowIcon();
    if (icon.isNull()) {
        icon = QIcon(QStringLiteral(":/qt-project.org/styles/commonstyle/images/standardbutton-apply-32.png"));
    }
    m_tray->setIcon(icon);

    // Left-click opens the menu too — Windows users expect that even
    // though the right-click context menu is the canonical entrypoint.
    connect(m_tray, &QSystemTrayIcon::activated,
            this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger && m_menu) {
            m_menu->popup(QCursor::pos());
        }
    });

    sv_log("Tray icon installed");
}

void TrayController::setVisible(bool visible) {
    if (!m_tray) {
        if (visible) buildTray();
        if (!m_tray) return;
    }
    if (visible) {
        m_tray->show();
    } else {
        m_tray->hide();
    }
}

bool TrayController::isVisible() const {
    return m_tray && m_tray->isVisible();
}

void TrayController::onToggleOverlay() {
    if (auto* o = PluginContext::instance().overlay()) {
        o->toggleVisible();
    }
}

void TrayController::onOpenSettings() {
    ConfigDialog::presentModal(nullptr);
}

void TrayController::onQuitTeamSpeak() {
    sv_log("Quit TeamSpeak requested via tray icon");
    std::exit(0);
}
