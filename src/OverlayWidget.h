#pragma once

#include <QtWidgets/QWidget>

// Top-level overlay window. Standard Qt::Tool so it does not steal focus
// from the TS3 main window. Runtime toggles of always-on-top /
// click-through use Win32 APIs on the live HWND rather than rebuilding
// the native window via setWindowFlags().
class OverlayWidget : public QWidget {
    Q_OBJECT
public:
    explicit OverlayWidget(QWidget* parent = nullptr);

    void setAlwaysOnTop(bool on);
    void setClickThrough(bool on);

signals:
    void frameChanged();

protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void moveEvent(QMoveEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void applyWindowFlags();            // constructor-only
    void applyAlwaysOnTopNative();      // runtime toggle
    void applyClickThroughNative();     // runtime toggle

    bool m_alwaysOnTop = true;
    bool m_clickThrough = false;
};
