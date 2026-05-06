# Speaker View for TeamSpeak 3

A small overlay window that shows who in your channel is currently
speaking — think Mumble's floating speaker list, but for TS3.

- Live list of speakers in your channel, sorted by activity.
- Per-speaker fade-out after a configurable delay, so rapid talkers
  don't make the list flicker.
- Frameless window with a custom title bar, native Windows drop shadow,
  and Windows 11 rounded corners.
- Follows the OS light / dark theme.
- Menu entries under **Tools → Plugins → Speaker View** to toggle the
  overlay, open the settings dialog, or quit TeamSpeak.
- Windows tray icon (notification area) with one-click Toggle / Settings
  / *Quit TeamSpeak*, independent of whether TS3 is in the foreground.
- Global hotkeys for Toggle and Quit, bindable under
  **Tools → Options → Hotkeys**.
- Localhost HTTP control server for Stream Deck and other external
  integrations — the cross-platform
  [`streamdeck-plugin-teamspeak-control`](../streamdeck-plugin-teamspeak-control)
  plugin pairs with this endpoint and reflects live state on the keys.

This is the **Windows port** of the original macOS plugin
([plugin-teamspeak3-speaker-view](../plugin-teamspeak3-speaker-view));
feature parity with the Mac version is preserved.

## Install

1. Install the **Microsoft Visual C++ 2015–2022 Redistributable (x64)**:
   <https://aka.ms/vs/17/release/vc_redist.x64.exe>. Without it the DLL
   fails to load with `STATUS_INVALID_IMAGE_FORMAT` (`0xc0000020`).
2. Grab the latest `speakerview-<version>.ts3_plugin` from the
   [Actions artifacts](../../actions) (open the most recent green
   `build` run, download the `speakerview-windows-x64` artifact, unzip).
3. Double-click the `.ts3_plugin` file. TS3's built-in addon installer
   takes it from there.
4. In TeamSpeak: **Tools → Options → Addons → Plugins** → enable
   *Speaker View*.

Requires **TeamSpeak 3 Client 3.6.x, 64-bit** (Plugin API version 26).

### Manual install (no installer dialog)

The artifact ZIP also contains raw `speakerview.dll` + `speakerview.ini`.
Drop both into

```
%APPDATA%\TS3Client\plugins\
```

then in TS3: **Tools → Options → Addons → Plugins → Reload All** →
enable *Speaker View*.

## Usage

The overlay appears as soon as the plugin is enabled. Drag its title
bar to move it; close it with the `×` in the title bar. Bring it back
via **Tools → Plugins → Speaker View → Toggle Speaker View**.

Settings dialog: **Tools → Options → Addons → Plugins → Speaker View →
Settings** (the gear icon in TS3's plugin list).

The dialog has two tabs:

**General** — overlay-related settings:

| Setting | Effect |
| --- | --- |
| Fade-out after talking | How long a speaker stays on screen after they stop (0–10 s). |
| Display mode | *All speakers in my channel* / *Only the latest speaker*. |
| Always on top | Keeps the overlay above other windows (default: on). |
| Remember position | Restores window position + width across sessions (default: on). |
| Click-through | Mouse events pass through the overlay to the window below. |
| Show channel name | Adds a second line per speaker with the channel name. |
| Also show my own voice | Includes you in the list when you talk. |
| Show tray icon | Windows notification-area icon with Toggle / Settings / Quit TeamSpeak. |

**Stream Deck / HTTP** — external-control endpoint:

| Setting | Effect |
| --- | --- |
| Enable HTTP control server | Bind a tiny HTTP/1.1 listener to `127.0.0.1:<port>` for Stream Deck / curl / scripts. |
| Port | Listening port; click *Apply* to restart on a new value. |

Routes (all `GET`, JSON responses):

| Path | Effect |
| --- | --- |
| `/health` | Liveness probe — `speakerview-control ok`. |
| `/mic/state`, `/mic/toggle` | Microphone (input) mute. |
| `/speaker/state`, `/speaker/toggle` | Speaker (output) mute. |
| `/away/state`, `/away/toggle` | Away ↔ available. |
| `/silent/state`, `/silent/toggle` | Combined: mic + speaker + away in lockstep. |
| `/quit` | Hard-quit TS3 (`exit(0)`). |

When no TS3 server is connected the toggle/state endpoints return
`{"…":null,"connected":false}` and the request is a no-op. Smoke-test
from PowerShell:

```powershell
curl http://127.0.0.1:30000/health
curl http://127.0.0.1:30000/mic/toggle
```

Settings are stored per user at
`%APPDATA%\TS3Client\plugins\speaker-view\settings.ini`, separate from
TS3's own config.

### Quick access — hotkeys

Two hotkey keywords are exposed to TS3's hotkey manager:

- **Toggle Speaker View**
- **Quit TeamSpeak (hard)**

Bind them under **Tools → Options → Hotkeys → Add → Plugins → Speaker
View** to keys of your choice.

### Quick access — Stream Deck

Pair this plugin with the
[`streamdeck-plugin-teamspeak-control`](../streamdeck-plugin-teamspeak-control)
Stream Deck plugin: drag the *Mic / Speaker / Away / Silent / Quit*
actions onto your keys, set the host to `127.0.0.1` and the port to
match the one shown in the *Stream Deck / HTTP* tab. Each button polls
the state endpoint at the configured interval, so the icon always
reflects the real TS3 state — even when you toggle mute via the UI or
a hotkey.

## Behaviour that was deliberately designed

- **2 s grace period** before fade-out starts — prevents flicker during
  normal speech pauses and breaths.
- **Own-channel filter** — you only see speakers in the same channel as
  you; nobody from elsewhere pops up.
- **Dynamic height** — the window grows when more people talk and
  shrinks back when they stop, while your manually chosen position and
  width stay put.
- **Off-screen frame discard** — if your saved position points at a
  monitor that is no longer attached, the plugin falls back to a safe
  default instead of hiding the window on a disconnected display.

## Build from source

There is no developer Windows machine. **CI is the build machine.**

Workflow (`.github/workflows/build.yml`) triggers on every push to
`main` and on every pull request. It produces:

- `speakerview.dll` (the plugin binary)
- `speakerview.ini` (TS3 metadata)
- `speakerview-<version>.ts3_plugin` (zipped, ready to double-click)

…and uploads them as a single workflow artifact named
`speakerview-windows-x64`. To get a build:

1. Push a branch (or open a PR). The *build* workflow runs automatically.
2. When it finishes (≈ 5 min), open the run on the Actions page.
3. Scroll to the *Artifacts* section at the bottom; download the ZIP.
4. Inside, the `.ts3_plugin` is what end users want — see *Install* above.

Toolchain used by CI:

- GitHub Actions `windows-latest` runner
- MSVC x64 via `ilammy/msvc-dev-cmd`
- Qt 5.15.2 via `jurplel/install-qt-action` (with the *Network* module
  added in 0.5.0 for the HTTP control server)
- Ninja generator
- TS3 Plugin SDK headers fetched from
  [github.com/teamspeak/ts3client-pluginsdk](https://github.com/teamspeak/ts3client-pluginsdk)
  at a pinned commit (API version 26)

### Local build (if you do have Windows)

Prerequisites: Visual Studio 2022 Build Tools (MSVC x64), CMake 3.21+,
Qt 5.15.x, and the TS3 Client Plugin SDK (headers only).

```powershell
cmake -S . -B build `
      -G Ninja `
      -DCMAKE_BUILD_TYPE=Release `
      -DTS3_SDK_DIR="C:\path\to\ts3client-pluginsdk" `
      -DCMAKE_PREFIX_PATH="C:\Qt\5.15.2\msvc2019_64"
cmake --build build --parallel
```

Output: `build\speakerview.dll`. Copy it + `resources\speakerview.ini`
to `%APPDATA%\TS3Client\plugins\`, or drop the four TS3 SDK headers
into `sdk/` in the repo (see [sdk/README.md](sdk/README.md)) and omit
the `-DTS3_SDK_DIR` flag.

### If the CI SDK download fails

TeamSpeak has rotated SDK URLs before (the old
`files.teamspeak-services.com` CDN is dead). If the *Download TS3
Plugin SDK* step fails:

1. Download the SDK manually from <https://teamspeak.com/en/downloads/>
   or <https://github.com/teamspeak/ts3client-pluginsdk>.
2. Drop the four required headers into `sdk/` (see
   [sdk/README.md](sdk/README.md)).
3. Comment out the *Download TS3 Plugin SDK* step in the workflow and
   remove `-DTS3_SDK_DIR=...` from the Configure step.
4. Do not commit the headers to a public repo — the SDK license
   typically restricts redistribution.

## Project layout

```
.github/workflows/build.yml   CI: builds the .ts3_plugin artifact
CMakeLists.txt                build definition (MSVC + Qt 5.15)
resources/speakerview.ini     plugin metadata TS3 reads on load
sdk/                          (optional) TS3 SDK headers; empty in repo
src/plugin.cpp                TS3 C-ABI entry points
src/PluginContext.{h,cpp}     singleton wiring: config/tracker/overlay
src/TS3Bridge.{h,cpp}         ts3_functions wrappers with QString/RAII
src/SpeakerTracker.{h,cpp}    talk-status state, VAD grace, fade clock
src/OverlayController.{h,cpp} snapshot → rows, geometry persistence
src/OverlayWidget.{h,cpp}     frameless top-level window + DWM effects
src/SpeakerRowWidget.{h,cpp}  one row: dot + nick + optional channel
src/ConfigModel.{h,cpp}       QSettings-backed settings
src/ConfigDialog.{h,cpp}      PLUGIN_OFFERS_CONFIGURE_QT_THREAD dialog
src/Log.{h,cpp}               forwards to TS3's logMessage
tasks/lessons.md              pitfalls hit during the port
tasks/todo.md                 checklist
```

## License

MIT — see [LICENSE](LICENSE). TS3 SDK headers remain the property of
TeamSpeak Systems GmbH and are not redistributed in this repo.
