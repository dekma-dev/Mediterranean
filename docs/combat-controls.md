# Combat and live interface — 2026-10-23

Follow-up: [mastery and response](mastery-and-response.md) adds eighteen talents per class, draft planning, attack buffering, new recovery silhouettes and readiness/cancellation feedback. The controls below remain applicable.

## Controls

Only the Esc game menu captures movement and combat controls. Character, inventory, atlas, journal, build, shop and quest windows keep keyboard movement live. Their visible rectangles still consume pointer input, preventing clicks or camera drags through buttons. Walking away from a conversation/shop closes it through the existing distance rules. Focus loss and character selection retain input suspension. Esc is not a world pause: enemies and cooldowns continue.

The bottom bar has ten larger slots, bound to **1–9, 0**. Default bindings are the four class actions followed by a potion; the remaining five slots start empty. This is ten assignable slots, not ten newly authored spells per class. Q remains a potion shortcut.

Drag between slots to swap their contents. Open **N** and drag a spell card onto a slot to assign it. Clicking a slot activates on release; dragging never casts. Unavailable/cooling-down spells can still be moved. Dropping outside a slot cancels. Layouts are independent per class and persist with F5 or Save & choose character in `saves/patrol.save.hotbars`; F9 reloads them along with the selected character. Concurrent writes use the existing conflict-safe storage and never silently replace another instance's settings.

## Class recovery and attack identity

| Class | Fourth action | Preparation | Main distinction |
| --- | --- | --- | --- |
| Warrior | Iron guard | Instant | Absorbs damage; absorbed hits earn Rage |
| Paladin | Prayer | 0.9 s | Healing and protection; heavy contact adds a small ward |
| Scout | Evasive stance | Instant | Evades one hit within three seconds; heavy shots snare |
| Priest | Greater heal | 1.5 s | Strong direct heal; retains offensive healing interactions |
| Druid | Regrowth | 1.2 s | Nature healing; heavy contact briefly roots |
| Mage | Frost ward | Instant | Barrier slows attackers; slower offensive preparation |
| Necromancer | Bone pact | Instant | Costs 10 health to form a barrier; retains life drain |
| Psionic | Inner focus | Instant | Fills Insight and grants a ward; heavy contact briefly stuns |
| Bard | Refrain | 1.0 s | Healing with existing rhythm/sustain mechanics |
| Engineer | Deploy plating | 0.8 s | Stationary barrier preparation; heavy contact exposes armour |
| Demonologist | Blood carapace | Instant | Costs 8 health; barrier and two-second Ravage cooldown reduction |

Only Paladin, Priest, Druid and Bard have direct healing in the fourth slot. Other classes retain intentional sustain passives where applicable, but their fourth action cannot heal. Defensive talents convert recovery power to barrier capacity; Scout can gain a small additional barrier through these talents. Mercy grants defensive classes additional low-health barrier capacity rather than healing.

Timed recovery requires grounded, stationary preparation. Movement, jumping, actual damage, Esc or focus loss interrupts it; healing/protection is applied only on completion. Starting a cast spends its cooldown, including interrupted casts. Other abilities and build changes cannot bypass an active recovery cast. Offensive class clocks drive both authored motion and contact timing; existing stationary-heavy interruption rules remain in force.

## Presentation

Buttons and slots ease their hover tint, show a pressed state, and use delayed tooltips. Spell dragging shows a cursor-following icon. Recovery has a progress bar and gathering hand/ground effects; impact effects add bounded class-coloured streaks. Warrior body preparation and follow-through now include restrained torso/hip/offhand motion; other class attacks use differentiated, generally slower playback rates.

UI clicks have a short muted wood/metal cue. Cast preparation and magical impacts use lower, softer synthesized resonances and airy transients; defensive actions use ward cues instead of universal healing audio. All sound remains locally synthesized: no downloaded recordings or extra middleware.

Reference principles: the official [Final Fantasy XIV Black Mage guide](https://na.finalfantasyxiv.com/jobguide/blackmage/) separates cast/recast timing, resource decisions and defensive actions. Black Desert's official [Warrior](https://www.console.playblackdesert.com/GameInfo/Class?classType=0) and [Sorceress](https://www.console.playblackdesert.com/GameInfo/Class?classType=8) descriptions emphasize different weapon identities and engagement ranges. These informed timing and class differentiation, not asset copying.

## Remaining acceptance

Automated checks verify timing, cancellation, real defensive hits, input ownership, drag assignment, serialization, audio bounds and rendered layouts. Manual checks 222–226 cover feel and listening. This pass does not establish parity with the reference games: spell icons still reuse the existing atlas, the hero still uses the current procedural rig, and four actions per class remain the content limit. Authored animation/art and listening review are still needed. GPU integration of imported skeletal assets remains a separate roadmap milestone.
