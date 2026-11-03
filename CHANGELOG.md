# Changelog

## v9

Patches the partner's behaviour on top of v8. **The IPS patch applies to the original, unmodified Streets of Rage 2 (USA) ROM** (it contains v8 too). Everything below was measured with scripted test runs in RetroLab; it has not been through long play-testing yet.

- **Jump kicks:** fewer, and only when there is a real gap to close (cooldown 40 -> 180 frames, range 32-90 -> 48-80 px). On the stage 1 test route her jump kicks went from 114 to about 50-64.
- **Screen edges:** with nothing to fight she heads for the middle-right of the screen (camera + 230) and never gets more than 150 px ahead of you, instead of pressing into the right edge and pulling the camera away.
- **Twitching:** she no longer flickers left/right while waiting for off-screen enemies (a v8 bug: turning to face the enemy moved her just outside her "at my spot" zone, so she stepped back and repeated every frame), and which side of an enemy she stands on no longer flips every frame. A last filter only lets a left/right or up/down reversal through after the previous direction was held 8 frames. Test route: pace twitches 37 -> 0, pad twitches 14 -> 0 (left/right) and 23 -> 0 (up/down).
- **Bar tables and chairs:** a table blocks her about 38 px away, 8 px short of her "spot", and she never swung. She now attacks from up to 24 px short of her spot if she did not move last frame. In the test the bar table breaks about 4 s in instead of about 19 s.
- **2P tag:** a small "2P" (the HUD font's own tiles, the same ones the character-select screen's 2P marker uses) floats over the CPU partner so you can tell the two players apart even when you pick the same character. It sits a fixed 92 px above her feet, follows her through jumps, and only shows during play in CPU PARTNER mode (not on the character select or other menus).
- No slowdown or crash added: on the stage 1 route 179 slow frames (2.2%) before and after, 56-59% 68000 busy, no freezes.
- Tried and removed: giving the partner the versus-mode alternate colours when both pick the same character. The colours come from a palette line that later stages overwrite, so her outfit changed colour in some places.

Source for the v9 patches is in `src/v9/` (`make-v9.js` applies them to the v8 ROM; each `.s` file explains one patch). To rebuild: build v8 as described in the README, then `node src/v9/make-v9.js v8.md v9.md`. The released IPS is the difference between the original ROM and that result.

## v8

- OPTIONS restored and extended: every difficulty is selectable again (**NORMAL** by default), **PLAYERS** is back as 1–9 lives (default 9), and a new **CONTINUES** row sets 3–9 continues per player (default 3).
- The starting-stage picker moved from the main menu (LEFT/RIGHT) into OPTIONS as the **ROUND** row. The main menu is back to its five entries.

## v7

- 9 continues per player (was 2).

## v6

- Title screen credit line.
- Difficulty fixed at MANIA (no longer changeable in OPTIONS).
- Fixed broken backgrounds (a dotted grid over the sky in stage 7's last area, and the same kind of damage in stage 8). The game has a timing race: when it runs late, its VBlank routine uploads graphics in the middle of the main program's screen writes and sends those writes to the wrong place - here into the blank tile the sky is drawn with. The unmodified game has it too, but rarely; the CPU's extra work made it common. Now a VBlank that arrives while the game is still busy leaves the screen alone, and the upload happens at the next one.
- Played through the whole game on MANIA (stage 1 to the ending) with no freezes or stalls.

## v5

- Options: PLAYERS removed; 9 lives by default (options rows below it moved up).
- The CPU no longer hugs walls and the screen edge (it follows the floor along diagonal walls and draws in enemies that wait off-screen).
- Tested on HARDEST (all 8 stages, scripted Player 1): stretches with the enemies waiting off-screen longer than 5 seconds went from about 2 minutes in total (the longest 35 seconds) to none. The CPU's survival is unchanged or slightly better (about the same damage taken, 14% fewer deaths than v4).
- Fixed a rare memory corruption present since v2: the free-special patch overwrote an instruction the game sometimes jumps to, which could scramble the stage variables (seen in stage 7).

## v4

- Jump kicks, back attacks, weapon pickup / use / knife throwing.
- Breaks item containers first; always collects a 1UP.
- Lands on its feet after being thrown.
- Difficulty: HARD removed; HARDEST is the default (HARDEST / MANIA).
- Tested on HARDEST with a scripted Player 1 over 128 runs (every stage, 5 minutes each): compared with v3 the CPU died 23% less often, took 9% less damage and dealt 17% more. Its partner died 10% less often too.

## v3

- You and the CPU may pick the same character.
- Difficulty: NORMAL removed, MANIA always available, default HARD.
- Fixed the CPU walking in place next to the wall-mounted alien heads before the Vehelit fight (stage 3).
- The CPU now enters its name in the ending's high-score screen.
- Crash-proofing: the CPU validates its saved target pointers and resets its memory if it finds data from another version (e.g. an emulator save state made with an older build).

## v2.1

- Fixed a freeze when you reached GAME OVER while the CPU was down.
- The CPU's working memory moved to RAM the game never reads.

## v1 – v2
- Earlier releases: CPU-controlled Player 2 and the CPU PARTNER mode; from v2 the AI is written in C and compiled for the 68000.
