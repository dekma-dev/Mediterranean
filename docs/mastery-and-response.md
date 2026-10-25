# Mastery and combat response — 2026-10-23

This pass advances the existing RPG, rather than replacing its engine or artwork. It addresses build depth, combat input, character action silhouettes, spell feedback and front-end response. It does not complete the whole production roadmap.

## Reference review

Re-read the project's [development research](rpg-development-research.md), roadmap, visual comparison and antique UI review. Revisited the previously extracted local recording contact sheets `build-video-review/pass1-e-00.jpg` (02:14–02:33 combat) and `pass3-approach-00.jpg` (01:17.500–01:17.983 approach, consecutive frames). This is a focused review of existing extractions, not a claim to have rewatched both entire videos.

The useful reference traits are continuous preparation/release/recovery, effects that identify an action and its status, compact functional controls, and build decisions that change play. Preserve the antique frame artwork; improve behavior behind it. The official [Riot gameplay-clarity article](https://www.riotgames.com/en/news/valorant-shaders-and-gameplay-clarity) also supports keeping gameplay information readable and bounding effect cost. We use that principle without adopting its engine, lighting design or assets.

## Eighteen talents per class

The existing nine foundation/discipline nodes retain their durable IDs and behavior. Nine advanced nodes extend the three paths at levels 5, 7 and 9. Each class has its own names. The level-ten budget remains five points, earned at levels 2, 3, 5, 7 and 9, so specialization still requires choices.

| Path | Level 5 | Level 7 | Level 9 capstone |
| --- | --- | --- | --- |
| Offensive | Basic cooldown reduced by 0.15 s | Heavy cooldown reduced by 0.8 s | Empowered heavy contact deals 20% more damage |
| Defensive | 30 additional maximum health | Two additional damage reduction | Successfully completed mobility grants an 18-point ward |
| Recovery | 20% cast speed and eight recovery power | Another 25% cast speed and one second less recovery cooldown | Half of excess direct healing becomes a ward; non-healers instead gain 12 ward capacity |

Cast-speed bonuses add before duration is divided: a 1.5-second cast with both nodes takes `1.5 / 1.45` seconds. They affect timed recovery, not offensive attack animation speed. Recovery power becomes healing for healing classes and half as much barrier capacity for defensive classes. Instant actions remain instant. Existing armor, health, class resource, healing and sacrifice rules remain authoritative.

The advanced chains each depend on the corresponding foundation node. Removing a foundation in a draft removes every dependent node. Capstone behavior is tested against real damage, movement completion/cancellation and healing overflow. Wards use maximum capacity replacement; repeatedly applying them does not stack capacity indefinitely. Overhealing still requires an accepted heal on an injured character.

## Custom build planning

1. Open **N**, then **Plan a build**. This copies the active build into a temporary draft.
2. Inspect nodes on **Foundations** or **Mastery**. Use **Add to draft** or **Remove from draft**. **Clear draft** changes only the plan.
3. Use **Review draft** to compare health, damage, defense, cooldowns and recovery preparation.
4. **Apply preview** or **Store in loadout** still requires confirmation. Application checks earned levels, point budget and safe gameplay state. Planning itself may inspect level-ten possibilities without granting them.
5. **Edit preview on talent tree** edits a preset or stored plan. **F5** saves the active build and stored loadouts. A temporary unreviewed draft is not saved.

Applying a build cannot restore health or reset existing cooldowns. The planner is nonmodal: moving and combat remain live, but the simulation rejects unsafe application. Save format 14 accepts prior versions 1–13, preserves the first nine talent bits and adds the advanced IDs. Versions below 14 reject advanced bits even in loadouts. Older game binaries cannot load newly saved version-14 profiles; use the rebuilt executable.

## Combat input and presentation

Basic/heavy inputs may buffer during the last **220 ms** of cooldown/recovery. The queue holds one action for at most 250 ms, commits only after the preceding action is finished, and revalidates the target/range/line of sight. New input replaces the pending action. Changing target, Esc/focus suspension, death or loading cancels it. No damage, cooldown or resource transaction occurs merely because an action was queued. The action bar marks the pending action **NEXT**; mouse clicks and physical number keys use the same route.

The spellbook now displays actual assigned keys and dynamic cooldowns. Recovery preparation shows its real duration and remaining time rather than the old universal “instant” label. Defensive classes use shield/evasion icon motifs instead of a healing hand. The resource display no longer assumes Heavy is still assigned to key 2.

Recovery gestures now distinguish protective bracing, Scout preparation, Engineer assembly, Psionic concentration and sacrifice from prayer. They retain the existing fixed-limb solve and event timing. Warrior/Engineer live wards face the actor as protective contours; Scout's one-hit evasion has a short-lived visual state. Other class spell motifs and hostile warnings remain intact.

A newly full reserve has one restrained readiness cue. Cancelled recovery and interrupted magical heavy preparation have a short fizzle rather than a completion sound. These remain synthesized sounds; numerical/headroom validation is not a listening-quality approval.

## Launcher and character hall

Launcher buttons ease their hover/focus highlight and redraw only the affected controls while visible. Character selection adds eased highlights, pressed-button displacement, a brief right-panel transition and sparse warm dust in the hall. The class page describes the expanded talent paths. Existing native keyboard navigation, creation safeguards, DPI handling and saved-profile ownership remain in place.

## Verification and remaining work

Final validation (2026-10-23): all 43 Release CTest suites pass (83.57 s). Ten focused Debug suites pass (62.27 s), followed by a final incremental Debug rebuild and four affected suites passing (22.17 s). Visual Studio Debug builds successfully and its hidden startup/render smoke test passes, including shaders, resize and fullscreen restoration. Build, character-creation and launcher captures were inspected. Logs are `mastery-final-tests.log` in both Ninja build directories and `build-local/mastery-smoke.png.log`.

The hidden 1920x1080 Release workload suite on the RTX 4060 completes with CPU-frame p95 of 1.108 ms (spawn), 3.627 ms (walking), 4.262 ms (combat) and 1.246 ms (merchant/UI). No measured CPU frame exceeds 16.667 ms; ending working set is 316.8 MiB. These are short synthetic measurements, not interactive FPS or a before/after improvement claim. GPU sample counts are too small for reliable tail conclusions. Full report: `build-ninja-release/mastery-benchmark.txt`.

Relevant suites cover all eleven class paths, capstone contacts, preview agreement, point/prerequisite rejection, atomic save migration, custom-plan confirmation, queued-input timing at 30/60/144/1000 Hz, target/menu cancellation, audio one-shots, fixed limb lengths, effect budgets and small/normal/ultrawide rendering.

The imported-hero/GPU integration, authored anatomy and clothing, expanded active spell library, terrain elevation and complete movement clip library are still open. There are still four active class actions and a potion on ten assignable slots. Eighteen talent choices are not eighteen distinct spells. The current procedural hero is visibly less developed than the hall artwork; this update does not claim reference-game visual parity. Subjective motion, UI transitions and sound still require live acceptance using manual checks 227–231.
