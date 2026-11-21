# Simultaneous quest tracking and activity feedback

## Visible change

The right-hand HUD now keeps the campaign and all six accepted provincial
commissions visible together. The routed objective has its full instructions
and a brass accent; the other rows show their actual counts or delivery state.
Ready rows use a gold question mark. A hunt that still requires its leader
explicitly says so, even when its ordinary counter has reached the threshold.

Click another row to follow it without opening a panel or stopping movement.
This removes an overriding manual map pin and switches the existing atlas,
minimap and walking-distance guidance to the selected assignment. The header
and detailed current objective still open J. Opening the inventory replaces
the list and its hit regions, as before. The tracker height and nameplate
exclusion follow the number of visible entries.

A bounded feed near the upper centre reports acceptance, objective increments,
readiness and actual delivery. Local progress uses a large concise counter;
ready notices name the speaker. Repeated updates of one assignment replace
its previous notice rather than flooding the screen. At most three notices
remain for five seconds, with a short reveal and a final fade. They do not
intercept clicks or movement. Panels and sparring hide the transient feed;
notices still age while hidden. The durable quest list retains current status.
Beside an interactable object, only the newest two notices are drawn so that
the E instruction remains clear. All three can be shown away from that prompt.

## Reference and implementation

Reviewed the existing extracted frame
`build-learning-review/pve2/frames/f000004.jpg`, from the supplied
`D:/assets/learning_material/pve_loc2.mp4`. It shows a right-side list of
simultaneous tasks and central sub-objective updates. This pass adapts that
information grouping to our actual campaign and commissions. It is not a
claim of complete video review or 1:1 interface parity.

`QuestTracking.h` reads existing simulation state without granting rewards,
advancing objectives or changing selection. Campaign text is now available
independently of the tracked commission. Delivery notices require the actual
completed ledger state; disappearance from a list alone is insufficient.
The completed campaign does not reappear as an unaccepted final chapter.
Rehearsals show only their own encounter and suppress campaign receipts.

The first observation after character/load/reset establishes a baseline and
does not replay old increments. Subsequent changes are matched by quest ID,
not by the selected row. Existing save format, quest rewards and combat rules
are unchanged. All new player-facing captions have Russian translations.
Models, faces, icons, class kits and audio assets are unchanged.

## Verification

`QuestTrackingTests` covers simultaneous objectives, switching selection,
untracked cache progress, real weapon contacts, the mandatory-leader rule,
readiness, actual delivery, campaign hand-off, completed campaigns,
rehearsal entry/exit, lifecycle baselines, coalescing, capacity and expiry.
Native tests dispatch a HUD click through `Game::Update`, check that all
commission rows are reachable, and verify inventory replacement and the
absence of movement capture. Cache/kill fixtures use diagnostic relocation;
creatures are weakened and stunned to isolate contact and quest accounting.
They are not difficulty or live traversal acceptance.

Native captures are under `quest-tracking-review` in each build output:
progress, selected objective, ready, loaded, inventory and batched notices
beside a speaker/in the open world, at 1280x720,
960x600 and 2560x1080. Progress at 1280, selected at 960 and readiness at
ultrawide were inspected; the resulting counter-readability correction is
included in the final sources.

Final build/test results are recorded in `progress.md`. Remaining manual
acceptance: follow several assignments through a live hostile route, check
notice readability during crowded combat, and test quest switching after a
manual map pin. Further quest-content expansion remains on the roadmap.
