// TeamSpeak 3 plugin C-ABI entry points. All exports are thin — they
// forward to PluginContext and its owned objects. TS3 calls these from
// unspecified threads; anything that touches Qt widgets must be marshalled
// to the Qt main thread.

#include "ConfigDialog.h"
#include "PluginContext.h"
#include "SpeakerTracker.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QMetaObject>
#include <QtCore/QString>

extern "C" {
#include "plugin_definitions.h"
#include "teamspeak/public_definitions.h"
#include "ts3_functions.h"
}

#define PLUGIN_API_VERSION 26
#define PLUGIN_NAME        "Speaker View"
#define PLUGIN_VERSION     "0.2.0"
#define PLUGIN_AUTHOR      "plugin-teamspeak3-speaker-view-win contributors"
#define PLUGIN_DESCRIPTION "Separate overlay window listing currently speaking members of your channel, with a configurable fade-out delay."

#define SV_EXPORT __declspec(dllexport)

extern "C" {

SV_EXPORT const char* ts3plugin_name(void) { return PLUGIN_NAME; }
SV_EXPORT const char* ts3plugin_version(void) { return PLUGIN_VERSION; }
SV_EXPORT int ts3plugin_apiVersion(void) { return PLUGIN_API_VERSION; }
SV_EXPORT const char* ts3plugin_author(void) { return PLUGIN_AUTHOR; }
SV_EXPORT const char* ts3plugin_description(void) { return PLUGIN_DESCRIPTION; }

SV_EXPORT void ts3plugin_setFunctionPointers(const struct TS3Functions funcs) {
    static TS3Functions sFns;
    sFns = funcs;
    PluginContext::instance().setTS3Functions(&sFns);
}

SV_EXPORT int ts3plugin_init(void) {
    PluginContext::instance().setupOnInit();
    return 0;
}

SV_EXPORT void ts3plugin_shutdown(void) {
    PluginContext::instance().teardownOnShutdown();
}

SV_EXPORT void ts3plugin_registerPluginID(const char* id) {
    PluginContext::instance().setPluginID(QString::fromUtf8(id));
}

SV_EXPORT int ts3plugin_offersConfigure(void) {
    // QT_THREAD: TS3 calls ts3plugin_configure on its own Qt main thread,
    // so we can construct QDialogs directly without extra marshalling.
    return PLUGIN_OFFERS_CONFIGURE_QT_THREAD;
}

SV_EXPORT void ts3plugin_configure(void* /*handle*/, void* qParentWidget) {
    QWidget* parent = reinterpret_cast<QWidget*>(qParentWidget);
    ConfigDialog::presentModal(parent);
}

SV_EXPORT void ts3plugin_onTalkStatusChangeEvent(uint64 serverConnectionHandlerID,
                                                  int status,
                                                  int isReceivedWhisper,
                                                  anyID clientID) {
    PluginContext::instance().setActiveServerHandlerID(serverConnectionHandlerID);
    if (auto* t = PluginContext::instance().tracker()) {
        t->handleTalkStatus(status, clientID, serverConnectionHandlerID,
                            isReceivedWhisper != 0);
    }
}

SV_EXPORT void ts3plugin_onClientMoveEvent(uint64 serverConnectionHandlerID,
                                            anyID clientID,
                                            uint64 oldChannelID,
                                            uint64 newChannelID,
                                            int /*visibility*/,
                                            const char* /*moveMessage*/) {
    if (auto* t = PluginContext::instance().tracker()) {
        t->handleClientMoved(oldChannelID, newChannelID, clientID,
                             serverConnectionHandlerID);
    }
}

SV_EXPORT void ts3plugin_onConnectStatusChangeEvent(uint64 serverConnectionHandlerID,
                                                     int newStatus,
                                                     unsigned int /*errorNumber*/) {
    if (auto* t = PluginContext::instance().tracker()) {
        t->handleConnectStatus(newStatus, serverConnectionHandlerID);
    }
}

SV_EXPORT void ts3plugin_onUpdateClientEvent(uint64 serverConnectionHandlerID,
                                              anyID clientID,
                                              anyID /*invokerID*/,
                                              const char* /*invokerName*/,
                                              const char* /*invokerUniqueIdentifier*/) {
    if (auto* t = PluginContext::instance().tracker()) {
        t->handleClientUpdate(clientID, serverConnectionHandlerID);
    }
}

}  // extern "C"
