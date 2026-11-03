#!/usr/bin/env node
'use strict';
// CPU Partner v9 = v8 plus small patches to the partner's brain (no compiler needed: each patch replaces bytes at a
// known address and checks the old bytes first). Usage: node make-v9.js "<v8 rom>" out.md [--only name,name]
// Addresses are in the v8 ROM's compiled C code (ai.c): see CHANGELOG.md in the repository root for what each one does.
const fs = require('fs');
const crypto = require('crypto');
const PATCHES = [
  // 1. Partner jump-kick cooldown 40 -> 180 frames (JK_CD in ai.c): "move.b #$28,75(a0)" -> "#$B4"
  { name: 'jk-cooldown', at: 0x1FA3D0, old: '0028', now: '00b4' },
  // 2. Partner jump-kick range 32..90 -> 48..80 (JK_MIN, JK_MAX): it closes a real gap instead of kicking up close
  { name: 'jk-min', at: 0x1FA3A2, old: '7020', now: '7030' },
  { name: 'jk-max', at: 0x1FA3AA, old: '705a', now: '7050' },
  // 3. Nothing to fight: head for the middle-right of the screen (camera + 230) and never more than 150 ahead of the
  //    human, instead of the far right edge (camera + 400): the partner no longer presses into the side of the screen
  //    or drags the camera away from the human. The new code is idle-target.s, placed in free ROM at $1FAC00;
  //    the old 3 instructions at $1FA5F0 become a jump to it.
  { name: 'idle-target-code', at: 0x1FAC00, old: 'ff'.repeat(30), now: '202f003cd0bc000000e6222f0074d2bc00000096b0816f022001' + '2f40007c4ef9001fa5fe' },
  { name: 'idle-target-hook', at: 0x1FA5F0, old: '203c00000190d0af003c2f40007c', now: '4ef9001fac00' + '4e714e714e714e71' },
  // 4. Which side of an enemy the partner stands on no longer flips every frame when she is level with it
  //    (sticky-side.s, in free ROM at $1FAC40; the compare at $1FA14E becomes a jump to it).
  { name: 'sticky-side-code', at: 0x1FAC40, old: 'ff'.repeat(50), now: '200490af0074b0bc000000106e18b0bcfffffff06d064a38fea0660a4238fea04ef9001fa15411fc0001fea04ef9001fa15c' },
  { name: 'sticky-side-hook', at: 0x1FA14E, old: 'b8af00746c08', now: '4ef9001fac40' },
  // 5. The twitching: waiting for an off-screen enemy she walks to her waiting spot, then turns to face him. The turn
  //    steps her 2 px, which put her just outside the 6 px "at my spot" zone, so she stepped back, turned away, and
  //    repeated every frame (a v8 bug). Once she is facing the enemy the zone is now 24 px, so the turn sticks.
  //    (pace-zone.s, in free ROM at $1FAC80; the zone test at $1F9264 becomes a jump to it)
  { name: 'pace-zone-code', at: 0x1FAC80, old: 'ff'.repeat(38), now: '7206082a0000000f67064a856f0860044a856e027218b2806d064ef9001f92864ef9001f926a' },
  { name: 'pace-zone-hook', at: 0x1F9264, old: '7206b2806c1c', now: '4ef9001fac80' },
  // 6. Breaking bar tables and chairs: a table blocks her ~38 px away, 8 px short of her "spot", and she never swung.
  //    If she did not move last frame she now attacks from up to 24 px short of her spot (box-reach.s, free ROM at
  //    $1FACC0; the tolerance test at $1FA028 becomes a jump to it).
  { name: 'box-reach-code', at: 0x1FACC0, old: 'ff'.repeat(30), now: '7205b2806c0c7218b2806d0cb878fe3866064ef9001fa02e4ef9001fa062' },
  { name: 'box-reach-hook', at: 0x1FA028, old: '7205b2806d34', now: '4ef9001facc0' },
  // 7. Anti-twitch filter on everything the brain presses (steady-pad.s, free ROM at $1FAD00): a left<->right or
  //    up<->down reversal is only let through if the previous direction was held for 8+ frames. The jump stub for
  //    ai_think at $1F9002 now points at the filter, which calls the real brain at $1FA8A0.
  { name: 'steady-pad-code', at: 0x1FAD00, old: 'ff'.repeat(96), now: '4eb9001fa8a00c780034f000642412000201000c41f8fea2611a020000f3800112000201000341f8fea46108020000fc80014e750c2800ff00016704522800014a016716b21067124a1067080c280008000165081081422800014e7512104e75' },
  { name: 'steady-pad-hook', at: 0x1F9002, old: '001fa8a0', now: '001fad00' },
  // 8. A floating "2P" tag over the CPU partner so she can be told apart from the human whatever she looks like
  //    (tag2p.s, free ROM at $1FAF00). Two extra sprites using the HUD font tiles ($792 "2", $7B0 "P"), added to
  //    the sprite list after the game builds it; only in CPU PARTNER mode ($FFFE10) and only when there is room.
  //    The call that builds the sprite list at $001A1A becomes a call to this code, which makes that call first.
  { name: 'tag2p-code', at: 0x1FAF00, old: 'ff'.repeat(152), now: '4eb900020bdc4a38fe106700008a0c780034f000640000804a78f0006700007848e7f8103038fc140c40004e64643238f006064100809278fc649278fc260441005c0c410088654a0c41016064443438f00206420080d478fc2251420c42008865300c4201b0642a47f8ec803600e74bd6c33800524436c136c436fc879236c2524436c136c436fc87b0504236c25478fc144cdf081f4e75' },
  { name: 'tag2p-hook', at: 0x1A1A, old: '4eb900020bdc', now: '4eb9001faf00' },
];
const [romFile, out, ...rest] = process.argv.slice(2);
if (!out) { console.error('usage: make-v9.js <v8 rom> <out rom> [--only a,b]'); process.exit(1); }
const only = rest[0] === '--only' ? rest[1].split(',') : null;
const rom = Buffer.from(fs.readFileSync(romFile));
const sha = (b) => crypto.createHash('sha1').update(b).digest('hex');
if (sha(rom) !== 'c7d140896d1010ab4e52a0de081a59a9e81a111c') { console.error('This is not CPU Partner v8 (SHA-1 c7d14089...).'); process.exit(1); }
for (const p of PATCHES) {
  if (only && !only.includes(p.name)) continue;
  const old = Buffer.from(p.old, 'hex'), now = Buffer.from(p.now, 'hex');
  if (!rom.subarray(p.at, p.at + old.length).equals(old)) { console.error(`${p.name}: unexpected bytes at $${p.at.toString(16)}`); process.exit(1); }
  now.copy(rom, p.at); console.log(`applied ${p.name}`);
}
let s = 0; for (let i = 0x200; i + 1 < rom.length; i += 2) s = (s + rom.readUInt16BE(i)) & 0xFFFF;
rom.writeUInt16BE(s, 0x18E);
fs.writeFileSync(out, rom); console.log(`${out}  SHA-1 ${sha(rom)}`);
