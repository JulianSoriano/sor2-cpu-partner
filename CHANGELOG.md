# Changelog

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
