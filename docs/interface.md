# Fullscreen and interface

Updated: 2026-10-19

## Combat readability

Merchant bag potion tooltips follow the same health/cooldown rules as the bag action button and right-click use. Trading captures movement and disables the action bar; it does not prohibit drinking from the bag.

The target frame shows numeric health and distance, with a countdown and filling amber strip during an enemy windup. Action slots show a draining cooldown mask, a recovery strip and tenths during the last second; positive remaining time rounds up so it never displays zero before becoming ready. Green/rust accents distinguish ready/unavailable slots, and hover cards provide textual status as well as cooldown duration and melee range or potion stock. Potion quantity is also visible on the slot. The objective tracker has a dark backing for contrast.

Indicators derive directly from current simulation state and do not intercept input or change action hitboxes. Three `combat-hud-*.png` diagnostics in each build directory cover 960x600, 1280x720 and 2560x1080, with disabled-click and read-only-state checks. Manual check 102 covers live readability.

## Display and menu behavior

Normal gameplay starts in borderless fullscreen on the window's monitor. F11, or the Display button in the Escape menu, switches between fullscreen and the previous window placement. The app does not change the monitor resolution, desktop scaling, taskbar settings or exclusive display mode. Per-monitor DPI awareness is process-local. Hidden tests remain hidden and restore their original client size after testing the fullscreen transition.

Escape always opens the game menu; pressing it again returns to the game. From another panel, Escape opens the menu. It never pauses simulation. The menu has Return, Character, Inventory, Atlas, Journal, Display, Save, Load and Exit buttons. Save/load results appear in the menu. Exit closes the game without an automatic save; F5 or Save explicitly writes progress.

Character and inventory are independent side windows. Press C then I to keep both open; each X or hotkey closes only its own window. Keyboard movement and combat remain available. Mouse orbit/zoom is captured over side windows and interactive HUD controls and remains available over the world. Camera orbit must begin with a fresh right-button press over the world: a gesture started in the bag or minimap cannot become an orbit by crossing its edge. During an accepted camera gesture, the cursor is hidden and pinned at its initial screen position. Foreground raw mouse deltas keep orbit working even at screen edges. RMB can now tilt upward to inspect the sky and sun; the camera retracts above the flat ground. RMB also turns the champion left/right with the view while standing still; strafing/backward travel retains that heading. Accepted attacks and active dashes keep their committed heading until recovery. Releasing RMB shows the cursor at that position and frees it. Focus/capture loss, resize, DPI/fullscreen changes, menus, load and shutdown cancel the lock; continuing to hold RMB cannot restart a cancelled gesture. Opening a side window over the pinned pointer also cancels orbit. Right-clicking items or the atlas keeps the pointer visible and free. Selecting worn gear opens its item in the bag without closing the character window.

Menu, atlas, journal and the quartermaster capture movement/camera/combat commands. Escape opens the menu over the side windows; closing it restores them. Trading itself closes on Escape. Enemies, committed attacks, cooldowns and other simulation time continue in every layout. Focus loss/minimization retains the existing single-player suspension behavior; it is independent of Escape.

The window owns transient cursor capture and raw mouse registration. It uses a one-pixel [ClipCursor](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-clipcursor) rectangle and `SetCursor(nullptr)`, releases capture before relinquishing focus, and preserves ordinary button messages for UI actions. [WM_INPUT](https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-input) supplies camera deltas; absolute devices establish a baseline before producing movement. No desktop cursor settings are changed. Added regressions are pending execution because the native build toolchain is unavailable in the current environment; see progress.md and manual check 119.

## Controls and screens

| Input | Behavior |
| --- | --- |
| Escape | Open/close the live game menu |
| F11 | Toggle borderless fullscreen/windowed |
| C | Character and equipment |
| I or B | Inventory |
| M | Atlas |
| J | Journal |
| Left click | Buttons, item slots, action-bar abilities and map waypoint |
| Drag item | Swap bag cells, equip into a compatible slot, or remove into the bag |
| Right-click item | Equip, unequip or drink |
| Sort button | Compact and order the bag, merging consumable stacks |
| Split half button | Move half the selected potion stack into the first free cell |
| Sell one button in shop | Sell one from the selected bag cell at the displayed price |
| Buyback item | Reclaim one into the bag at its sale price |
| E near the quartermaster / E in the shop | Browse / close trade |
| Character arrows / Front | Rotate the dressed preview / reset its facing |
| Right click on atlas | Clear waypoint |
| X button / same panel hotkey | Close the current panel |
| F2 / F3 | Hide HUD / show renderer metrics |

The supplied Allods screenshot is the primary layout reference for the current pass: carved green/gold frames, raised title plaques, serif labels, a character alcove and adjacent attributes on the left, a compact bag on the right, a circular minimap, and a bottom combat dock. Both side windows leave the middle of the world visible without dimming it. The bag replaces the objective tracker while open.

The HUD has player health/XP, target health and windup state, a north-up circular minimap, objective tracker, context action prompt, five functional action slots, gold and panel shortcuts. Four additional engraved action cells are visibly empty and unbound. Hovering an action shows its effect and availability. Interface scaling uses a centered 1280x720 design canvas, maintaining proportions on standard and ultrawide displays.

### Inventory

The compact six-column bag displays 24 actual storage cells. Worn items occupy separate weapon/chest/legs/feet/head/off-hand/shoulders/hands/waist/back slots and are not duplicated in the bag. Item details compare weapon damage or damage reduction against currently worn gear. Left-click selects an item; right-click equips/removes gear or drinks a draught. The detail button offers the same action. Potion ownership, death, full health and cooldown rules remain simulation-validated.

Drag between bag cells to move or swap; matching consumables merge up to 20 per stack. Drag gear onto its compatible character slot to equip it and return the previous item to the source cell. Drag worn gear into the bag to remove it or swap with compatible gear in the destination. Sort compacts cells and merges stacks. Incompatible drops change nothing; commands include the expected source item to reject stale drags after keyboard equipment changes. Closing a window, Escape, focus loss, minimization or resizing cancels the gesture.

The selected bag cell is tracked separately from its item type. Split half moves the floor of half its potion count into the first empty cell; at least two potions and an empty cell are required. Drinking or selling uses that exact selected stack. Q still uses the first available stack. Dragging matching stacks together or Sort merges them. Stale source cells reject without consuming another stack or granting healing.

Ward armor automatically equips on reward, placing the tunic in the bag; both remain freely interchangeable afterward. Removing gear updates combat attributes immediately. Version 7 saves preserve exact cells, split stacks, ten worn slots, purchases and thirteen item types in buyback; versions 1–6 migrate without losing existing rewards. Older saves receive the four new starting travel pieces once. The catalog adds shoulder guards, gloves, a belt with pouches and a cloak to the existing swords, potion and clothing/armor. Permanent disposal and movable windows remain future work.

### Character

The character screen places ten working equipment slots in two columns of five around a lit 3D preview. The preview and world share `CharacterView` and the actual worn outfit. The watchguard blade has a brass/gold finish; ward armor has a chest crest and edged shoulder plates. The bought helm adds steel head protection and cheek guards; the shield has a blue face, brass rim and metal boss. Starting travel gear adds riveted leather shoulders, fitted gloves, a removable belt with pouches and a blue cloak with a brass clasp and gait-driven hem. These pieces have no armor bonus. The cloak is procedural overlapping cloth geometry, not a cloth simulation. Removing equipment removes its visual; neutral underclothes remain under removed clothing. Arrows rotate the preview and Front resets it. Filled slots open details in the bag. Attributes derive from worn gear: starting clothes add no protection, ward adds 2, helm 1, shield 2. The art remains procedural and is not a finished character model.

The shared model now has a shaped jaw/face, fitted hair, flatter boots, closer-fitting trim and fixed-length arm/leg poses. The resting sword and shield attach to the hands in both views; walking and strikes use the same pose solver in the world. This adds no interface controls or equipment rules. See [graphics.md](graphics.md) for close-up validation, resource changes and the remaining animation limits.

### Quartermaster

An orange marker identifies the village quartermaster at `(3, -12)`, beside an original wooden supply counter. E opens a central green/gold shop with the bag already open; C adds the character window without closing trade. Each row has an item icon, stock, price, effect and Buy button. The displayed availability comes from the same simulation rules as the purchase command. Sold-out, locked or unaffordable rows cannot be purchased.

The buyback shelf wraps thirteen item types over two rows. The shop stocks six healing draughts (6 gold each), one helm (20) and one shield (35). Equipment unlocks after the patrol hand-in. Purchases update gold, item ownership and remaining stock together, put items in the bag and report the result. Drag or right-click the bought gear to equip it while the shop stays open. Stock survives loading and respawn. E or X closes trade. Death or loss of trading eligibility also closes it. The world remains live, but underlying HUD attacks and movement keys are captured.

Select a bag cell and click Sell one; the button displays its price. Worn items show Unequip to sell and cannot be sold directly. Right-click remains equip/remove/drink. Nine buyback cells below the offers show the merchant-held item types, counts and reclaim prices. Hover for the name and eligibility; click to reclaim one into the bag. Unaffordable items stay visible but cannot be reclaimed. All items remain held across leaving, saving/loading and respawn; only a new expedition clears them. Buyback groups identical potions and has no expiry or discarded history. Reclaiming costs exactly the sale price, while fresh purchases cost more. Selling never replenishes retail stock. Prices are listed in [gameplay rules](gameplay.md).

### Atlas

The atlas and minimap share a generated 512-square hand-drawn survey: parchment grain and muted washes, sepia coastline contours and cliff hachures, outlined roads, sketched roofs and branchwork, ruins, supply stall and dock planks. The coastline comes from the rendered island mesh, roads from SurfaceAt and landmarks from WorldMap. The full sheet adds paper margins, dark ink lettering, marginal leader lines, an eight-point compass, named areas, a five-metre scale and a legend. Live markers and waypoint clicks retain the same undistorted world coordinates. Player facing, living raiders, unclaimed loot, keeper, anchor and current objective are live overlays. Orange marks the quartermaster; cyan marks the eastern sparring post, unlocked after both quests. A live practice card shows the remaining time, damage dealt/taken and resource rules; the round result remains visible for eight seconds.

Click traversable ground to place a waypoint; right click or Clear removes it. Invalid ground does not replace an existing valid waypoint. Waypoints are session UI state and do not move the character or alter saves. The illustration depicts this small flat test island; it is not a large authored world atlas or a claim that additional terrain exists.

The illustrated plaza, village street and dirt routes follow the same surface data as the ground shader. The dock footprint clears the neighboring tree/building. Source map/font texture sizes are unchanged. The new drawn-map treatment has not been built or visually accepted in this environment; the existing scaled InterfaceTests now request atlas-drawn captures for review. Manual check 120 covers appearance and interaction alignment.

The atlas uses muted roof washes keyed to each building's role and marks the striped supply canopy. The blue-gray watch hall is distinguishable from the warm homestead and olive storehouse. The gold keeper marker now floats above a dressed NPC at the existing quest location; the quest commands and rewards remain the same.

### Journal

Both quest cards show their current completion/unlock state, objectives and actual rewards. A button opens the atlas. Reading the journal leaves combat active.

## Art and rendering

Original UI shapes/icons, embossed rails, leaf inlays, curtains and gradients are authored in C++; no reference screenshots or downloaded art are included. The installed Windows Georgia font is rasterized into a 512x256 R8 atlas at startup. No font file is copied or modified. The map is generated once and uploaded with mipmaps. The circular minimap reuses that texture through a bounded triangle fan, with aspect-correct UVs. Font and map textures together still use 1,529,172 bytes (about 1.46 MiB), additional to the existing world material/shadow allocation. These are texture allocations, not total process or VRAM use.

The interface uses alpha-blended batched triangles, with a small dark text shadow for contrast. A character preview flushes preceding UI, renders the shared model in a bounded viewport with studio lighting, then restores the game camera and viewport. It does not allocate a second large scene target or update the simulation.

## Reference research

Research used web search/indexed excerpts and image-search references. Direct article/image fetches were unavailable for some sources, so this is not a claim to have inspected every original page or to reproduce a specific live-game version.

- [Allods Online quick-start guide](https://allods.my.games/en/game/game-guide/quick-start): interface/navigation and game-menu reference.
- [Allods character and inventory screenshot in its forum](https://allods.my.games/forum/index.php?page=Thread&postID=74926): character information alongside an item grid and tooltips.
- [Allods character equipment reference](https://alloder.pro/main/guides/sistema-odevaniya-v-krae-mira-r1055/): dressed character, equipment slots and adjacent attributes, used as a visual reference.
- [Blizzard's UI/HUD overview](https://worldofwarcraft.blizzard.com/en-us/news/23837944/get-into-the-grid-of-things-with-the-updated-ui-and-hud): readable HUD organization and combined backpack presentation.
- [Blizzard's map/quest interface preview](https://worldofwarcraft.blizzard.com/en-gb/news/15707238/patch-602-preview-user-interface): connecting map and quest information while reducing clutter.
- [Allods basics guide](https://allods.my.games/en/game/game-guide/basics): merchant and NPC roles, checked through indexed excerpts for this village pass.
- [Blizzard's Stormwind tour](https://news.blizzard.com/en-us/article/20142731/welcome-to-stormwind-a-guided-tour): equipment and supply vendor roles.
- [World of Warcraft classic manual](https://us.media.blizzard.com/manuals/wow/wow-classic-manual-enUS.pdf): equipment contributing to offensive/defensive attributes; consulted through indexed excerpts. Prices, stock, protection values and quest restrictions here are original prototype rules.

The user's supplied Allods character/bag screenshot subsequently clarified the desired visual direction and window composition. The current green/gold skin follows that reference's arrangement with original code-drawn decoration. This is a functional prototype adaptation; it does not reproduce Allods' finished illustrations, character art or full item catalog. No game assets were downloaded or copied.

## Validation

`InterfaceTests` checks Escape transitions, simultaneous character/bag state, independent closing, return from the menu, live damage during all five panels, ownership/use rules, equipment changes and save round trips. A hidden DirectX window checks item, close, action-bar, menu and map click routing at 960x600, 1280x720 and 2560x1080. It also drags items between bag cells and worn slots, rejects incompatible drops, removes boots by right-click, sorts the bag and cancels a drag through Escape. Side-window surfaces capture the pointer while the world gap and available action-bar abilities remain usable. Central overlays reject clicks aimed at the underlying HUD. DirectX debug warnings/errors are rejected when that optional runtime is available.

Trading coverage also drives sell/buyback and odd-stack splitting through sampled Game input. Scaled pointer tests select the second potion stack, split, sell one, reclaim into the first stack and drink from the second, checking each exact cell. Worn sales and empty shelf clicks are disabled. Domain tests validate all catalog sale/reclaim cycles, transactional denial, balanced gold, saved buyback and v4–v6 migration. These checks exercise the production input/layout/command code without synthesizing real desktop input.

`-SmokeTest` validates hidden fullscreen size/restoration and renders spawn, raider, captain and veteran scenes. Each scenario captures the world at three sizes, two inspection views, and Menu/Inventory/Equipment/Atlas/Journal plus the combined Equipment+Bag layout at 960x600, 1280x720 and 1600x900. The veteran adds base/travel outfits, sparring with/without side windows, shop, purchased equipment and buyback at three sizes, a dressed world close-up, fully equipped sparring, eight close-up poses, seven environment views, six animation layers and four live jump stages: 132 smoke captures per configuration. Suffixes include `-quartermaster`, `-purchased-equipment`, `-buyback`, `-watchman-character`, `-equipped-sparring` and `-pose-*`. InterfaceTests separately writes `inventory-drag.png`. Representative captures were visually inspected. Debug and Release pass all twelve CTest suites.

Merchant integration checks route sampled E/click inputs through Game, buy actual gear, block walking while shopping and allow it after E closes trade. Scaled pointer checks buy/equip both new items with all three windows open, reject sold-out buttons and underlying attacks, and route portrait rotation. TradeTests verifies persistent charges/stock, unavailable purchases, armor in real combat and save migration.

Hit regions are invalidated when panels change, a snapshot loads or HUD visibility changes, and are checked against the current panel state and rendered size. Input cannot activate an invisible previous-panel button before the next render. Additional regression tests cover opening/closing the menu while clicking, old gear regions after load, resize during a drag, and right-button gestures crossing from the bag/minimap into the world.

The 2026-10-18 input fix cancels gestures and invalidates hit regions before processing F11's frame. Releasing a dragged item together with F11 cannot commit a drop before the window transition; a fresh drag afterward works normally. On defeat, the current central panel closes and gestures cancel so the recovery prompt becomes visible. Side windows remain open. Enter recovers the defeated character even if the menu, atlas or journal is reopened. Regression tests cover both behaviors, and `InterfaceTests` writes a separate `defeat-recovery.png` for visual inspection.

Real keyboard/mouse interaction, Alt-Tab, multiple physical monitors, display scaling changes and longer gameplay sessions still need the manual checklist. Automated pointer tests call the same interface routing with coordinates. Game integration tests feed sampled input frames through the same transition logic used by Win32 polling, including focus loss/restoration; they do not synthesize desktop input.

The historical quartermaster-pass stationary benchmark reported 26 draw submissions and CPU/GPU medians of 0.293/0.179 ms. The current `-BenchmarkSuite` also measures the merchant with character and bag windows, plus walking/orbit and combat/orbit. After the character pass, the merchant workload reports 40 peak submissions, 134,006 peak submitted triangles, CPU median/p95 0.780/0.947 ms and GPU median/p95 0.352/0.368 ms. This uses a hidden 1280x720 window, 90 warmup and 600 measured frames, with 600 asynchronous GPU observations. See [graphics.md](graphics.md) for before/after data and limits. These measurements do not guarantee visible/fullscreen FPS. Re-run `-Backend Ninja -Configuration Release -BenchmarkSuite` for current local results.

## Item comparison hover cards

Bag and equipment cells show a 309x246 reference-pixel card with icon, item type, level-adjusted damage/protection, comparison against worn gear, description, count/value and contextual action. Positive differences are green, negative differences red, with explicit numeric signs as well as color. Shop Buy buttons expose the same pre-purchase comparison and availability. Cards clamp inside the 1280x720 reference canvas and appear to the opposite side of the pointer; they create no interactive hit region. Existing reverse hit order and drag suppression remain authoritative. Six tooltip diagnostics cover 960x600, 1280x720 and 2560x1080. Manual readability/input acceptance is check 91.

The expanded wearable slots follow the body-slot convention described in the [official WoW Classic manual](https://us.media.blizzard.com/manuals/wow/wow-classic-manual-enUS.pdf), with the dressed-character emphasis of the [official Allods game guide](https://allods.my.games/en/game/game-guide/basics). [Blizzard's movement guide](https://worldofwarcraft.blizzard.com/en-us/news/20151231) also documents held-RMB mouse camera control. The items, meshes, travel-gear balance and control implementation here are original prototype work; no reference assets were downloaded.
