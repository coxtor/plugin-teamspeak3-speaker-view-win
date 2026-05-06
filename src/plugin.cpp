// TeamSpeak 3 plugin C-ABI entry points. All exports are thin — they
// forward to PluginContext and its owned objects. TS3 calls these from
// unspecified threads; anything that touches Qt widgets must be marshalled
// to the Qt main thread.

#include "ConfigDialog.h"
#include "OverlayController.h"
#include "PluginContext.h"
#include "SpeakerTracker.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QMetaObject>
#include <QtCore/QString>

#include <cstdlib>
#include <cstring>

extern "C" {
#include "plugin_definitions.h"
#include "teamspeak/public_definitions.h"
#include "ts3_functions.h"
}

namespace {
// Menu item IDs. Numbering is local to this plugin; TS3 prefixes them
// with our registered plugin ID so there are no cross-plugin collisions.
enum : int {
    kMenuToggleOverlay = 1,
    kMenuOpenSettings  = 2,
};
}

#define PLUGIN_API_VERSION 26
#define PLUGIN_NAME        "Speaker View"
#define PLUGIN_VERSION     "0.4.5"
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

// Required by TS3's SDK contract when initMenus / infoData allocates
// memory: TS3 keeps our PluginMenuItem** and item buffers around, then
// later calls us back with this function to release them. Without this
// export TS3 silently drops the entire menu structure.
SV_EXPORT void ts3plugin_freeMemory(void* data) {
    std::free(data);
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

// ---------------------------------------------------------------------------
// Plugins menu entry: a single toggle that shows/hides the overlay.
// TS3 wires it into Tools (Extras) -> Plugins -> Speaker View.
//
// Memory ownership: TS3 takes ownership of the PluginMenuItem array and
// its contents after initMenus returns, and frees them with free().
// Allocations must therefore come from malloc, not new/new[].
// ---------------------------------------------------------------------------

namespace {
PluginMenuItem* makeMenuItem(PluginMenuType type, int id,
                             const char* text, const char* icon) {
    auto* item = static_cast<PluginMenuItem*>(std::malloc(sizeof(PluginMenuItem)));
    item->type = type;
    item->id = id;
    std::strncpy(item->text, text, PLUGIN_MENU_BUFSZ - 1);
    item->text[PLUGIN_MENU_BUFSZ - 1] = '\0';
    std::strncpy(item->icon, icon ? icon : "", PLUGIN_MENU_BUFSZ - 1);
    item->icon[PLUGIN_MENU_BUFSZ - 1] = '\0';
    return item;
}
}

SV_EXPORT void ts3plugin_initMenus(struct PluginMenuItem*** menuItems, char** menuIcon) {
    constexpr int kItemCount = 2;
    auto* items = static_cast<PluginMenuItem**>(
        std::malloc(sizeof(PluginMenuItem*) * (kItemCount + 1)));
    items[0] = makeMenuItem(PLUGIN_MENU_TYPE_GLOBAL, kMenuToggleOverlay,
                            "Toggle Speaker View", "");
    items[1] = makeMenuItem(PLUGIN_MENU_TYPE_GLOBAL, kMenuOpenSettings,
                            "Settings\xE2\x80\xA6", "");  // "Settings…"
    items[kItemCount] = nullptr;  // terminator
    *menuItems = items;
    *menuIcon = nullptr;
}

SV_EXPORT void ts3plugin_onMenuItemEvent(uint64 /*serverConnectionHandlerID*/,
                                         enum PluginMenuType type,
                                         int menuItemID,
                                         uint64 /*selectedItemID*/) {
    if (type != PLUGIN_MENU_TYPE_GLOBAL) return;
    if (menuItemID == kMenuToggleOverlay) {
        if (auto* o = PluginContext::instance().overlay()) {
            o->toggleVisible();
        }
    } else if (menuItemID == kMenuOpenSettings) {
        // Open the same dialog TS3 invokes through the gear icon. Parent
        // is null — TS3 doesn't pass a parent here, and ConfigDialog is
        // tolerant of that.
        ConfigDialog::presentModal(nullptr);
    }
}

}  // extern "C"
