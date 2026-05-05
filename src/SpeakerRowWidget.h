#pragma once

#include "SpeakerTracker.h"

#include <QtWidgets/QWidget>

class QLabel;
class QGraphicsOpacityEffect;

class SpeakerRowWidget : public QWidget {
    Q_OBJECT
public:
    explicit SpeakerRowWidget(const SpeakerSnapshot& snapshot,
                              bool showChannel,
                              QWidget* parent = nullptr);

    uint16_t clientID() const { return m_clientID; }

    void updateSnapshot(const SpeakerSnapshot& s,
                        bool showChannel, double fadeDuration);

    void animateRemoval(int durationMs, std::function<void()> completion);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    uint16_t m_clientID = 0;
    QLabel* m_nickLabel = nullptr;
    QLabel* m_channelLabel = nullptr;
    QGraphicsOpacityEffect* m_opacity = nullptr;
    bool m_talking = false;
};
