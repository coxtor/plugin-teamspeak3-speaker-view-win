#include "NativeStyle.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QPalette>
#include <QtWidgets/QStyle>
#include <QtWidgets/QStyleFactory>
#include <QtWidgets/QWidget>

namespace {
// Cached so every call doesn't re-instantiate. QStyleFactory returns a new
// owned pointer each time; widgets don't take ownership when we setStyle(),
// so we intentionally leak these singletons for the plugin's lifetime.
QStyle* nativeStyle() {
    static QStyle* s = []() -> QStyle* {
        // Try the Windows 10/11 native style first, fall back to the older
        // windowsvista style, then plain windows. On a Qt 5.15 build shipped
        // with TS3 only windowsvista is guaranteed to exist.
        for (const char* name : {"windows11", "windowsvista", "windows"}) {
            if (QStyle* cand = QStyleFactory::create(QString::fromLatin1(name))) {
                return cand;
            }
        }
        return nullptr;
    }();
    return s;
}

QPalette systemPalette() {
    // QGuiApplication::palette() reflects the host's override palette. The
    // style's standardPalette() is the one the native style ships with —
    // that's what a normal Windows app starts from.
    if (QStyle* s = nativeStyle()) return s->standardPalette();
    return QPalette();
}
}

void applyNativeWindowsLook(QWidget* w) {
    if (!w) return;
    if (QStyle* s = nativeStyle()) {
        w->setStyle(s);
    }
    w->setPalette(systemPalette());
}
