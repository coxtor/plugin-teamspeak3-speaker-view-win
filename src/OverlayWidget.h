#pragma once

#include <QtWidgets/QWidget>

// Top-level overlay window. Frameless + translucent, tool-style so it does
// not steal focus from the TS3 main window.
class OverlayWidget : public QWidget {
    Q_OBJECT
public:
    explicit OverlayWidget(QWidget* parent = nullptr);

    void setAlwaysOnTop(bool on);
    void setClickThrough(bool on);
    void setBorderless(bool on);

signals:
    void frameChanged();

protected:
    void paintEvent(QPaintEvent* event) override;
    void moveEvent(QMoveEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void applyWindowFlags();
    bool m_alwaysOnTop = true;
    bool m_clickThrough = false;
    bool m_borderless = false;
};
