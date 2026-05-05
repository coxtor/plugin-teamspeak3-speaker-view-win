#include "Log.h"

#include "PluginContext.h"

#include <QtCore/QByteArray>

extern "C" {
#include "plugin_definitions.h"
#include "teamspeak/public_definitions.h"
#include "ts3_functions.h"
}

void sv_log(const QString& msg) {
    const TS3Functions* fns = PluginContext::instance().ts3Functions();
    if (!fns || !fns->logMessage) return;
    const QByteArray utf8 = msg.toUtf8();
    fns->logMessage(utf8.constData(), LogLevel_INFO, "speakerview", 0);
}
