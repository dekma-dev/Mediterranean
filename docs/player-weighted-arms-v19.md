# Loaded weapon arms — 2026-11-11

The editable source is `assets/authored/player-motion-v19.blend`. This pass
changes 21 weapon-arm actions: twelve directional runs, three jumps and six
sword/staff/bow attacks. The 57-bone rest skeleton and all non-arm curves in
all 51 Blender actions are unchanged. Every native record header and every
non-arm channel is preserved byte for byte against v18; 36 complete PCL5
records are also unchanged. Twenty-one frozen sources include v18, the
character/equipment sources and the original thirteen-model collection.
Faces, eyes, hair, scalp, materials and expression shading remain frozen.

## Source and playback changes

The previous carried hand retained an older Cartesian grip path while the
free arm used the revised shoulder/elbow trajectories. Running now authors
the loaded upper arm and elbow together, with different excursions for
sword, staff and bow. The weapon direction trails the arm cycle. A stable
handle frame replaces the source wrist-roll calculation based on a nearly
parallel forearm and shaft. Fingers retain a closed grip; the runtime grip
and two-handed staff/bow constraints remain active.

Jump arms have a compact bent-elbow arc, continuous orientation keys and
vertical compensation during landing compression. The native transition
into equipped flight lasts 300 ms instead of 220 ms; free-arm timing is
unchanged. A fully gravity-stabilized candidate increased transition spikes
and was rejected in favour of the body-relative, independently keyed arc.

Attack weapon curves are filtered before baking and use delayed follow-through
after contact. Shared staff grips are fitted into both source-arm reach
spheres before solving either hand. The overhead sword has a clearer raised
preparation. Native sword entrance is capped at 190 ms instead of 220 ms;
ordinary sword attacks and staff/bow entrance times retain their shorter
existing limits. A 120 ms candidate exceeded the existing weapon-speed bound
and was rejected. Presentation recovery is longer by 150 ms for light attacks
and 200 ms for heavy attacks. Authoritative contact/release, cooldowns, movement
speed, jump physics and save data are unaffected by these changes.

## Measurements and review scope

The matched 120 Hz native running replay reduces loaded-wrist acceleration
RMS from about 135.85 to 34.1–34.2 m/s² for all three equipment types. A new
regression measures motion relative to the shoulder, so the unchanged pelvis
response cannot dominate the arm check. It requires a genuine carrying arc,
a continuously bent loaded elbow, a stable palm attachment and bounded
relative acceleration. It does not permit freezing the hand to pass.

The diagonal sword and staff attacks also reduce weapon-tip acceleration in
the matched replay. The larger overhead preparation and revised jumps do not
improve every whole-character acceleration statistic. Their changed excursion
and preserved body-contact response are included in those measurements;
they must not be presented as a universal reduction in acceleration or as
human motion-capture validation. Existing 1 ms continuity, floor-clearance,
two-hand grip, interruption and simulation-isolation checks remain in force.

The integration run also exposed a stale `ClassEquipmentTests` UI fixture:
it clicked class-creation controls on the saved-character roster. The capture
now selects the exact saved profile and advances roster pages as necessary.
This changes only the test; it does not change character selection or gameplay.
The original failed run is retained in the local validation record, alongside
the targeted rerun.

Local review: `.blender-local/motion-v19-20261111/review.html`. It compares
twelve identical scenarios before/after, with sword, staff and bow runs,
jumps, light and heavy attacks, plus a game-world recording. Chronological
front/side/back frames are inspected for pose, attachment and silhouette;
the videos permit review at normal and reduced playback speed. The animation
is authored and baked, not mocap or a rigid-body weapon simulation.

Blender opens on `Sword_Run`; the assembled character mesh remains in the
unchanged `player-hero-study.blend`. Authoring and review scripts stay ignored
by Git. Detailed metrics, preservation checks and final validation are stored
locally in `.blender-local/motion-v19-20261111/validation-summary.json`.

## Validation results

All twelve selected animation/art suites pass in Release and Debug. The
initial Release run took 296.21 seconds and passed eleven suites; after the
roster-fixture correction, the isolated `ClassEquipmentTests` rerun passed
in 19.07 seconds. Debug passed all twelve in 442.03 seconds. Existing motion
limits were not relaxed. The new loaded-arm regression additionally checks
shoulder-relative wrist acceleration, carrying excursion, elbow flexion and
palm attachment for sword, staff and bow.

All six game outputs passed fixture-save smoke launches: Ninja Release/Debug,
Visual Studio Release/Debug, and the frontier native/debug outputs. Resource
extraction confirms that each executable embeds the exact current PCL5 motion,
KIT1 equipment and GWT2 garment archives. Body data and face/scalp shader blocks
remain unchanged. The local review contains 25 verified 1280×720, 30 fps videos:
twelve matched before/after pairs and one game-world recording. Final contact
sheets and selected native/game captures were visually inspected; this is
frame-based review, not a claim of motion-capture or real-time visual validation.
