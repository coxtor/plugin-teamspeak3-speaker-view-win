#pragma once

#include <QtWidgets/QWidget>

// Top-level overlay window. A standard Qt::Tool window so it does not
// steal focus from the TS3 main window.
class OverlayWidget : public QWidget {
    Q_OBJECT
public:
    explicit OverlayWidget(QWidget* parent = nullptr);

    void setAlwaysOnTop(bool on);
    void setClickThrough(bool on);

signals:
    void frameChanged();

protected:
    void moveEvent(QMoveEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void applyWindowFlags();
    bool m_alwaysOnTop = true;
    bool m_clickThrough = false;
};
