# Action-bar readability — 2026-11-18

The current request is to make the ability bar larger. Revisited the previously decoded 23-second frame from `D:\assets\learning_material\pve_raid.mp4` (`build-hud-density/reference-raid/frames/f000004.jpg`) and the native `build-hud-density/combat-after.png`. The reference separates illuminated ability artwork, small binding labels and cooldown numbers. Our compact transform reduced the whole dock, including the artwork and labels. This pass prioritizes the requested readability over copying that recording's exact pixel density. This was a sampled frame comparison, not a new full-video review.

## Player-visible changes

- The default Full HD dock grows from approximately 604 to 888 pixels wide; buttons grow from about 55 to 81 pixels. Both rows remain available on keys 1-0 and Ctrl+1-0.
- Esc -> Action bar size offers 80-120% in ten-percent steps. It changes only the dock and its associated presentation. On a 4K viewport the physical growth is capped; windowed and ultrawide placement remains centered.
- Key plates separate labels from bright artwork. Existing original class icons, cooldown sweeps, queued-action and ready/chain cues are retained. A carried spell highlights its destination.
- Cast/opportunity indicators sit above the actual dock. The target windup meter retains its own space; world nameplates reserve the displayed dock/cast rectangles.
- Tooltips measure complete Russian/English prose before positioning the card. They no longer silently omit the final sentences after two lines.
- A drop between slots or on the dock frame keeps the original binding. Intentional removal into the world and right-click unbinding remain available. Changing size during a drag cancels it safely.

## Persistence

`HOTBARS 9` stores the dock percentage before the existing per-class rows. Versions 1-8 preserve every stored binding and adopt 100%. Invalid size, incomplete input and trailing data reject the entire candidate. The menu persists through the existing atomic compare-and-write helper, using the roster's `.hotbars` sidecar. It does not change the character save. Concurrent/external changes retain their contents and produce a visible menu error.

## Validation

RPGNative and the selected test targets build in Ninja Release and Visual Studio Debug. InterfaceTests, CombatHudTests, CombatHudWorldTests, SpellChainsTests and CharacterSelectionTests pass in both configurations (Release: 55.83 s; Debug: 151.00 s). After the final tooltip-padding adjustment, all three affected Release UI/HUD suites were rerun and passed in 32.46 s; the Debug run includes that final adjustment. The Russian coverage audit passes with 2,136 source occurrences and 1,985 catalog entries, and `git diff --check` passes. Native 100%/120% dock, full-description tooltip, settings/conflict and active-cast images were visually inspected. These are focused checks, not a claim that the entire project test suite was run.

The focused native checks exercise all twenty slots at 960x600, 1280x720, 1920x1080, 2560x1080, 2560x1440 and 3840x2160, at 80%, 100% and 120%. They cover cross-row movement, right-click removal, world removal, dock-gap release, resize/scale cancellation, minimap picking and unchanged simulation state. Additional integration checks drive the actual settings menu, restart from an isolated profile, retain bindings and reject an external write conflict without changing the hero file. Existing interface, spell-chain and character-selection checks cover keyboard routing, book assignment and the relocated return-to-roster button.

Native evidence is generated in the existing build directories: `combat-hud-review/action-size-{80,100,120}.png`, `action-tooltip-120.png`, `settings-conflict.png`, four-class cooldown feedback, cast captures and `combat-hud-world-review/`. No model, face, authored animation or source artwork is changed. No download, commit or push is part of this work. Full-speed subjective acceptance and complete reference correspondence remain open.
