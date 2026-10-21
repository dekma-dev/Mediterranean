# Launcher artwork

`dawnwatch.png` is an unmodified 1280x720 capture of this project's own Dawnwatch village, made by `RPGNative.exe --smoke-test` on 2026-10-21. It was copied from the local `build-assets/vs-spawn-overview.png` diagnostic. It contains the project's geometry and procedural materials; no external game artwork was copied or downloaded.

The launcher embeds it through `launcher/Launcher.rc`. Scaling, the caption gradient and typography are drawn by the launcher at runtime. Replace this source image deliberately when the game art changes; normal builds do not regenerate the capture.
