#include "OverlayController.h"

#include "ConfigModel.h"
#include "Log.h"
#include "OverlayWidget.h"
#include "PluginContext.h"
#include "SpeakerRowWidget.h"

#include <QtCore/QSet>
#include <QtCore/QTimer>
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>
#include <QtWidgets/QVBoxLayout>

namespace {
// A saved frame is usable only if it intersects at least one currently
// attached screen by a meaningful amount. Without this check, a frame
// that was saved on a second monitor (e.g. one that Parallels exposes
// and later hides) parks the overlay off-screen on every subsequent
// launch — the window is technically visible to Qt but nothing of it
// lands on any physical display.
bool frameIsOnScreen(const QRect& r) {
    if (!r.isValid()) return false;
    constexpr int kMinVisible = 80;  // at least an 80x40 corner must land somewhere
    for (QScreen* s : QGuiApplication::screens()) {
        const QRect inter = s->availableGeometry().intersected(r);
        if (inter.width() >= kMinVisible && inter.height() >= 40) return true;
    }
    return false;
}
}

OverlayController::OverlayController(QObject* parent) : QObject(parent) {
    sv_log("OverlayController: constructing OverlayWidget");
    m_window = new OverlayWidget();
    sv_log(QStringLiteral("OverlayController: OverlayWidget constructed, flags=0x%1 attrs: visible=%2 size=%3x%4")
           .arg(static_cast<quint32>(m_window->windowFlags()), 0, 16)
           .arg(m_window->isVisible())
           .arg(m_window->width()).arg(m_window->height()));

    // Rows go into the window's content layout (below its custom title bar).
    m_layout = m_window->contentLayout();

    if (auto* cfg = PluginContext::instance().config()) {
        QRect saved = cfg->windowFrame();
        const bool onScreen = frameIsOnScreen(saved);
        if (cfg->rememberFrame() && saved.isValid() && onScreen) {
            sv_log(QStringLiteral("OverlayController: restoring saved frame %1,%2 %3x%4")
                   .arg(saved.x()).arg(saved.y())
                   .arg(saved.width()).arg(saved.height()));
            m_window->setGeometry(saved);
        } else {
            if (saved.isValid() && !onScreen) {
                sv_log(QStringLiteral("OverlayController: saved frame %1,%2 %3x%4 is off-screen; "
                                      "using default position")
                       .arg(saved.x()).arg(saved.y())
                       .arg(saved.width()).arg(saved.height()));
                // Forget the stale off-screen frame so next save writes a sane one.
                cfg->setWindowFrame(QRect());
            } else {
                sv_log("OverlayController: no saved frame; moving to 120,120");
            }
            m_window->move(120, 120);
        }
    } else {
        sv_log("OverlayController: WARNING no config available at construction");
    }

    sv_log("OverlayController: calling m_window->show()");
    m_window->show();
    sv_log(QStringLiteral("OverlayController: after show(): visible=%1 geom=%2,%3 %4x%5 winId=0x%6")
           .arg(m_window->isVisible())
           .arg(m_window->x()).arg(m_window->y())
           .arg(m_window->width()).arg(m_window->height())
           .arg(static_cast<quintptr>(m_window->winId()), 0, 16));

    connect(m_window, &OverlayWidget::frameChanged,
            this, &OverlayController::onWindowFrameChanged);

    QTimer::singleShot(0, this, [this]() {
        m_frameArmed = true;
        if (m_window) {
            sv_log(QStringLiteral("[+0ms] visible=%1 geom=%2,%3 %4x%5")
                   .arg(m_window->isVisible())
                   .arg(m_window->x()).arg(m_window->y())
                   .arg(m_window->width()).arg(m_window->height()));
        }
    });
    for (int delay : {100, 500, 2000}) {
        QTimer::singleShot(delay, this, [this, delay]() {
            if (!m_window) return;
            sv_log(QStringLiteral("[+%1ms] visible=%2 minimized=%3 geom=%4,%5 %6x%7")
                   .arg(delay)
                   .arg(m_window->isVisible())
                   .arg(m_window->isMinimized())
                   .arg(m_window->x()).arg(m_window->y())
                   .arg(m_window->width()).arg(m_window->height()));
        });
    }
}

OverlayController::~OverlayController() {
    teardown();
}

void OverlayController::teardown() {
    if (m_window) {
        onWindowFrameChanged();  // flush any last move/resize
        m_window->hide();
        // Synchronous delete: deleteLater() would be dispatched after TS3
        // unloads our DLL on "Reload All", leading to a crash in the event
        // loop. Rows are children of m_window and go with it.
        delete m_window;
        m_window = nullptr;
    }
    m_rowsByClient.clear();
}

void OverlayController::applyConfig(const ConfigModel& cfg) {
    m_showChannel = cfg.showChannel();
    m_fadeDuration = cfg.fadeOutSeconds();
    m_rememberFrame = cfg.rememberFrame();

    if (!m_window) return;
    m_window->setAlwaysOnTop(cfg.alwaysOnTop());
    m_window->setClickThrough(cfg.clickThrough());
}

void OverlayController::onWindowFrameChanged() {
    if (!m_frameArmed || !m_window) return;
    ConfigModel* cfg = PluginContext::instance().config();
    if (!cfg || !cfg->rememberFrame()) return;
    cfg->setWindowFrame(m_window->geometry());
}

void OverlayController::applySnapshot(QList<SpeakerSnapshot> snapshot) {
    if (!m_window) return;

    // Remove only the tail stretch/spacer; keep row widgets in place.
    for (int i = m_layout->count() - 1; i >= 0; --i) {
        QLayoutItem* item = m_layout->itemAt(i);
        if (item && !item->widget()) {
            delete m_layout->takeAt(i);
        }
    }

    QSet<uint16_t> seen;
    for (const SpeakerSnapshot& s : snapshot) {
        seen.insert(s.clientID);
        SpeakerRowWidget* row = m_rowsByClient.value(s.clientID, nullptr);
        if (!row) {
            // Parent to the content container (owner of m_layout), not the
            // top-level window — otherwise rows get laid out on top of the
            // custom title bar.
            QWidget* contentParent = m_layout->parentWidget();
            row = new SpeakerRowWidget(s, m_showChannel, contentParent);
            m_rowsByClient.insert(s.clientID, row);
        } else {
            row->updateSnapshot(s, m_showChannel, m_fadeDuration);
            m_layout->removeWidget(row);
        }
        m_layout->addWidget(row);
        row->show();
    }
    m_layout->addStretch();

    // Animate out any rows no longer present.
    const auto keys = m_rowsByClient.keys();
    for (uint16_t key : keys) {
        if (seen.contains(key)) continue;
        SpeakerRowWidget* row = m_rowsByClient.take(key);
        m_layout->removeWidget(row);
        row->animateRemoval(200, [row]() {
            row->deleteLater();
        });
    }

    // Auto-fit height to contents — but only when the user is not locking
    // the frame. Otherwise we'd fight their manual sizing on every snapshot.
    if (!m_rememberFrame) {
        m_window->adjustSize();
    }
}
