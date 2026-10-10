# Streets of Rage 2 — CPU Partner

ROM hack of the Sega Genesis game **Streets of Rage 2** (USA version) that adds a new mode
allowing the CPU to control the 2nd Player. You play Player 1; the CPU fights alongside you as Player 2, with
its own AI written in C and compiled for the console's 68000 CPU.

`2P CPU MOD BY RAMENPHOLIFE`

> **No ROM is included.** This repository contains an IPS patch and the source code only.
> You need your own copy of the original game.

---

## Quick start

1. Get a copy of **Streets of Rage 2 (USA)** (2,097,152 bytes).
   Check it before patching:

   | | |
   |---|---|
   | CRC32 | `e01fa526` |
   | MD5 | `cb75d5a6919b61efe917bea5762c1cb7` |
   | SHA-1 | `8b656eec9692d88bbbb84787142aa732b44ce0be` |

2. Apply [`patch/Streets of Rage 2 - CPU Partner v7.ips`](patch/) with any IPS patcher
   (Floating IPS, Lunar IPS, RomPatcher.js, …).
3. Load the patched ROM in any Genesis emulator or flash cart.
4. On the main menu choose **CPU PARTNER**.

## What you get

### Menus
| Where | What |
|---|---|
| Title screen | Credit line `2P CPU MOD BY RAMENPHOLIFE` (the game's font has no lower case). |
| Main menu | `1 PLAYER / 2 PLAYERS / DUEL / OPTIONS / CPU PARTNER`. **LEFT/RIGHT** picks the starting stage (1–8) for any mode. `2 PLAYERS` and `DUEL` are skipped when no second controller is plugged in. |
| Character select | After you lock in, **your pad picks the CPU's character**. You and the CPU may pick the same fighter. |
| Options | Difficulty is fixed at **MANIA**. The PLAYERS (lives) row is gone. |
| Lives / continues | Every player starts with **9 lives** and **9 continues**. |

### What the CPU partner does
- **Never hits you.** Friendly fire between the two players is switched off in CPU PARTNER mode only (normal 2-player mode is untouched).
- **Jump kicks** lined-up enemies 32–90 px away, closing the gap in the air and knocking them down instead of walking into their punches. Also jump-kicks jet-pack enemies.
- **Back attack** (attack + jump) when it is crowded and an enemy is right behind it.
- **Weapons:** picks up knives, pipes, swords and kunai, fights with them (keeping more distance with long weapons) and **throws knives and kunai** at lined-up enemies out of reach.
- **Breaks containers first** (arcade machines, barrels, drums, crates…), then collects what falls out.
- **Pickups:** always takes money, gold and 1UPs; takes an apple below half health and a turkey below a quarter.
- **Dodges** fast attackers (knife throwers, bikers, jet-packs, thrown weapons) by side-stepping out of their lane.
- **Special move for free** (no health cost to the CPU) at most every ~2 s, when surrounded or about to be hit. Your own special still costs health.
- **Recovers from throws:** holds up and taps jump to land on its feet.
- **Follows the floor:** slides along diagonal walls instead of pushing into them and walks up the middle of the street.
- **Draws in enemies waiting off-screen** instead of standing at the screen edge (which makes them hang back forever).
- **Pushes the screen forward** on its own; if you lag behind, the screen edge brings you along.
- **Handles every prompt by itself:** continues, re-picks its character, enters its high-score name (`CPU`), and chooses GAME OVER if you are out of continues so the game ends normally.

### Bug fixes to the original game
- **Background corruption** (dotted grid over the sky in stage 7's last area and in stage 8): a timing race in the game's VBlank handler, made much more frequent by the extra CPU load. Fixed for all modes. See [How it works → VBlank race](docs/HOW_IT_WORKS.md#7-the-vblank-race-background-corruption).

## Building from source

Requirements:
- **vbcc 0.9g** for m68k (`vbccm68k`) — newer versions work but won't reproduce the release byte-for-byte
- **vasm** with the Motorola syntax module (`vasmm68k_mot`)
- **gcc** (host compiler, used to generate struct offsets)
- **Python 3**

```sh
# put the original ROM in the repository root, named exactly:
#   "Streets of Rage 2 (USA).md"
cd src
python3 build.py              # -> src/sor2_cpu_partner_v7.bin
python3 build.py debug        # test build that snapshots the AI's input each frame
```

The release build is byte-identical to the ROM the IPS patch produces.

## Documentation

- [docs/HOW_IT_WORKS.md](docs/HOW_IT_WORKS.md) — full technical breakdown: memory map, every code patch, the AI, the game mechanics it relies on, and the bug fixes.
- [CHANGELOG.md](CHANGELOG.md) — what changed in each version and how it was tested.

## Repository layout

```
patch/   IPS patch for Streets of Rage 2 (USA)
src/     ai.c            the CPU partner's brain (C, compiled for the 68000)
         scan.asm        hand-written 68000 versions of the AI's two hot loops
         cpu_partner.asm hooks, menus, filters, bug fixes (68000 assembly)
         build.py        compiles, assembles and patches the original ROM
         host.c          native build of ai.c for differential testing on a PC
docs/    technical write-up
```

## Credits

Mod by **RamenPhoLife**. Streets of Rage 2 © SEGA 1992; music © Yuzo Koshiro.
This is an unofficial fan modification and is not affiliated with or endorsed by SEGA.
