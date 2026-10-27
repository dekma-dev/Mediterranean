# Player-facing language coverage

Russian is the default. Esc > Language switches between Russian and English and
stores the choice in `language.ini` beside the executable. The launcher uses the
same preference the next time it starts. Each build directory has its own setting.

The catalog in `game/Russian.inc` covers the launcher, loading screens, character
creation/selection, HUD, inventory and equipment, map, journal, trade, settings,
all eleven class descriptions and their abilities/talents, and all twelve quests.
Quest offers, reminders, turn-ins and completion receipts have full authored
translations. Native Win32 startup/error captions use `platform/LocalizedText.h`.
Driver-provided diagnostic details and developer benchmark reports retain their
original technical terminology; they are not quest or interface prose.

English simulation/save identifiers are unchanged. Translation is performed at
the presentation boundary: exact full messages take priority over the fallback
for composed values. Candidate lookup is indexed by first byte, and the dynamic
text cache is bounded to 512 entries. Do not translate save keys or compare a
localized caption to decide gameplay behavior.

Character creation accepts Russian letters, including Ё/ё, through Windows text
input. The twenty-character limit counts UTF-8 codepoints; backspace removes a
whole character. Explicit nicknames are preserved, while built-in class titles
are localized. Old English-named saves remain compatible. The font atlas includes
Cyrillic, guillemets and the mathematical minus without increasing its 512x512 R8
texture allocation.

## Verification and future text changes

Run `python scripts/check-localization.py` after adding player text. This scans
game strings, launcher/Win32 paths, renderer errors and quest passages. It rejects
remaining Latin prose, duplicate catalog keys and unsupported untranslated
messages. `tests/localization-nonplayer.json` is an explicit list of technical
identifiers and developer diagnostics, not an exemption for whole interface files.
The audit is also registered as `LocalizationCoverage` when CMake finds Python.
Python is optional for building/running the native game.

`LocalizationTests` checks the full catalog, supported glyphs, composed values,
language persistence and complete class/quest descriptions. Character selection
tests cover Russian typing, codepoint deletion and save round trips. Interface,
launcher and creation tests render native captures at several sizes.

The source audit detects missing translations, not all grammatical or layout
problems. Inspect native screenshots as well. Full descriptions must not be
silently cut to the old English line count. The discipline inspector now measures
the complete paragraph; compact nodes show names/status and keep their detailed
effects in the inspector. Recheck long names in both languages when editing them.

This language milestone does not establish visual, animation, environment or audio
parity with the user's recordings. Those require separate reference acceptance.
