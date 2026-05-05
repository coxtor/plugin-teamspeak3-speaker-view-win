#include "NativeStyle.h"

#include <QtGui/QPalette>
#include <QtWidgets/QWidget>

namespace {
// Hand-rolled light palette matching the Windows 10/11 default theme.
// We do NOT change the QStyle — per-widget setStyle() during construction
// in a plugin whose host uses a different global style is unreliable on
// Qt 5.15 and was making our top-level window fail to appear at all.
// Swapping only the palette gives a bright, native-looking UI without
// touching the widget's painting backend.
QPalette lightWindowsPalette() {
    QPalette p;
    const QColor window(0xF0, 0xF0, 0xF0);
    const QColor base(0xFF, 0xFF, 0xFF);
    const QColor alt(0xF7, 0xF7, 0xF7);
    const QColor button(0xE1, 0xE1, 0xE1);
    const QColor accent(0x00, 0x78, 0xD4);         // Windows default accent
    const QColor text = Qt::black;
    const QColor disabledText(0x80, 0x80, 0x80);

    p.setColor(QPalette::Window,          window);
    p.setColor(QPalette::WindowText,      text);
    p.setColor(QPalette::Base,            base);
    p.setColor(QPalette::AlternateBase,   alt);
    p.setColor(QPalette::ToolTipBase,     QColor(0xFF, 0xFF, 0xDC));
    p.setColor(QPalette::ToolTipText,     text);
    p.setColor(QPalette::Text,            text);
    p.setColor(QPalette::Button,          button);
    p.setColor(QPalette::ButtonText,      text);
    p.setColor(QPalette::BrightText,      Qt::red);
    p.setColor(QPalette::Highlight,       accent);
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::Link,            accent);

    p.setColor(QPalette::Disabled, QPalette::WindowText, disabledText);
    p.setColor(QPalette::Disabled, QPalette::Text,       disabledText);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, disabledText);
    return p;
}
}

void applyNativeWindowsLook(QWidget* w) {
    if (!w) return;
    w->setPalette(lightWindowsPalette());
}
