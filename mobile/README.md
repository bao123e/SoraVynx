# SoraVynx Android

React Native menu + ARM64 integration preview for **Player/Event**. No Auto Daily. Uses [`@react-native-menu/menu`](https://github.com/react-native-menu/menu) 2.0.0. Read [installation and limitations](INSTALL.vi.md) before installing.

The UI runs as a companion app; a signature-protected receiver connects to the backend inside the integrated game APK. It cannot control the stock game. Feature switches reflect runtime capability and acknowledgement. Windows interpreter offsets are never reused.

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

Use the **Build Android Player and Event preview** Actions workflow for the APK pair. It verifies the original XAPK hash against `integration/profile.json`, builds the menu and native backend, adds the loader, and signs both APKs with the same ephemeral key. Build tools: Node 22, JDK 17, Android SDK 36, NDK 27.1, APKTool 2.12.1; Dobby pinned at `5dfc8546954ce3b3198132ab13fddb89ee92cdd7`, logging disabled and an explicit ELF relocation portability fix in `tools/prepare_dobby.py`. If the upstream `latest` XAPK changes, the build intentionally stops for re-analysis.

Game APKs are built from the supplied public download during CI; they are not source files. Build output is available in the workflow artifact for 14 days. Host tests cover typed callback filtering, Event score bounds, independent flags, failed capability checks, malformed IPC responses and APK merge integrity. **On-device startup, game integrity/login compatibility and gameplay effects remain unverified.**

This directory is AGPL-3.0-or-later, as in the repository. Dobby is Apache-2.0; React Native and `@react-native-menu/menu` are MIT; Be Vietnam Pro is SIL OFL (see `source/assets/fonts/OFL.txt`). Game assets/libraries retain their original ownership.
