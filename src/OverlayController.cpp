#include "OverlayController.h"

#include "ConfigModel.h"
#include "OverlayWidget.h"
#include "PluginContext.h"
#include "SpeakerRowWidget.h"

#include <QtCore/QSet>
#include <QtCore/QTimer>
#include <QtWidgets/QVBoxLayout>

OverlayController::OverlayController(QObject* parent) : QObject(parent) {
    m_window = new OverlayWidget();
    m_layout = new QVBoxLayout(m_window);
    m_layout->setContentsMargins(6, 6, 6, 6);
    m_layout->setSpacing(4);
    m_layout->addStretch();

    // Restore saved frame if any.
    if (auto* cfg = PluginContext::instance().config()) {
        QRect saved = cfg->windowFrame();
        if (cfg->rememberFrame() && saved.isValid()) {
            m_window->setGeometry(saved);
        } else {
            m_window->move(120, 120);
        }
    }

    m_window->show();

    connect(m_window, &OverlayWidget::frameChanged,
            this, &OverlayController::onWindowFrameChanged);

    // Arm frame persistence after the current event loop turn, so the initial
    // setGeometry doesn't get persisted back on top of nothing.
    QTimer::singleShot(0, this, [this]() { m_frameArmed = true; });
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
    m_showAvatar = cfg.showAvatar();
    m_showChannel = cfg.showChannel();
    m_fadeDuration = cfg.fadeOutSeconds();
    m_rememberFrame = cfg.rememberFrame();

    if (!m_window) return;
    m_window->setAlwaysOnTop(cfg.alwaysOnTop());
    m_window->setBorderless(cfg.borderless());
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
            row = new SpeakerRowWidget(s, m_showAvatar, m_showChannel, m_window);
            m_rowsByClient.insert(s.clientID, row);
        } else {
            row->updateSnapshot(s, m_showAvatar, m_showChannel, m_fadeDuration);
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
