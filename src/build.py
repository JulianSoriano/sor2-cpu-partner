"""Build: C brain + assembly glue + the later patches (v9_patches.json, explained in patches-v9/*.s) -> patched ROM.

Needs vbcc (vbccm68k), vasm (vasmm68k_mot) and a host gcc, plus the original
"Streets of Rage 2 (USA).md" one folder up. Writes sor2_cpu_partner_v9.bin.
"""
import subprocess, struct, sys, os
HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, '..', 'Streets of Rage 2 (USA).md')
OUT = os.environ.get('AIOUT') or os.path.join(HERE, 'sor2_cpu_partner_v9.bin')
ASM_BASE = 0x1F8000
C_BASE = 0x1F9000
OPT = os.environ.get('VBOPT', '1')
END = 0x200000

def jsr(a): return bytes.fromhex('4eb9') + struct.pack('>I', a)
def jmp(a): return bytes.fromhex('4ef9') + struct.pack('>I', a)
NOP = bytes.fromhex('4e71')
E_ = lambda n: ASM_BASE + 6 * n

PATCHES = [
    (0x0007CC, '31f8fc08fc6e', jsr(E_(0))),
    (0x006768, '4eb900001af2', jsr(E_(1))),
    (0x0067E2, '1038fc09080000016728', jsr(E_(2)) + bytes.fromhex('60000054')),
    (0x006890, '0c780003fc18', jsr(E_(3))),
    (0x006A02, '1e38fc0b6104', jsr(E_(4))),
    (0x004CD2, 'b7ca670001b8', jsr(E_(5))),
    (0x0052BC, 'b7ca67000088', jsr(E_(6))),
    (0x004BA6, 'b7ea003467000108', jsr(E_(7)) + NOP),
    (0x006726, '0c780003fc1867044278fc18', jsr(E_(8)) + NOP * 3),
    (0x005162, 'b7ea008a670000a8', jsr(E_(9)) + NOP),
    (0x00393E, '4a6900826b0c', jsr(E_(13))),
    (0x0038BC, '4a6900826b0c', jsr(E_(14))),
    (0x001A40, '11fc0003fc10', jsr(E_(15))),
    (0x00B784, '0c6900360000', jsr(E_(16))),
    # title screen: credit line under the copyright notices, which move up a row
    (0x005C8A, '41fa008e4eb900001af2', jsr(E_(19)) + NOP * 2),
    (0x005D0A, '4c22', bytes.fromhex('4b22')), (0x005D1A, '4d16', bytes.fromhex('4c16')),
    # VBlank: no VDP work while the main loop is still busy (see VbEntry)
    (0x0004AA, '610015024bf8ef00', jsr(E_(20)) + NOP),
    (0x0004B2, '082d0005001e', jsr(E_(22))),
    (0x000554, '082d0005001e', jsr(E_(21))),
    (0x0005D4, '082d0005001e', jsr(E_(21))),
    (0x0006BE, '08380000fc54', jsr(E_(23))),
    (0x000730, '082d0006001e', jsr(E_(24))),
    (0x000762, '082d0006001e', jsr(E_(24))),
    (0x000412, '31fc0002fd06', bytes.fromhex('31fc0008fd06')),   # default lives: 9 (stored as 8)
    # continues: chosen in OPTIONS (3-9, default 3).  The counter is decremented
    # before the CONTINUE n prompt is shown, so SetCont starts it at n + 1.
    (0x006E02, '357c000300a4', jsr(E_(25))),
    (0x006E58, '357c000300a4', jsr(E_(25))),
    # ---- OPTIONS screen: LEVEL / PLAYERS / CONTINUES / ROUND / BGM / SE / VOICE / EXIT
    (0x00F896, '5378a9006a00000c31fc', jsr(E_(17)) + bytes.fromhex('60000076')),   # cursor up   (8 rows)
    (0x00F8CA, '5278a9000c780007a900', jsr(E_(18)) + bytes.fromhex('60000042')),   # cursor down (8 rows)
    (0x00F8F4, '3038a900d040', jmp(E_(11))),                          # row -> handler (adds CONTINUES)
    (0x00FC9E, '3438a9005242', jmp(E_(12))),                          # drawing routine
    (0x00FC7C, '4278fd0e', NOP * 2),                                  # keep ROUND when OPTIONS opens
    (0x00F9B4, '303c0004', bytes.fromhex('303c0008')),                # PLAYERS up to 9 without the cheat
    (0x00F974, '6600ff9e', bytes.fromhex('6000ff9e')),                # LEVEL: VERY EASY and MANIA without the cheat
    (0x00F99C, '6600ff76', bytes.fromhex('6000ff76')),
    # TRAP #15 -> SpecTrap (the vectors were all unused, pointing at the error handler)
    (0x0000BC, '00000206', struct.pack('>I', E_(10))),
# special moves: "subq.w #8,$80(a2) / rts" becomes "trap #15 / nop / rts". (v2-v4 put a
# 6-byte jmp there, which overwrote the rts that $B3E8 branches to: rare memory corruption.)
] + [(a, '516a00804e75', bytes.fromhex('4e4f4e714e75')) for a in (0xaed6, 0xafaa, 0xb03e, 0xb17e, 0xb1ca, 0xb28c, 0xb2f4, 0xb3dc)]

def run(cmd):
    r = subprocess.run(cmd, capture_output=True, text=True, cwd=HERE)
    if r.returncode: print(r.stdout, r.stderr); sys.exit(1)
    return r

def build(extra_cflags=(), debug=False, out=None):
    # vbcc: a mature 68000 C compiler emitting vasm Motorola syntax
    run(['vbccm68k', '-quiet', '-c99', '-O=%s' % OPT, '-cpu=68000', '-fpu=0', *extra_cflags, 'ai.c', '-o=ai_vb.asm'])
    ENTRIES = ('ai_think', 'ai_reset', 'ai_free_special')
    src = open(os.path.join(HERE, 'ai_vb.asm')).read().splitlines()
    src = [l for l in src if not l.strip().startswith(('section', 'idnt'))]
    hdr = ['\torg\t$%X' % C_BASE] + ['\tdc.w\t$4EF9\n\tdc.l\t_%s' % n for n in ENTRIES]   # fixed 6-byte jmp abs.l (no optimisation)
    # struct Mem offsets for the hand-written scans, taken from the C layout
    with open(os.path.join(HERE, 'memoff.c'), 'w') as f:
        f.write('#include <stdio.h>\n#include <stddef.h>\n#include "host.c"\nint main(void){\n')
        for fld in ('tick', 'vt', 'tnc', 'urgent', 'ign_t', 'ign', 'ex', 'ix', 'ctgt', 'citem', 'tdx', 'tdd'):
            f.write('printf("M_%s equ $FFFFFE20+%%d\\n",(int)offsetof(struct Mem,%s));\n' % (fld, fld))
        for k in ('VDT', 'VMIN', 'VMAX'):
            f.write('printf("%s equ %%d\\n", %s);\n' % (k, k))
        f.write('return 0;}\n')
    run(['gcc', *[f for f in extra_cflags if f.startswith('-D')], '-o', 'memoff', 'memoff.c'])
    open(os.path.join(HERE, 'mem.inc'), 'w').write(run(['./memoff']).stdout)
    open(os.path.join(HERE, 'ai_mot.asm'), 'w').write('\n'.join(hdr + src) + '\n\tinclude\t"scan.asm"\n')
    run(['vasmm68k_mot', '-Fbin', '-m68000', '-quiet', '-o', 'ai.bin', '-L', 'ai.lst', 'ai_mot.asm'])
    cblob = open(os.path.join(HERE, 'ai.bin'), 'rb').read()
    with open(os.path.join(HERE, 'c_syms.inc'), 'w') as f:
        for k, nme in enumerate(ENTRIES):
            f.write('%s equ $%06X\n' % (nme, C_BASE + 6 * k))
    run(['vasmm68k_mot', '-Fbin', '-m68000', '-no-opt', '-quiet'] + (['-DDEBUGSNAP=1'] if debug else []) + os.environ.get('GLUEDEF','').split() + ['-o', 'glue.bin', '-L', 'glue.lst', 'cpu_partner.asm'])
    ablob = open(os.path.join(HERE, 'glue.bin'), 'rb').read()
    assert len(ablob) <= C_BASE - ASM_BASE, 'glue too big'
    assert C_BASE + len(cblob) <= END, 'C too big'
    rom = bytearray(open(SRC, 'rb').read())
    assert all(b == 0xFF for b in rom[ASM_BASE:END])
    rom[ASM_BASE:ASM_BASE + len(ablob)] = ablob
    rom[C_BASE:C_BASE + len(cblob)] = cblob
    def direct(new):   # call the routine itself rather than its jump-table slot
        new = bytes(new)
        if new[:2] in (b'\x4e\xb9', b'\x4e\xf9'):
            t = struct.unpack('>I', new[2:6])[0]
            k = t - ASM_BASE
            if 0 <= k < 6 * 32 and k % 6 == 0 and ablob[k:k + 2] == b'\x4e\xf9':
                new = new[:2] + ablob[k + 2:k + 6] + new[6:]
        return new
    for addr, orig, new in PATCHES:
        new = direct(new)
        orig = bytes.fromhex(orig)
        assert rom[addr:addr + len(orig)] == orig, hex(addr)
        assert len(new) == len(orig), hex(addr)
        rom[addr:addr + len(new)] = new
    import json    # the v9 patches: small edits to the code above, each one checks the bytes it replaces
    for p in json.load(open(os.path.join(HERE, 'v9_patches.json'))):
        at, old, now = p['at'], bytes.fromhex(p['old']), bytes.fromhex(p['now'])
        assert rom[at:at + len(old)] == old, p['name']
        assert len(now) == len(old), p['name']
        rom[at:at + len(now)] = now
    cs = sum(struct.unpack('>%dH' % ((len(rom) - 0x200) // 2), rom[0x200:])) & 0xFFFF
    rom[0x18E:0x190] = struct.pack('>H', cs)
    open(out or OUT, 'wb').write(rom)
    print('glue %d bytes, C %d bytes, checksum %04X' % (len(ablob), len(cblob), cs))
    return out or OUT

if __name__ == '__main__':
    build(extra_cflags=tuple(os.environ.get('AIDEF', '').split()), debug='debug' in sys.argv, out=os.path.join(HERE, 'sor2_v9_debug.bin') if 'debug' in sys.argv else None)
