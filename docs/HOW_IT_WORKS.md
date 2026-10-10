# How the CPU Partner mod works

This is a technical breakdown of the mod: how new code is put into an existing cartridge
image, what in the original game is changed, how the AI makes its decisions, which game
mechanics it relies on, and the bugs found along the way.

All addresses are for **Streets of Rage 2 (USA)**. ROM addresses are written `$xxxxxx`;
RAM addresses are written as the game uses them, `$FFxxxx` (the 68000 sees work RAM at
`$FF0000–$FFFFFF`; short addressing `$Fxxx.w` means `$FFFxxx`).

---

## 1. Architecture

The original ROM is 2 MB and its last 32 KB (`$1F8000–$1FFFFF`) are unused (`$FF` filler).
All new code lives there:

| Range | Contents |
|---|---|
| `$1F8000` | **Glue** (`cpu_partner.asm`): a table of fixed 6-byte `jmp` entry points, then the hooks, menu code, filters and bug fixes. |
| `$1F9000` | **AI** (`ai.c`, compiled by vbcc, plus `scan.asm`): a small table of entry points (`ai_think`, `ai_reset`, `ai_free_special`), then the compiled C. |

The original code is changed only at the **patch sites** listed in [section 3](#3-patches-to-the-original-code).
Each site replaces a few original instructions with a `jsr` into the glue; the glue
re-executes the displaced instructions and then does its extra work. `build.py` checks
every site against the original bytes before patching, so it refuses to patch the wrong ROM.
At the end it recomputes the header checksum at `$18E`.

### Build pipeline (`src/build.py`)
1. `vbccm68k -c99 -O=1 -cpu=68000` compiles `ai.c` to vasm assembly.
2. A tiny host program, compiled with gcc from `host.c`, prints the offsets of the
   AI's memory struct into `mem.inc`, so the hand-written assembly in `scan.asm` uses
   exactly the same layout as the C code.
3. `vasmm68k_mot` assembles the C output plus `scan.asm` at `$1F9000`, and `cpu_partner.asm`
   at `$1F8000`.
4. The patches are applied. A `jsr` to a glue table entry is shortened to a `jsr`
   straight to the routine.

### Rules the C code follows
- **No writable globals.** All persistent state is in one struct at a fixed RAM address
  (`struct Mem` at `$FFFE20`, 84 bytes), so nothing depends on a C runtime or data section.
- **Its own stack.** The AI runs on a private stack below `$FFE7A0`, RAM the game never
  touches, so the game's stack is not deepened. It uses about 360 bytes.
- **Version check.** `struct Mem` starts with a magic byte. A save state made with another
  build has a different value there, and the AI then clears its memory instead of trusting it.
- **Pointer checks.** Cached object pointers are checked to be real enemy or item slot
  addresses before use.

---

## 2. Memory map

### Game objects (work RAM)
| Address | Object | Stride |
|---|---|---|
| `$FFEF00` | Player 1 | — |
| `$FFF000` | Player 2 (the CPU) | — |
| `$FFF100–$FFF6FF` | 6 enemy slots | `$100` |
| `$FFF700–$FFF9FF` | 6 item/object slots (pickups, weapons, containers, projectiles) | `$80` |

### Object fields used by the AI
| Offset | Meaning |
|---|---|
| `+$00` | state (word) |
| `+$02` | x position |
| `+$06` | y position (includes height: depth = `y − z`) |
| `+$0C` | type |
| `+$0F` | bit 0: facing right (upper bits: animation) |
| `+$1E` | bit 5 / bit 6: sprite tiles / HUD icon need uploading (read by the VBlank handler) |
| `+$32` | items: flags (bit 7 can be picked up, bit 6 container, bit 0 held). Players: bit 0 holding a weapon |
| `+$34` | players: pointer to the weapon held |
| `+$3A` | prompts: cursor position |
| `+$49` | bit 7: can be hit |
| `+$50` | invulnerability timer |
| `+$5E` | height (negative = in the air) |
| `+$80` | health (players start with 104) |
| `+$83` | lives (byte) |
| `+$A4` | continues |

### Player states (`+$00`)
| Value | State | Value | State |
|---|---|---|---|
| 2 | standing / walking | 36 | special move |
| 4 | hit | 40 | forward special |
| 10 | knocked down | 44 | picking something up |
| 12 | grabbed | 46 | throwing a weapon |
| 20 | attack combo / back attack | 48 | weapon attack |
| 24 | jump (and jump attacks) | 50 | dropping in after losing a life |
| `$34`/`$36` | character select | `$38` | CONTINUE / GAME OVER prompt |
| `$3A` | game over | `$3C` | high-score name entry |

### Item types
| Type | Item | Type | Item |
|---|---|---|---|
| 60 | knife | 140 | money bag |
| 62 | pipe | 142 | gold bar |
| 66 | sword | 144 | 1UP |
| 68 | kunai | 146 | apple |
| 76, 96, 102, 104, 110 | containers (arcade machine, barrels, …) | 148 | turkey |

A weapon on the floor has state 8, flags `$80` and height 0. A container is state 2 with
flag bit 6 set; one hit breaks it and its slot turns into whatever was inside.

### Globals
| Address | Meaning |
|---|---|
| `$FFFC08`/`$FC0A` | P1 / P2 pad: held buttons; `$FC09`/`$FC0B` newly pressed (bits `SACBRLDU`) |
| `$FFFC10` | main-loop "frame finished" flags (non-zero while the main loop waits for VBlank) |
| `$FFFC11` | VBlank counter |
| `$FFFC22` / `$FC26` | camera x (negated) / camera y |
| `$FFFC42` / `$FC44` | stage × 2 / area × 2 |
| `$FFFC4B` | VBlank in progress |
| `$FFFD04` | difficulty (0–10 in steps of 2; 10 = MANIA) |
| `$FFFD06` | starting lives − 1 |
| `$FFFD0E` | starting stage |
| `$FFFD11` | "same character allowed" (the game's own cheat flag) |

### RAM used by the mod (`$FFFE00–$FFFEFF` and the AI stack; the game never touches either)
| Address | Meaning |
|---|---|
| `$FFFE10` | `AIFLAG`: 1 = CPU PARTNER mode (2 = player filters only, for testing) |
| `$FFFE11` | previous AI pad byte (to generate "newly pressed" bits) |
| `$FFFE12` | character select: P1 locked in last frame |
| `$FFFE13` | starting stage picked on the main menu |
| `$FFFE14` | game stack pointer saved while the AI runs |
| `$FFFE18`/`$19` | VBlank count at the last AI run / frames still to skip |
| `$FFFE1A` | VDP H/V counter when the main loop finished the last frame |
| `$FFFE1C` | VBlank fix: the main loop was waiting at this VBlank |
| `$FFFE20–$FE73` | `struct Mem`, the AI's memory |
| `$FFE638–$FFE79F` | AI private stack |

---

## 3. Patches to the original code

| Site | Original | Now |
|---|---|---|
| `$0007CC` | `move.w $FC08,$FC6E` (per-frame input copy) | **AiHook**: runs the AI and writes Player 2's pad |
| `$001A40` | `move.b #3,$FC10` (main loop finished) | **FrameDone**: also records the VDP beam position for the AI's time budget |
| `$006768` | draw OPTIONS | **MenuDraw**: also draws `CPU PARTNER` and `STAGE n` |
| `$0067E2` | main-menu up/down | **MenuNav**: 5 entries, skips 2P/DUEL without a second pad, LEFT/RIGHT stage |
| `$006726` | menu reset when no P2 pad | **MenuFix** |
| `$006890` | menu start | **MenuStart**: sets `AIFLAG`, starting stage, difficulty, same-character flag |
| `$006A02` | P2 character select input | **SelectP2**: after P1 locks in, P1's pad drives the P2 cursor |
| `$00B784` | continue: "bump character if same as partner" | **ContSame**: skipped when same character is allowed |
| `$004CD2`, `$0052BC`, `$004BA6`, `$005162` | hit / grab / thrown-object / thrown-body collision loops | **FfHit / FfGrab / FfThrown / FfBody**: skip player-vs-player in CPU PARTNER mode |
| `$00393E`, `$0038BC` | camera x / y: "wait for both players" | **CamX / CamY**: the CPU alone can push the screen |
| `$00F95E`, `$00F98A` | options LEVEL left/right | **OptUp / OptDown**: always MANIA |
| `$00F896`, `$00F8CA` | options cursor up/down | **OptNavUp / OptNavDown**: skip the removed PLAYERS row |
| `$00FCF2` | draw PLAYERS row | branch over it; the rows below are moved up one line (VDP addresses at `$FE50–$FE9B`) |
| `$005C8A` | title: draw `MUSIC © YUZO KOSHIRO` | **TitleText**: also draws the credit line (copyright lines moved up a row: `$5D0A`, `$5D1A`) |
| `$0000BC` + 8 special-move routines (`$AED6 … $B3DC`) | `subq.w #8,$80(a2)` / `rts` | `trap #15` / `nop` / `rts`; TRAP #15 → **SpecTrap**: no health cost for the CPU |
| `$0004AA`, `$0004B2`, `$000554`, `$0005D4`, `$0006BE`, `$000730`, `$000762` | VBlank handler | **VbEntry / VbGate***: no VDP work while the main loop is busy ([section 7](#7-the-vblank-race-background-corruption)) |
| `$00040C` | `move.w #4,$FD04` | difficulty 10 (MANIA) |
| `$000412` | `move.w #2,$FD06` | 8 (9 lives) |
| `$006E02`, `$006E58` | `move.w #3,$A4(a2)` | 10 (9 continues: the counter is decremented before `CONTINUE n` is shown) |

---

## 4. The frame: running an AI inside a 1992 game

### Input
`AiHook` replaces the instruction where the game copies the pads at the start of a
gameplay frame. In CPU PARTNER mode (and not in the attract demo) it:

1. switches to the AI's private stack and calls `ai_think()`, which returns a pad byte;
2. writes it to `$FC0A` (held) and, by comparing with last frame's byte, `$FC0B` (newly
   pressed), exactly as if a second controller were plugged in. The game itself can't tell
   the difference.

When the game is paused, Player 2's pad is released, except during high-score name
entry, where the ending freezes the game but the CPU still has to type its name.

### Time budget
The 68000 has no spare frame time when the screen is busy. Two checks keep the AI from
causing slowdown:
- **Dropped frame:** `$FC11` counts VBlanks. If it moved by more than one since the AI's
  last run, the game is lagging, and the AI skips a run (repeating its last pad).
- **Late frame:** `FrameDone` records the VDP's H/V counter when the main loop finished
  the previous frame. If that was on or after scanline `$C8`, there is no room this frame
  either.

On top of that, the AI's two loops over all objects are hand-written in assembly
(`scan.asm`), and the target/item selection runs only every other frame. Most frames the
AI costs a few percent of the CPU.

---

## 5. The AI (`ai.c`)

`think()` runs once per frame and returns the buttons to press (`SACBRLDU`).

### 5.1 Prompts and menus
| Player 2 state | Response |
|---|---|
| `$34/$36` character select | press START |
| `$38` CONTINUE / GAME OVER | press START while the human is playing; if the human is at GAME OVER, move to GAME OVER and confirm (that prompt never times out, so waiting would freeze the game) |
| `$3C` name entry | type `C`, `P`, `U`, then END |
| knocked into the air (10/16) | hold UP and tap JUMP to land on its feet |

### 5.2 Committed actions
Some actions take several frames and are played out before anything else:
- `hold`: keep a pad for N frames (special move, back attack, knife throw).
- jump kick: jump toward the target with the direction held, press attack 6 frames
  after take-off, and keep the direction until landing.

### 5.3 Scans
| Scan | When | Result |
|---|---|---|
| `ai_threats` (asm) | every frame | nearest **danger**: an enemy mid-attack, facing us and in reach, or anything fast (enemy or thrown object) heading at us in our lane, from position deltas between frames; plus how many enemies crowd us |
| `ai_select` (asm) | every other frame | best **target** (closest by `|dx| + 2|ddepth|`, on screen, hittable, not the one we gave up on) and best **pickup** (money, gold, 1UP always; apple below ½ health, turkey below ¼; health and 1UP are "urgent") |
| `ai_select2` (C) | every other frame | nearest intact **container**, nearest **weapon** on the floor (when unarmed), nearest enemy **waiting off-screen** |

The C versions of the two asm scans are kept in `ai.c` as the reference; the native build
(`host.c`) runs them, and a differential test compares the ROM against it frame by frame
(section 9).

### 5.4 Decisions, in priority order
1. **Danger.** Surrounded (two or more close) or something fast about to hit → special move
   (free, at most every 120 frames). Otherwise side-step out of its lane for 16 frames.
   A side-step that gets nowhere (walled in) is abandoned for 2 seconds.
2. **Errands.** Unless an enemy is right on top of the CPU (within 36 px):
   urgent pickup (health, 1UP) → **container** (stand 30 px beside it, face it, hit it) →
   **weapon** (if within 140 px or no enemy around) → money. Anything not reached within 250
   frames is ignored for a while (e.g. behind a barrel).
3. **Fight the target.**
   - **Knife or kunai in hand**, target lined up (±3 depth) 44–160 px away → turn and throw (attack + jump).
   - **Crowded** with an enemy right behind → back attack (unarmed only; armed, the same buttons throw the weapon).
   - **Target 32–90 px away**, within ±6 depth, standing → **jump kick**.
   - **Jet-pack enemies** in the air: keep 52 px away, line up, hit them when they come low (or jump-kick them).
   - **Enemy waiting off-screen** (see 5.6) → pace instead of chasing it to the screen edge.
   - Otherwise walk to 24 px from the target (30 px with a pipe or sword), on its side or the other one if that is off-screen; match depth; face it; attack. If it keeps backing out of range, step in for a moment.
4. **Nothing to fight.** Walk right to scroll the screen, keeping to the middle of the
   floor. If the human is out (continue screen), wait where they were.
5. **Stuck.** Pushing but not moving for 40 frames: first side-step around the obstacle;
   blocked again near the same spot → give up on that goal for a while (or, with nothing to
   fight, wait 3 seconds instead of walking into a wall).

### 5.5 Following the floor
The game keeps, per stage and area, a small table of floor boundaries at ROM `$D890`
(indexed by `$FC42`/`$FC44`). Each record holds the top edge `y0`, a second top edge `y1`,
and the x ranges of diagonal walls, with flags:
- bit 1: a top wall running down to the right between x `c` and `d` (top = `y0 + (x−c)/2`),
  then `y1`;
- bit 0: a bottom-right diagonal limiting depth near the start of an area;
- the bottom edge is the camera y + 216.

`floor_at(x)` reproduces this, which lets the CPU:
- **slide along a diagonal wall** when walking (add DOWN when the floor ahead starts
  lower), instead of pushing into it. Pushing into it stopped the screen from scrolling,
  and the next group of enemies never came;
- **walk forward in the middle of the floor** when there is nothing to fight.

### 5.6 Enemies that wait off-screen
Enemies keep their distance (about 130–160 px) from the nearest player and only move in
when players move. With a player standing still at the screen edge they wait just
outside the screen, sometimes for half a minute. When the CPU's target, or any standing
enemy, is off-screen, the CPU:
1. stands back from that edge (x 200 of 320), facing it, stepping up and down;
2. after 2 seconds, if the enemy still holds back, walks back and forth between
   x 150 and 250.

---

## 6. Other game changes

### No friendly fire
The game tests every attack, grab and thrown object against every other object. The four
filter hooks skip a candidate when **both** objects are players, but only in CPU PARTNER
mode and not in the attract demo, so normal 2-player mode keeps its friendly fire.

### Camera
Normally the screen only scrolls when both players reach the scroll point. `CamX/CamY`
ignore Player 1 for that test while the CPU is alive and in play, so the CPU can lead; a
lagging human is pushed along by the screen edge as usual.

### Free special move
SoR2 charges a player 8 health when their special hits something. The 8 routines that do
this end with `subq.w #8,$80(a2)` / `rts`. Each is now `trap #15` / `nop` / `rts`. The
trap handler skips the subtraction for Player 2 in CPU PARTNER mode and otherwise does
it, returning the same condition flags. TRAP #15 was unused (all trap vectors pointed at
the error handler).

---

## 7. The VBlank race (background corruption)

**Symptom:** a dotted grid over the sky in stage 7's last area (and similar damage in
stage 8).

**Cause:** the sky is drawn with tile 0, the blank tile. Something was writing into it.
Watching every VRAM write showed the culprit: the HUD routine that draws an enemy's name
and health bar (`$1CC4–$1DAC`, main loop). It sets the VDP address, then writes the
characters, and at that moment **VBlank interrupted it**. The VBlank handler uploads the
players' HUD icons and sprite tiles itself (`$4AE`, `$550`, `$5D0`, `$72C–$790`), plus a
DMA at `$191C` and one requested through `$FC54`. Its last upload ends at VRAM `$FFFF`, so
the VDP's address wrapped to `$0000`. When the HUD routine resumed, its writes went to
tile 0.

The unmodified game has the same race. It only fires when the main loop runs past a
frame while drawing the HUD, which the extra AI work made far more common.

**Fix:** the VBlank handler now notes at entry whether the main loop is waiting (`$FC10`
non-zero) or still busy. When it is busy, every block in the handler that touches the VDP
is skipped. Their request flags stay set, so the uploads happen at the next VBlank.
`$FC4B` is cleared so the DMA at `$191C` is skipped too. The pads are still read every
VBlank. A lag frame simply shows the previous frame a little longer. All game modes get
this fix.

## 8. A bug in the mod itself (v2–v4)

The first free-special patch replaced `subq.w #8,$80(a2)` / `rts` with a 6-byte `jmp`,
overwriting the `rts`. In one of the 8 routines, `$B3DC`, the game branches **to that
`rts`** (`beq $B3E0` at `$B3E8`), and so jumped into the middle of the `jmp`'s address.
The CPU executed the address as an instruction, which wrote over the stage and area
variables. The trap version keeps every original `rts` in place. `build.py` was then
checked with a scan for branches into any patched range.

---

## 9. How it was tested

None of this was tuned by hand-playing alone. Testing used a headless copy of the
**Genesis Plus GX** emulator (libretro core) driven from Python, with added debug hooks:
RAM and VRAM write logs with the program counter, exec coverage, breakpoints that capture
registers, and instruction counters.

- **Scripted Player 1:** the same AI compiled natively (`host.c` with the players
  swapped) plays Player 1, so whole stages run unattended.
- **Benchmarks:** every stage, several start offsets, 5 minutes each, comparing CPU health
  lost, deaths, damage dealt and time spent waiting for off-screen enemies between builds
  (e.g. v4: 23% fewer CPU deaths than v3 on HARDEST; v5: waits over 5 s eliminated).
- **Parameter sweeps** for the jump-kick range and timing, weapon distance, back-attack
  crowding and pacing, each run on fresh start offsets to avoid overfitting.
- **Differential test:** a debug build copies the RAM the AI is about to read; the native
  build runs `think()` on that copy, and the pad output and the AI's whole memory are
  compared with the ROM's, every frame, on every stage. They agree on every frame tested.
- **Crash sweeps** on every stage, a full playthrough from stage 1 to the ending on MANIA,
  and runs through the final boss into the credits.
