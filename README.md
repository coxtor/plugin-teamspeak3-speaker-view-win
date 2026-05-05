# plugin-teamspeak3-speaker-view-win

Windows port of [plugin-teamspeak3-speaker-view](../plugin-teamspeak3-speaker-view)
— a TeamSpeak 3 client plugin showing a separate overlay window with the
members of the current channel who are currently speaking, similar to
Mumble's overlay. Each entry fades out after a configurable delay.

## Stack (locked)

- **UI**: Qt 5.15 (WidgetKit). TS3 itself embeds Qt 5.15, so end users
  install nothing extra — the plugin attaches to TS3's own Qt.
- **Compiler**: MSVC (x64). TS3 is MSVC-built; matching the toolchain
  avoids ABI friction.
- **Build**: CMake.
- **CI**: GitHub Actions on `windows-latest`. The CI runner is the only
  build machine — neither developer has a local Windows environment.
- **Plugin API**: 26 (TS3 Client 3.6.x).
- **Config dialog mode**: `PLUGIN_OFFERS_CONFIGURE_QT_THREAD` (works
  because we share Qt with the host).

## Prerequisites

- **TeamSpeak 3 Client 3.6.x**, 64-bit (Plugin API version 26).
- **Microsoft Visual C++ 2015–2022 Redistributable (x64)** — required
  because the plugin is MSVC-built. Without it, the DLL fails to load
  with `STATUS_INVALID_IMAGE_FORMAT` (`0xc0000020`). Download:
  <https://aka.ms/vs/17/release/vc_redist.x64.exe>.

## How to get a binary

Push to `main` (or open a PR). GitHub Actions builds a
`speakerview-<version>.ts3_plugin` file and attaches it as a workflow
artifact. Download the artifact ZIP, unzip, and double-click the
`.ts3_plugin` — TS3's built-in addon installer takes it from there.

For a manual install instead (no installer dialog), drop both
`speakerview.dll` and `speakerview.ini` from the artifact into:

```
%APPDATA%\TS3Client\plugins\
```

In TS3: **Tools → Options → Addons → Plugins → Reload All → enable
"Speaker View"**.

### If the CI SDK download fails

The CI tries to fetch the TS3 Plugin SDK headers from TeamSpeak's CDN
(exact URL may rot over time). If that step fails:

1. Download the SDK manually from <https://teamspeak.com/en/downloads/>.
2. Drop the four required headers into `sdk/` of the repo (see
   [sdk/README.md](sdk/README.md) for the exact layout).
3. Comment out the "Download TS3 Plugin SDK" step and remove the
   `-DTS3_SDK_DIR=...` flag from the Configure step.
4. Note: the SDK license typically restricts redistribution, so do not
   commit those headers to a public repo.

## Feature parity with macOS

All features are ported from the Mac implementation. Behavioural
invariants established on Mac must be preserved:

- **2 s VAD grace period** before fade-out begins (prevents flicker in
  natural speech pauses).
- **Phantom-entry guard**: `NOT_TALKING` for an already-removed client
  is a no-op.
- **Fade-clock integrity**: only the first `talking → not-talking`
  transition stamps the stop time.
- **Auto-fit height suppressed when `rememberFrame` is on** — the user's
  manual sizing is not overwritten on every snapshot.
- **Own-channel filter**: only speakers in the local user's channel.
- **Settings owned by the plugin**, not by TS3. Stored at
  `%APPDATA%\TS3Client\plugins\speaker-view\settings.ini`.

## Local build (optional, if you eventually get a Windows machine)

Prerequisites: Visual Studio 2022 (MSVC x64), CMake 3.21+, Qt 5.15.x
(official installer or `aqtinstall`), the TS3 Client Plugin SDK
(headers only).

```powershell
cmake -S . -B build `
      -G "Visual Studio 17 2022" -A x64 `
      -DTS3_SDK_DIR="C:\path\to\ts3client-pluginsdk-26" `
      -DCMAKE_PREFIX_PATH="C:\Qt\5.15.2\msvc2019_64"
cmake --build build --config Release
```

Output: `build\Release\speakerview.dll`. Copy it + `resources\speakerview.ini`
to the TS3 plugin directory above.

Alternatively, drop the four TS3 SDK headers into `sdk/` (see
[sdk/README.md](sdk/README.md)) and omit `-DTS3_SDK_DIR`.

## License

MIT — see [LICENSE](LICENSE). TS3 SDK headers belong to TeamSpeak Systems
GmbH and are not redistributed here.
