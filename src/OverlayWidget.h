#pragma once

#include <QtWidgets/QWidget>

// Top-level overlay window. Standard Qt::Tool so it does not steal focus
// from the TS3 main window. Window flags are assigned only once in the
// constructor; runtime toggles use Win32 APIs to avoid HWND recreation
// (which crashes TS3 when children have active graphics effects).
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
    void moveEvent(QMoveEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void applyAlwaysOnTopNative();
    void applyClickThroughNative();

    bool m_alwaysOnTop = true;
    bool m_clickThrough = false;
};
