#include "NativeStyle.h"

#include <QtWidgets/QStyle>
#include <QtWidgets/QStyleFactory>
#include <QtWidgets/QWidget>

namespace {
// Cached so every call doesn't re-instantiate. QStyleFactory returns a new
// owned pointer each time; widgets don't take ownership when we setStyle(),
// so we intentionally leak this singleton for the plugin's lifetime.
//
// On the Qt 5.15 shipped with TS3 the concrete style plugin availability is
// not guaranteed. If NONE of our preferred native styles exist, we must
// return nullptr and let the caller keep the inherited host style —
// returning QPalette() or silently clobbering the widget's palette with a
// default-constructed one would paint everything black.
QStyle* nativeStyle() {
    static QStyle* s = []() -> QStyle* {
        for (const char* name : {"windows11", "windowsvista", "windows"}) {
            if (QStyle* cand = QStyleFactory::create(QString::fromLatin1(name))) {
                return cand;
            }
        }
        return nullptr;
    }();
    return s;
}
}

void applyNativeWindowsLook(QWidget* w) {
    if (!w) return;
    QStyle* s = nativeStyle();
    if (!s) return;  // No native style available — keep host defaults.
    w->setStyle(s);
    w->setPalette(s->standardPalette());
}
