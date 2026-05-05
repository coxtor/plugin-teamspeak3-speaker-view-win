#pragma once

#include "SpeakerTracker.h"

#include <QtCore/QHash>
#include <QtCore/QObject>
#include <cstdint>

class ConfigModel;
class OverlayWidget;
class SpeakerRowWidget;
class QVBoxLayout;

class OverlayController : public QObject {
    Q_OBJECT
public:
    explicit OverlayController(QObject* parent = nullptr);
    ~OverlayController() override;

    void applyConfig(const ConfigModel& cfg);

    // Show the overlay if hidden, hide it otherwise. Driven by the
    // Tools -> Plugins -> Speaker View menu entry.
    void toggleVisible();

    void teardown();

public slots:
    void applySnapshot(QList<SpeakerSnapshot> snapshot);

private slots:
    void onWindowFrameChanged();

private:
    OverlayWidget* m_window = nullptr;
    QVBoxLayout* m_layout = nullptr;
    QHash<uint16_t, SpeakerRowWidget*> m_rowsByClient;
    bool m_showChannel = false;
    double m_fadeDuration = 2.5;
    bool m_rememberFrame = true;
    bool m_frameArmed = false;
    // True while we're programmatically resizing the window to fit its
    // content. The resize fires moveEvent/resizeEvent -> frameChanged;
    // suppress persistence in that window so we don't rewrite settings.ini
    // on every speaker event.
    bool m_autoSizing = false;
};
