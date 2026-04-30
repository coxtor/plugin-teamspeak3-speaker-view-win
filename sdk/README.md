# TeamSpeak 3 Plugin SDK headers

The SDK headers are not committed to this repository — they belong to
TeamSpeak Systems GmbH and their license typically restricts redistribution.

Download the **TeamSpeak 3 Client Plugin SDK** from
<https://teamspeak.com/en/downloads/> ("SDKs" section; look for the one
called "ts3client-pluginsdk-xx", NOT the "TS Client Library SDK").

The four files needed:

- `plugin_definitions.h` (top of `include/`)
- `ts3_functions.h` (top of `include/`)
- `teamspeak/public_definitions.h`
- `teamspeak/public_errors.h`

Expected layout once supplied:

```
sdk/
├── plugin_definitions.h
├── ts3_functions.h
└── teamspeak/
    ├── public_definitions.h
    └── public_errors.h
```

The build system will be configured to accept either this in-repo layout
or a `TS3_SDK_DIR` variable pointing at the extracted SDK. Exact
mechanism TBD — see the "Open decisions" section in the top-level README.
