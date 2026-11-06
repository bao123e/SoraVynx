# SoraVynx Android

React Native menu + ARM64 integration preview for **Player/Event**. No Auto Daily. Uses [`@react-native-menu/menu`](https://github.com/react-native-menu/menu) 2.0.0. Read [installation and limitations](INSTALL.vi.md) before installing.

The UI runs as a companion app; a signature-protected receiver connects to the backend inside the integrated game APK. It cannot control the stock game. Feature switches reflect runtime capability and acknowledgement. Windows interpreter offsets are never reused.

[Download the APK pair directly](https://github.com/bao123e/SoraVynx/releases/download/android-v0.1.0-preview.1/SoraVynx-Android-0.1.0-preview.1.zip) · [Release and individual APKs](https://github.com/bao123e/SoraVynx/releases/tag/android-v0.1.0-preview.1). The release preserves the tested build's APK bytes and signing identity.

## Checks

```sh
npm ci
npm run typecheck
npm run lint
npm test
python -m unittest discover -s tests -p '*_test.py'
cc -D_GNU_SOURCE -std=c11 -Itests/stubs tests/native_test.c -ldl -o /tmp/soravynx-native-test
/tmp/soravynx-native-test
```

Use the **Build Android Player and Event preview** Actions workflow for the APK pair. It verifies the original XAPK hash against `integration/profile.json`, builds the menu and native backend, adds the loader, and signs both APKs with the same retained key. Public release builds now require repository secrets `SORAVYNX_ANDROID_KEYSTORE_BASE64`, `SORAVYNX_ANDROID_STORE_PASSWORD`, and optionally `SORAVYNX_ANDROID_KEY_ALIAS` (default `soravynx`). They stop before downloading/building if the key is missing. The already published preview.1 used a discarded key and cannot be updated in place; this change prevents repeating that mistake. Internal startup regression builds use disposable keys and do not publish replacement APKs. Build tools: Node 22, JDK 17, Android SDK 36, NDK 27.1, APKTool 2.12.1; Dobby pinned at `0527af1b00b8c99cbfdd1efad3ad3fca57fadc60`, logging disabled. The public CDN link may expire; provide a refreshed direct URL in the workflow `xapk_url` input if needed. A different XAPK hash intentionally stops the build for re-analysis.

Game APKs are built from the supplied public download during CI; they are not source files. Build output is available in the workflow artifact for 14 days. Host tests cover typed callback filtering, Event score bounds, independent flags, failed capability checks, malformed IPC responses and APK merge integrity. **On-device startup, game integrity/login compatibility and gameplay effects remain unverified.**

This directory is AGPL-3.0-or-later, as in the repository. Dobby is Apache-2.0; React Native and `@react-native-menu/menu` are MIT; Be Vietnam Pro is SIL OFL (see `source/assets/fonts/OFL.txt`). Game assets/libraries retain their original ownership.

[Windows USB data inspection and backup helper](data-helper/README.vi.md) checks external data access before any proposed replacement. It cannot read private app data or guarantee the whole 12 GB can be preserved.
