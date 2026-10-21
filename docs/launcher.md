# Mediterranean launcher

`MediterraneanLauncher.exe` is our native Windows launcher. It runs beside `RPGNative.exe` and provides Play, display options and game status. It uses Win32/GDI+ for its interface and an embedded capture from our own game; the game renderer remains DirectX 11.

## Build and play

From the project root:

```powershell
.\scripts\build-native.ps1 -Backend Ninja -Configuration Release -Test -Launcher
```

For subsequent sessions, double-click `build-ninja-release/MediterraneanLauncher.exe`. The Debug counterpart is `build-ninja-debug/MediterraneanLauncher.exe`. A Visual Studio build produces `build-local/Debug/MediterraneanLauncher.exe` (or `Release/`). Both executables must stay in the same output folder. Building the launcher target also builds the game.

Choose **Borderless fullscreen** or **Windowed**, then click **Play**. The window-size presets are 1280x720, 1600x900 and 1920x1080. Windowed mode fits the selected size to the available desktop work area while retaining its aspect ratio; fullscreen uses the monitor dimensions. F11 remains available in the game.

After Play, the game opens **Choose Your Character**. Review the current hero's nickname, level, class and equipped 3D preview, then select **Enter World**. An existing `saves/patrol.save` supplies the character and campaign state; without a save, the level-1 starter is offered. An unreadable or invalid save disables entry and offers Refresh after repairing the file. Selection never writes a save. F5 saves and F9 loads once inside the world. Closing the launcher never terminates the game or saves gameplay progress.

Use Tab/Shift+Tab to move focus, Enter/Space to activate, arrow keys or the preview buttons to rotate, and R to refresh the character. Escape focuses Exit game; confirm to leave. F11 works on this screen too. Gameplay and combat audio remain inactive until entry. If the file changes while this screen is open, Enter World refreshes the displayed hero and requires another confirmation.

There is currently one campaign character, named **Frontier guard**, of class **Dawnwatch guard**. The name/class are shared with the HUD and character panel. This screen does not add character creation, renaming, additional classes or multiple save slots; the save format remains unchanged.

The launcher minimizes after starting the game, disables repeat launches while that game is open, and returns after the child exits. It reports a nonzero exit code and allows another attempt. Another launcher or ordinary game instance from the same executable folder is rejected. Debug and Release folders are separate instances; avoid playing both against the shared save at once.

**Game guide** opens `START_AND_DEBUG.md` using the associated Windows app. **Saves folder** opens the project's save directory, creating that directory if it does not exist. An unavailable file association or folder produces an inline message.

## Preferences and debugging

Display mode and window size are stored in `launcher-settings.ini` beside the launcher when you press Play or close it. Debug, Release and Visual Studio outputs therefore have independent preferences. Invalid preferences fall back to defaults with a message. Failure to write preferences does not prevent starting with the current choices. No registry, monitor mode or machine setting is changed.

Use `-Launcher` to open the launcher through the build helper; `-Run` still starts the game directly. These switches are mutually exclusive. To debug game code with F5, keep **RPGNative** as the Visual Studio startup project. Selecting **MediterraneanLauncher** instead debugs the launcher; Visual Studio does not automatically attach to its child game.

The game also accepts these interactive arguments:

```powershell
.\build-ninja-debug\RPGNative.exe --windowed --resolution 1600x900
.\build-ninja-debug\RPGNative.exe --fullscreen
```

Direct game launches also open character selection. Legacy `--resume` arguments and the old resume preference field remain readable for compatibility, but do not bypass selection. Only the documented resolution presets are accepted. Existing `--smoke-test`, `--benchmark` and `--benchmark-suite` developer modes retain their hidden behavior and are separate from launcher options.

## Validation and current scope

`CharacterSelectionTests` covers saved/new/invalid/repaired character states, nickname/level/class, frozen gameplay, keyboard focus, cancelled mouse gestures, resize/focus changes, changed-save review, exact campaign restoration and held-input isolation. Hidden DirectX captures cover 960x600, 1280x720, 1920x1080 and 2560x1080 plus first-run and error views. These checks use isolated build-directory fixtures. Live acceptance is manual check 176.

`LauncherTests` covers preference/argument validation, Windows argument escaping, Unicode and spaced executable paths, working directory, duplicate launches, process exit/relaunch, closing the parent's handle without terminating the child, and launching the real game through the process service. Hidden engine startup checks cover all three window sizes, desktop fitting, fullscreen, restored veteran state and corrupt-save rejection. These tests use fixture saves and do not modify player progress.

`LauncherUiTests` checks control selection, enabled states, tab order, the default Play action, resource availability and layout bounds, and creates 100/150/200% diagnostic renders plus a missing-game view. Hidden native combo boxes omit their selected owner-drawn item from `WM_PRINT`, so the capture invokes the same selection drawing callback explicitly. These are application-rendered diagnostics; live mouse/keyboard use, minimize/restore and moving between monitors remain manual checks 174–175.

The launcher currently serves local development builds. Save and guide paths refer to this checkout, as they did before; simply copying the executables to another PC is not a complete distribution workflow. Accounts, downloads, patch servers, signed updates, installation and portable save locations are future distribution work. No network request, asset download or dependency installation is performed by this launcher.
