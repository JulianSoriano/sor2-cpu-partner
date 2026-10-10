/* Streets of Rage 2 - CPU partner brain (v5).
 * Compiled for the 68000 with vbcc (vbccm68k -c99 -cpu=68000) and linked into
 * free ROM space. Called once per gameplay frame from the AiHook stub;
 * returns the pad byte for Player 2 (SACBRLDU, 1 = held).
 * No writable C globals: persistent state lives at fixed RAM (struct Mem).
 * The per-object scans have hand-written 68000 versions in scan.asm; the C
 * versions below are the reference used by the native test build.
 */
typedef signed char s8;
typedef unsigned char u8;
typedef short s16;
typedef unsigned short u16;
typedef long s32;

#define PAD_U 0x01
#define PAD_D 0x02
#define PAD_L 0x04
#define PAD_R 0x08
#define PAD_B 0x10   /* attack */
#define PAD_C 0x20   /* jump */
#define PAD_A 0x40   /* special */
#define PAD_S 0x80   /* start */

#ifdef HOST   /* native build for differential testing against the ROM */
extern unsigned char hostram[0x10000];
static inline s16 host_w(long a) { unsigned o = (unsigned)a & 0xffff; return (s16)((hostram[o] << 8) | hostram[(o + 1) & 0xffff]); }
#define W(a)  host_w((long)(a))
#define UW(a) ((u16)host_w((long)(a)))
#define BY(a) (hostram[(unsigned)(long)(a) & 0xffff])
extern unsigned char hostrom[0x200000];
#define RW(a) ((s16)((hostrom[(long)(a)] << 8) | hostrom[(long)(a) + 1]))
#else
#define RW(a) (*(const s16 *)(a))   /* ROM word */
#define W(a)  (*(volatile s16 *)(a))
#define UW(a) (*(volatile u16 *)(a))
#define BY(a) (*(volatile u8 *)(a))
#endif

#ifdef HOST
typedef long OBJ;
#define FW(o, off) host_w((o) + (off))
#define FB(o, off) (hostram[((o) + (off)) & 0xffff])
#define OBJ_AT(a) ((OBJ)(a))
#define OLOW(o) ((s16)(o))
#define OFROM(v) ((OBJ)(0xFFFF0000L | (u16)(v)))
#else
typedef u8 *OBJ;           /* object base; fields read with d16(An) */
#define FW(o, off) (*(s16 *)((o) + (off)))
#define FB(o, off) (*(u8 *)((o) + (off)))
#define OBJ_AT(a) ((OBJ)(a))
#define OLOW(o) ((s16)(long)(o))
#define OFROM(v) ((OBJ)(0xFFFF0000L | (u16)(v)))
#endif
#ifdef SWAP   /* host test bot playing Player 1 */
#define P1 OBJ_AT(0xFFFFF000L)
#define P2 OBJ_AT(0xFFFFEF00L)
#else
#define P1 OBJ_AT(0xFFFFEF00L)
#define P2 OBJ_AT(0xFFFFF000L)
#endif
#define EN0 OBJ_AT(0xFFFFF100L)
#define IT0 OBJ_AT(0xFFFFF700L)
#define NEN 6
#define NIT 6

/* object fields */
#define O_ST    0x00
#define O_X     0x02
#define O_Y     0x06
#define O_TYPE  0x0C
#define O_FACE  0x0F   /* bit 0: facing right */
#define O_IFL   0x32   /* item flags */
#define O_FL49  0x49   /* bit 7: can be hit */
#define O_INV   0x50
#define O_Z     0x5E   /* height, negative = in the air */
#define O_HP    0x80

#define CAMX   (-W(0xFFFFFC22L))
#define PAUSED (W(0xFFFFFC3EL) & 0xFF)

#define HP_MAX 104
#define MEM_MAGIC 0xA7     /* change whenever struct Mem changes layout */
/* cached object pointers come from RAM: only trust real slot addresses */
#define EN_OK(v) ((u16)(v) >= 0xF100 && (u16)(v) <= 0xF600 && ((v) & 0xFF) == 0)
#define IT_OK(v) ((u16)(v) >= 0xF700 && (u16)(v) <= 0xF980 && ((v) & 0x7F) == 0)

/* item types */
#define IT_MONEY  140
#define IT_GOLD   142
#define IT_1UP    144
#define IT_APPLE  146
#define IT_TURKEY 148
#define IT_KNIFE  60      /* weapons: types below 140 */
#define IT_PIPE   62
#define IT_KUNAI  68      /* (66 is the sword) */

/* player object, extra fields */
#define O_ARMED 0x32       /* bit 0: holding a weapon */
#define O_WPN   0x36       /* low word of the held weapon's address (long at $34) */

/* tuning */
#define ATK_R     24      /* preferred horizontal distance to a target */
#ifndef ATK_WIN
#define ATK_WIN   32      /* start swinging within ATK_R + ATK_WIN */
#endif
#define ZONEMAX   60
#define YTOL      3
#define XTOL      6
#ifndef SPEC_CD
#define SPEC_CD   120     /* frames between specials ("sparingly") */
#endif
#ifndef SPEC_DIST
#define SPEC_DIST 56
#endif
/* v4 moves */
#ifndef JK_MIN
#define JK_MIN    32      /* jump kick when the target is this far ... */
#endif
#ifndef JK_MAX
#define JK_MAX    90      /* ... up to this far (horizontally) */
#endif
#ifndef JK_DD
#define JK_DD     6       /* and lined up in depth */
#endif
#ifndef JK_CD
#define JK_CD     40
#endif
#ifndef JK_AT
#define JK_AT     6       /* press attack this many frames after the jump starts */
#endif
#define JUMP_T    60      /* give up on a jump after this long */
#ifndef BOX_R
#define BOX_R     30      /* stand this far from a container to hit it */
#endif
#ifndef WPN_RANGE
#define WPN_RANGE 140     /* fetch weapons up to this far away while enemies are around */
#endif
#ifndef THR_MIN
#define THR_MIN   44
#endif
#ifndef THR_MAX
#define THR_MAX   160
#endif
#ifndef WPN_R
#define WPN_R     30      /* preferred distance when swinging a long weapon */
#endif
#ifndef BK_CROWD
#define BK_CROWD  2       /* back attack only with this many enemies around us */
#endif
#ifndef PACE_TGT
#define PACE_TGT  1
#endif
#ifndef PACE_IDLE
#define PACE_IDLE 1
#endif
#ifndef SLIDE
#define SLIDE     2       /* 1: always, 2: only when not fighting */
#endif
#ifndef MIDFLOOR
#define MIDFLOOR  1
#endif
#ifndef PACE_T
#define PACE_T    120     /* off-screen enemy: hold at PACE_X this long, then walk PACE_A..PACE_B */
#endif
#ifndef PACE_X
#define PACE_X    200
#endif
#ifndef PACE_A
#define PACE_A    150     /* enemy waiting off-screen to the right: pace between */
#endif
#ifndef PACE_B
#define PACE_B    250     /* these screen x (mirrored for the left) */
#endif
#ifndef CLOSE_X
#define CLOSE_X   36      /* an enemy this close comes before any errand */
#endif

#ifdef NOSPLIT          /* scan everything every frame (1-frame velocities) */
#define VDT 1
#define VMIN 3
#define VMAX 16
#define SCAN_THREATS 1
#define SCAN_SELECT 1
#elif defined(SPLIT2)   /* threats on odd frames (2-frame velocities), selection on even */
#define VDT 2
#define VMIN 6
#define VMAX 32
#define SCAN_THREATS (M->tick & 1)
#define SCAN_SELECT (!(M->tick & 1))
#else                   /* threats every frame, target/item choice every other frame */
#define VDT 1
#define VMIN 3
#define VMAX 16
#define SCAN_THREATS 1
#define SCAN_SELECT (!(M->tick & 1))
#endif

struct Mem {               /* at $FFFE20, max 192 bytes (the game never touches $FE00-$FEFF) */
    u8  tick;
    u8  spec_cd;
    u8  dodge_t;
    u8  dodge_dir;
    u8  stuck_n;
    u8  zone;
    u8  ign_t;
    u8  hold;              /* frames to keep holding the same pad */
    u8  hold_pad;
    u8  jump_t;
    u8  pick_t;
    u8  urgent;            /* cached item is a health pickup we need */
    u8  vt;                /* tick of the last threat scan */
    u8  tnc;               /* threat scan: enemies crowding us */
    u8  idle_t;            /* nothing to fight and the way ahead is blocked: stand still */
    u8  magic;             /* MEM_MAGIC once initialised by this version */
    u8  ndodge_t;          /* a dodge got nowhere: no threat dodging for a while */
    u8  dblk;              /* frames the current dodge has not moved us */
    u8  nstk;              /* blocked again near the same spot: how many times */
    u8  iign_t;            /* ignoring item iign for this many frames */
    u8  ipur;              /* frames spent going for the current item */
    u8  paced;             /* pace_for() ran this frame */
    s16 ign;
    s16 lastx, lasty;
    s16 ex[NEN];           /* enemy x last frame */
    s16 ix[NIT];           /* item x last frame */
    s16 lasttgt;
    s16 ctgt, citem;       /* cached target / item (low address word, 0 = none) */
    s16 tdx, tdd;          /* threat scan: nearest danger (dx, depth diff) */
    s16 stkx, stky;        /* where we last got blocked */
    s16 iign, ipick;       /* item we gave up on / item we are going for */
    /* v4 */
    s16 cbox, cwpn;        /* best container to break / weapon to pick up (0 = none) */
    u8  jdir;              /* jump kick: direction held */
    u8  jk_cd;             /* frames before the next jump kick */
    u8  bk_cd;             /* frames before the next back attack */
    u8  thr_cd;            /* frames before the next knife throw */
    u8  jair;              /* jump kick: frames since take-off */
    u8  pace;              /* waiting for an off-screen enemy: walking right (1) or left (0) */
    u8  wait_t;            /* frames spent pacing for off-screen enemies */
    s16 woff;              /* screen x of the nearest enemy waiting off-screen (0 = none) */
};
#ifdef TRACE
#ifdef HOST
extern s16 hosttrace[32];
#define DBG(n, v) (hosttrace[n] = (s16)(v))
#else
#define DBG(n, v) (*(volatile s16 *)(0xFFFF9100L + 2 * (n)) = (s16)(v))
#endif
#else
#define DBG(n, v) ((void)0)
#endif
#ifdef HOST
extern struct Mem hostmem;
#define M (&hostmem)
#else
#define M ((struct Mem *)0xFFFFFE20L)
#endif

#define iabs(v) ((v) < 0 ? -(v) : (v))
#define depth(o) (FW(o, O_Y) - FW(o, O_Z))

#define en_valid(o) (FW(o, O_ST) != 0 && (FB(o, O_FL49) & 0x80) && FW(o, O_INV) == 0 && FW(o, O_HP) > 0)

/* enemy is standing/attacking (not knocked down or recovering) */
#define en_upright(est, o) ((est) != 0 && (est) != 6 && (est) != 8 && (est) != 10 && FW(o, O_HP) > 0)

static int face_or_attack(OBJ me, int dx, int pad, int atk)
{
    /* dx = target x - my x */
    int want_r = dx > 0;
    int face_r = FB(me, O_FACE) & 1;
    if (want_r != face_r)
        return (int)((pad & ~(PAD_L | PAD_R)) | (want_r ? PAD_R : PAD_L));
    return (M->tick & 1) ? atk : 0;
}

/* The walkable floor at x (the game's own collision table, $D890): depth
 * from *lo to *hi. Diagonal walls (stage 1's buildings, ...) move lo. */
#define FLOOR_TBL 0xD890L
static void floor_at(int x, int *lo, int *hi)
{
    int sg = W(0xFFFFFC42L), ar = W(0xFFFFFC44L);
    if ((u16)sg > 14 || (u16)ar > 30 || ((sg | ar) & 1)) { *lo = -32000; *hi = 32000; return; }
    long b = FLOOR_TBL + sg;
    long r = b + RW(b) + 8 * (long)ar;
    int y0 = RW(r), y1 = RW(r + 2), fl = RW(r + 12);
    int top = y0, bot = W(0xFFFFFC26L) + 216;
    if (fl & 2) {                          /* top wall running down to the right */
        int c = RW(r + 8), d = RW(r + 10);
        if (x >= d) top = y1; else if (x >= c) top = y0 + ((x - c) >> 1);
    }
    if (fl & 1) {
        int a = RW(r + 4), e = RW(r + 6);
        if (x >= a) {
            top = y1;
            if (x < e) { int t = y0 + ((e - x) >> 1); if (t < bot) bot = t; }
        }
    }
    *lo = top; *hi = bot;
}

/* walk toward (gx, gd); returns pad bits */
static int walk_to(int mx, int md, int gx, int gd, int xt, int dt);

/* An enemy waiting just off-screen (screen x tsx) keeps its distance from the
 * nearest player and only moves in when players move; standing at the screen
 * edge, or standing still anywhere, keeps it waiting. Pace back and forth in
 * the part of the screen facing it, at depth gd. */
static int pace_for(OBJ me, int cam, int mx, int md, int tsx, int gd)
{
    int lo, hi;
    int hx = cam + (tsx < 0 ? 320 - PACE_X : PACE_X);
    floor_at(hx, &lo, &hi);
    if (gd < lo + 16) gd = lo + 16; else if (gd > hi - 16) gd = hi - 16;
    if (M->wait_t < 255) M->wait_t++;
    M->paced = 1;
    if (M->wait_t < PACE_T) {
        /* first stand back from the edge facing it, stepping up and down */
        if (iabs(hx - mx) > 6) return walk_to(mx, md, hx, gd, 4, 40);
        if ((FB(me, O_FACE) & 1) != (tsx > 0)) return tsx > 0 ? PAD_R : PAD_L;
        if (md <= gd - 12) M->pace = 1; else if (md >= gd + 12) M->pace = 0;
        return M->pace ? PAD_D : PAD_U;
    }
    {
        /* still waiting: walk back and forth */
        int msx = mx - cam;
        int a = tsx < 0 ? 320 - PACE_B : PACE_A, b = tsx < 0 ? 320 - PACE_A : PACE_B;
        if (msx <= a) M->pace = 1; else if (msx >= b) M->pace = 0;
        return walk_to(mx, md, M->pace ? cam + b + 8 : cam + a - 8, gd, 2, 4);
    }
}

static int walk_to(int mx, int md, int gx, int gd, int xt, int dt)
{
    int p = 0;
    int ex = gx - mx, ed = gd - md;
    if (ex > xt) p |= PAD_R; else if (ex < -xt) p |= PAD_L;
    if (ed > dt) p |= PAD_D; else if (ed < -dt) p |= PAD_U;
    return p;
}

/* The two per-object scans. The ROM uses hand-written 68000 versions
 * (scan.asm); these C versions define the behaviour and are what the
 * native test build runs, so the differential test checks one against
 * the other. */
#if defined(HOST) || defined(CSCAN)
static void ai_threats(long mx_, long md_)
{
    int mx = (int)mx_, md = (int)md_;
    OBJ o; int i;
    {
        int danger_dx = 0x7fff; int danger_dd = 0; int n_close = 0;
        int vok = (u8)(M->tick - M->vt) == VDT;
        M->vt = M->tick;
        s16 *px = M->ex;
        for (i = 0, o = EN0; i < NEN; i++, o += 0x100, px++) {
            int x = FW(o, O_X);
            int vx = x - *px;
            int est = FW(o, O_ST);
            *px = x;
            if (!en_upright(est, o)) continue;
            int dx = x - mx, dd = depth(o) - md;
            if (iabs(dx) < 40 && iabs(dd) < 12 && (FB(o, O_FL49) & 0x80)) n_close++;
#ifndef NO_ATKDODGE
            {   /* enemy mid-attack, facing us, close enough to connect */
                int atk = est >= 24 && est < 60 && est != 36 && est != 38;
                int reach = FW(o, O_TYPE) == 24 ? 112 : 72;
                int facing_us = (FB(o, O_FACE) & 1) ? (dx < 0) : (dx > 0);
                if (atk && facing_us && iabs(dd) < 12 && iabs(dx) < reach && iabs(dx) < iabs(danger_dx)) {
                    danger_dx = dx; danger_dd = dd;
                }
            }
#endif
            if (!vok || iabs(vx) > VMAX) continue;             /* respawn jump */
            if (iabs(vx) >= VMIN && iabs(dd) < 14 && FW(o, O_Z) > -40 &&
                ((vx > 0 && dx < 0) || (vx < 0 && dx > 0)) && iabs(dx) < 120) {
                if (iabs(dx) < iabs(danger_dx)) { danger_dx = dx; danger_dd = dd; }
            }
        }
        px = M->ix;
        for (i = 0, o = IT0; i < NIT; i++, o += 0x80, px++) {
            int ist = FW(o, O_ST);
            if (ist == 0) continue;                          /* empty slot */
            int x = FW(o, O_X);
            int vx = x - *px;
            *px = x;
            if (ist == 2) continue;
            if (!(FB(o, O_IFL) & 0x80)) continue;
            if (!vok || iabs(vx) < VMIN || iabs(vx) > VMAX) continue;
            int dx = x - mx, dd = FW(o, O_Y) - md;
            if (iabs(dd) < 16 && ((vx > 0 && dx < 0) || (vx < 0 && dx > 0)) && iabs(dx) < 120)
                if (iabs(dx) < iabs(danger_dx)) { danger_dx = dx; danger_dd = dd; }
        }
        M->tdx = danger_dx; M->tdd = danger_dd; M->tnc = n_close;
    }
}

static void ai_select(long cam_, long mx_, long md_, long hp_)
{
    int cam = (int)cam_, mx = (int)mx_, md = (int)md_, hp = (int)hp_;
    OBJ o, tgt = 0, item = 0; int i, urgent = 0;
    /* ---- pick a target enemy ---- */
    int best = 0x7fff;
    for (i = 0, o = EN0; i < NEN; i++, o += 0x100) {
        DBG(16 + i, -1);
        if (!en_valid(o)) continue;
        DBG(16 + i, -2);
        if (M->ign_t && OLOW(o) == M->ign) continue;
        int ox = FW(o, O_X);
        int sx = ox - cam;
        if (sx < -48 || sx > 368) continue;
        int d1 = ox - mx, d2 = depth(o) - md;
        int d = iabs(d1) + 2 * iabs(d2);
        if (sx < 4 || sx > 316) d += 64;
        DBG(16 + i, d); DBG(24 + i, sx);
        if (d < best) { best = d; tgt = o; }
    }

    DBG(5, tgt); DBG(6, best);
    /* ---- items worth grabbing ---- */
    int ibest = 0x7fff;
    for (i = 0, o = IT0; i < NIT; i++, o += 0x80) {
        if (FW(o, O_ST) != 2) continue;
        if ((FB(o, O_IFL) & 0xC1) != 0x80) continue;      /* a pickup, not a weapon or crate */
        int t = FW(o, O_TYPE);
        int want = 0, urg = 0;
        if (t == IT_MONEY || t == IT_GOLD) want = 1;
        else if (t == IT_1UP) { want = 1; urg = 1; }
        else if (t == IT_APPLE && hp < HP_MAX / 2) { want = 1; urg = 1; }
        else if (t == IT_TURKEY && hp < HP_MAX / 4) { want = 1; urg = 1; }
        if (!want) continue;
        int ox = FW(o, O_X);
        int sx = ox - cam;
        if (sx < 0 || sx > 320) continue;
        int d1 = ox - mx, d2 = FW(o, O_Y) - md;
        int d = iabs(d1) + 2 * iabs(d2);
        if (urg) d -= 400;
        if (d < ibest) { ibest = d; item = o; urgent = urg; }
    }
    DBG(7, item); DBG(8, ibest);
    M->ctgt = tgt ? OLOW(tgt) : 0; M->citem = item ? OLOW(item) : 0; M->urgent = urgent;
}
#else
extern void ai_threats(long mx, long md);
extern void ai_select(long cam, long mx, long md, long hp);
#endif

/* v4: containers to break and weapons to pick up (item slots) */
static void ai_select2(int cam, int mx, int md, int armed)
{
    OBJ o, box = 0, wpn = 0; int i, bb = 0x7fff, wb = 0x7fff;
    for (i = 0, o = IT0; i < NIT; i++, o += 0x80) {
        int st = FW(o, O_ST);
        if (st == 0 || !(FB(o, O_FL49) & 0x80)) continue;
        int fl = FB(o, O_IFL);
        int ox = FW(o, O_X), sx = ox - cam;
        if (sx < 8 || sx > 312) continue;
        int d = iabs(ox - mx) + 2 * iabs(FW(o, O_Y) - md);
        if (st == 2 && (fl & 0x40)) {                    /* intact container */
            if (d < bb) { bb = d; box = o; }
        }
#ifndef NOWPN
        else if (!armed && st == 8 && (fl & 0xC1) == 0x80 && FW(o, O_TYPE) < IT_MONEY && FW(o, O_Z) == 0) {
            if (d < wb) { wb = d; wpn = o; }               /* weapon lying on the floor */
        }
#endif
    }
    M->cbox = box ? OLOW(box) : 0; M->cwpn = wpn ? OLOW(wpn) : 0;
    /* enemies standing just off-screen, waiting for us to come into view */
    {
        int wbest = 0x7fff, w = 0;
        for (i = 0, o = EN0; i < NEN; i++, o += 0x100) {
            if (FW(o, O_ST) != 2 || !en_valid(o)) continue;
            int sx = FW(o, O_X) - cam;
            if (sx >= -8 && sx <= 328) continue;
            int d = iabs(sx - 160);
            if (d < 360 && d < wbest) { wbest = d; w = sx; }
        }
        M->woff = w;
    }
}

/* an upright enemy right behind us (we face away from it)? */
static int enemy_behind(OBJ me, int mx, int md)
{
    OBJ o; int i;
    int face_r = FB(me, O_FACE) & 1;
    for (i = 0, o = EN0; i < NEN; i++, o += 0x100) {
        int est = FW(o, O_ST);
        if (!en_upright(est, o) || !(FB(o, O_FL49) & 0x80)) continue;
        int dx = FW(o, O_X) - mx, dd = depth(o) - md;
        if (iabs(dd) < 8 && iabs(dx) < 40 && (face_r ? dx < 0 : dx > 0)) return 1;
    }
    return 0;
}

static int think(void)
{
    OBJ me = P2, ally = P1, o;
    int st = FW(me, O_ST);
    int cam, mx, md, hp;
    int i;
    int pad = 0;

    if (st == 0) return 0;
    if (M->magic != MEM_MAGIC) {
        /* state from another version (e.g. an emulator save state): start clean */
        u8 *p = (u8 *)M;
        for (i = 0; i < (int)sizeof(struct Mem); i++) p[i] = 0;
        M->magic = MEM_MAGIC;
    }
    M->tick++;

    /* ---- continue prompt, character pick, name entry ---- */
    if (st >= 0x34 && st <= 0x3E) {
        if (PAUSED && st != 0x3C) return 0;   /* name entry also runs while frozen */
        if (st == 0x38) {                      /* CONTINUE / GAME OVER */
            int a = FW(ally, O_ST);
            if (a == 0x3A) {
                /* The human is out for good (GAME OVER). This prompt never
                 * times out, so waiting here would freeze the game: move the
                 * cursor to GAME OVER (byte $3A = 0 means CONTINUE) and confirm. */
                if (FB(me, 0x3A) == 0) return (M->tick & 7) == 0 ? PAD_D : 0;
                return (M->tick & 7) == 4 ? PAD_S : 0;
            }
            if (a == 0 || a >= 0x34) return 0;   /* only while the human plays */
            return (M->tick & 7) == 0 ? PAD_S : 0;
        }
        if (st == 0x34 || st == 0x36)          /* SELECT PLAYER */
            return (M->tick & 7) == 0 ? PAD_S : 0;
        if (st == 0x3C) {
            /* high-score name entry: spell "CPU" (letters A=0 .. ; $72 = letter
             * under the cursor, $3A = 0/2/4 for the three slots, 6 = on END) */
            int pos = FW(me, 0x3A), cur = FW(me, 0x72);
            int want = pos == 0 ? 2 : pos == 2 ? 15 : 20;
            if (M->tick & 1) return 0;           /* release between presses */
            if (pos >= 6 || cur > 0x20) return PAD_C;
            if (cur < want) return PAD_R;
            if (cur > want) return PAD_L;
            return PAD_C;
        }
        /* other prompts: confirm with C */
        return (M->tick & 3) == 0 ? PAD_C : 0;
    }

#ifndef NORECOVER
    /* thrown or knocked into the air: hold UP and tap JUMP to land on our feet */
    if ((st == 10 || st == 16) && FW(me, O_Z) < 0)
        return (M->tick & 1) ? (PAD_U | PAD_C) : PAD_U;
#endif
#ifdef ESCAPE
    /* held by an enemy: special breaks the hold */
    if (st == 12 && (FB(me, O_FL49) & 0x10))
        return (M->tick & 2) ? PAD_A : 0;
#endif
    if (M->spec_cd) M->spec_cd--;
    if (M->ign_t) M->ign_t--;
    if (M->pick_t) M->pick_t--;
    if (M->ndodge_t) M->ndodge_t--;
    if (M->iign_t) M->iign_t--;
    if (M->jk_cd) M->jk_cd--;
    if (M->bk_cd) M->bk_cd--;
    if (M->thr_cd) M->thr_cd--;

    /* keep a committed action going (special, jump kick) */
    if (M->hold) { M->hold--; return M->hold_pad; }
    if (M->jump_t) {
        /* jump kick: jump toward the target, kick on the way up, hold the
         * direction until we land */
        M->jump_t--; M->jair++;
        int dir = M->jdir;
        if (M->jair > 6 && st != 24) M->jump_t = 0;   /* landed, or knocked out of it */
        else if (M->jair <= 2) return dir | PAD_C;
        else if (M->jair >= JK_AT && M->jair < JK_AT + 2) return dir | PAD_B;
        else return dir;
    }

    cam = CAMX;
    mx = FW(me, O_X);
    md = depth(me);
    hp = FW(me, O_HP);
    DBG(0, cam); DBG(1, mx); DBG(2, md); DBG(3, hp); DBG(4, st);

    /* ---- threats (odd frames): fast movers and thrown weapons heading at us.
     * Velocities are 2-frame deltas; they are only trusted when the previous
     * scan was exactly two frames ago. */
    if (SCAN_THREATS) {
        int danger_dx, danger_dd, n_close;
        ai_threats(mx, md);
        danger_dx = M->tdx; danger_dd = M->tdd; n_close = M->tnc;
        if (st == 2) {
            /* surrounded, or something fast is about to hit: free special */
            if (M->spec_cd == 0 && (n_close >= 2 || iabs(danger_dx) < SPEC_DIST)) {
                M->spec_cd = SPEC_CD;
                M->hold = 3; M->hold_pad = PAD_A;
                return PAD_A;
            }
            if (danger_dx != 0x7fff && iabs(danger_dx) < 110 && !M->ndodge_t) {
                /* side-step out of its lane */
                if (!M->dodge_t) M->dblk = 0;
                M->dodge_t = 16;
                M->dodge_dir = danger_dd >= 0 ? PAD_U : PAD_D;
            }
        }
    }
    if (M->dodge_t) {
        M->dodge_t--;
        /* blocked by the edge of the floor: go the other way */
        if (M->dodge_t < 13 && md == M->lasty && (M->dodge_dir & (PAD_U | PAD_D)))
            M->dodge_dir ^= (PAD_U | PAD_D);
        /* walled in both ways (e.g. a wall-mounted enemy that is always
         * "attacking"): stop dodging for 2 seconds instead of walking in place */
        if (mx == M->lastx && md == M->lasty) {
            if (++M->dblk >= 24) { M->dblk = 0; M->dodge_t = 0; M->ndodge_t = 120; }
        } else M->dblk = 0;
        M->lastx = mx; M->lasty = md;
        return M->dodge_dir;
    }

    OBJ tgt = 0, item = 0, goal = 0, box = 0, wpn = 0; int urgent = 0;
    if (!M->paced) M->wait_t = 0;        /* stopped pacing last frame: start over */
    M->paced = 0;
    int armed = FB(me, O_ARMED) & 1, wtype = 0;
    if (armed && IT_OK(UW(me + O_WPN))) wtype = FW(OFROM(UW(me + O_WPN)), O_TYPE);
    if (SCAN_SELECT) ai_select2(cam, mx, md, armed);
    if (IT_OK(M->cbox)) { box = OFROM(M->cbox); if (FW(box, O_ST) != 2 || !(FB(box, O_IFL) & 0x40)) box = 0; }
    if (IT_OK(M->cwpn)) { wpn = OFROM(M->cwpn); if (armed || FW(wpn, O_ST) != 8 || FW(wpn, O_Z) != 0) wpn = 0; }
    if (SCAN_SELECT) {
        ai_select(cam, mx, md, hp);
        if (EN_OK(M->ctgt)) tgt = OFROM(M->ctgt);
        if (IT_OK(M->citem)) item = OFROM(M->citem);
        urgent = M->urgent;
    } else {
        /* odd frames: reuse the last choice if it is still good */
        if (EN_OK(M->ctgt)) { tgt = OFROM(M->ctgt); if (!en_valid(tgt) || (M->ign_t && M->ctgt == M->ign)) tgt = 0; }
        if (IT_OK(M->citem)) { item = OFROM(M->citem); if (FW(item, O_ST) != 2) item = 0; }
        urgent = M->urgent;
    }
    if (M->iign_t) {   /* unreachable for now */
        if (item && OLOW(item) == M->iign) item = 0;
        if (box && OLOW(box) == M->iign) box = 0;
        if (wpn && OLOW(wpn) == M->iign) wpn = 0;
    }
    {
        /* errands: health/1UP first, then break containers, grab a weapon,
         * collect money - but fight first if an enemy is right on top of us */
        int c1 = tgt ? FW(tgt, O_X) - mx : 0, c2 = tgt ? depth(tgt) - md : 0;
        int enemy_close = tgt && iabs(c1) < CLOSE_X && iabs(c2) < 10;
        int gk = 0;
        if (item && urgent) { goal = item; gk = 1; }
        else if (!enemy_close) {
            if (box) { goal = box; gk = 2; }
            else if (wpn && (!tgt || iabs(FW(wpn, O_X) - mx) < WPN_RANGE)) { goal = wpn; gk = 3; }
            else if (item) { goal = item; gk = 1; }
        }
        if (goal) {
            int gx = FW(goal, O_X), gy = FW(goal, O_Y);
            /* give up on something we cannot get to (e.g. behind barrels) */
            if (OLOW(goal) != M->ipick) { M->ipick = OLOW(goal); M->ipur = 0; }
            if (++M->ipur >= 250) { M->ipur = 0; M->iign = OLOW(goal); M->iign_t = 240; }
            if (gk == 2) {
                /* stand beside the container, face it, hit it */
                int hx = gx + (mx < gx ? -BOX_R : BOX_R);
                int hs = hx - cam;
                if (hs < 8 || hs > 312) hx = gx + (gx - hx);
                int e1 = hx - mx, e2 = gy - md;
                if (iabs(e1) <= 5 && iabs(e2) <= 3) pad = face_or_attack(me, gx - mx, 0, PAD_B);
                else pad = walk_to(mx, md, hx, gy, 3, 2);
            } else {
                int e1 = gx - mx, e2 = gy - md;
                if (iabs(e1) <= 7 && iabs(e2) <= 5) {
                    pad = (M->tick & 1) ? PAD_B : 0;     /* pick it up */
                } else {
                    pad = walk_to(mx, md, gx, gy, 4, 3);
                }
            }
            goto stuck;
        }
    }

    if (tgt) {
        M->idle_t = 0;
        int tx = FW(tgt, O_X), td = depth(tgt), tz = FW(tgt, O_Z);
        /* long weapons (pipe, sword) keep a little more distance */
        int ar = (armed && wtype != IT_KNIFE && wtype != IT_KUNAI) ? WPN_R : ATK_R;
        int gx = tx + (mx < tx ? -ar : ar);
        int gs = gx - cam;
        if (gs < 12 || gs > 308) gx = tx + (tx - gx);       /* other side */
        {
            /* An enemy waiting just off-screen keeps its distance from the
             * nearest player and only moves in when players move. Standing at
             * the screen edge (or standing still anywhere) keeps it waiting:
             * pace back and forth in the part of the screen facing it. */
            int tsx = tx - cam;
            if (PACE_TGT && (tsx < -8 || tsx > 328) && tz > -20 && FW(tgt, O_ST) == 2) {
                pad = pace_for(me, cam, mx, md, tsx, td);
                goto stuck;
            }
        }
        int dd = td - md;
        int rdx = tx - mx, adx = iabs(rdx);
        int ex = gx - mx;

        DBG(9, tx); DBG(10, td); DBG(11, tz); DBG(12, gx); DBG(13, dd); DBG(14, adx); DBG(15, ex);
        if (dd > YTOL) pad |= PAD_D; else if (dd < -YTOL) pad |= PAD_U;

        if (st == 2) {
            int face_r = FB(me, O_FACE) & 1, want_r = rdx > 0;
            int tup = en_upright(FW(tgt, O_ST), tgt);
            /* knife / kunai: throw it at a lined-up target out of reach */
            if (armed && (wtype == IT_KNIFE || wtype == IT_KUNAI) && !M->thr_cd &&
                iabs(dd) <= 3 && adx >= THR_MIN && adx <= THR_MAX && tz > -8) {
                if (face_r != want_r) return want_r ? PAD_R : PAD_L;
                M->thr_cd = 60;
                M->hold = 2; M->hold_pad = PAD_B | PAD_C;
                return PAD_B | PAD_C;
            }
            /* crowded with someone behind us: back attack (unarmed only:
             * with a weapon the same buttons throw it) */
            if (!armed && !M->bk_cd && M->tnc >= BK_CROWD && adx <= ar + 12 && iabs(dd) <= 6 &&
                face_r == want_r && enemy_behind(me, mx, md)) {
                M->bk_cd = 30;
                M->hold = 2; M->hold_pad = PAD_B | PAD_C;
                return PAD_B | PAD_C;
            }
            /* jump kick: close the gap in the air and knock the target down */
            if (!M->jk_cd && tup && iabs(dd) <= JK_DD && adx >= JK_MIN && adx <= JK_MAX &&
                (tz > -8 || (tz < -20 && adx < 60))) {
                M->jk_cd = JK_CD;
                M->jdir = want_r ? PAD_R : PAD_L;
                M->jump_t = JUMP_T; M->jair = 1;
                return M->jdir | PAD_C;
            }
        }

        /* flying target (jetpack): keep our distance, line up, hit it when low */
        if (tz < -20) {
            int hx = tx + (mx < tx ? -52 : 52);
            pad = walk_to(mx, md, hx, td, 6, 3);
            if (adx < 44 && iabs(dd) < 6 && tz > -36)
                pad = face_or_attack(me, rdx, pad, PAD_B);
            goto stuck;
        }

        if (iabs(ex) > XTOL) {
            if (!(iabs(dd) > 8 && adx < ar && ((ex > 0) == (rdx > 0))))
                pad |= ex > 0 ? PAD_R : PAD_L;
        }
        if (iabs(dd) <= 6 && adx <= ar + ATK_WIN) {
            if (adx > ar + 12) {
                M->zone++;
                if (M->zone > ZONEMAX) {
                    if (M->zone > ZONEMAX + 30) M->zone = 0;
                    return (int)((pad & (PAD_U | PAD_D)) | (rdx > 0 ? PAD_R : PAD_L));
                }
            } else {
                M->zone = 0;
            }
            return face_or_attack(me, rdx, pad, PAD_B);
        }
        goto stuck;
    }

    /* ---- nothing to fight ---- */
    if (PACE_IDLE && M->woff) {
        /* enemies waiting just off-screen: draw them in */
        int lo, hi;
        floor_at(mx, &lo, &hi);
        pad = pace_for(me, cam, mx, md, M->woff, (lo + hi) >> 1);
    } else if (M->idle_t) {
        /* the screen will not scroll yet (end of a section, a boss about to
         * appear): wait instead of walking into the wall */
        M->idle_t--;
    } else {
        /* push the screen forward, keeping to the middle of the floor so
         * diagonal walls do not stop us */
        int ax = FW(ally, O_X);
        int ast = FW(ally, O_ST);
        int gx = cam + 400, lo, hi;
        if (ast == 0 || ast >= 0x30) gx = ax;   /* human is out: wait */
        if (gx - mx > 10) pad |= PAD_R;
        else if (gx - mx < -40) pad |= PAD_L;
        floor_at(mx + 32, &lo, &hi);
        lo = (lo + hi) >> 1;
        if (MIDFLOOR) { if (md < lo - 12) pad |= PAD_D; else if (md > lo + 12) pad |= PAD_U; }
    }

stuck:
    /* walking along a diagonal wall: slide along it instead of pushing into it */
    if (SLIDE && (SLIDE == 1 || !tgt) && st == 2 && (pad & (PAD_L | PAD_R))) {
        int lo, hi, lo0, hi0;
        floor_at(mx, &lo0, &hi0);
        floor_at(mx + ((pad & PAD_R) ? 8 : -8), &lo, &hi);
        if (lo > lo0 && md <= lo + 1) pad = (pad & ~PAD_U) | PAD_D;
        else if (hi < hi0 && md >= hi - 1) pad = (pad & ~PAD_D) | PAD_U;
    }
    /* pushing but not moving: give up on that target for a while, or side-step */
    if (pad & (PAD_U | PAD_D | PAD_L | PAD_R)) {
        if (st == 2 && mx == M->lastx && md == M->lasty) {
            if (++M->stuck_n >= 40) {
                int sx = mx - M->stkx, sy = md - M->stky;
                M->stuck_n = 0;
                /* blocked again near where we were last blocked? */
                M->nstk = (iabs(sx) < 12 && iabs(sy) < 12) ? M->nstk + 1 : 1;
                M->stkx = mx; M->stky = md;
                if (M->nstk == 1) {
                    /* first try: side-step around the obstacle */
                    M->dodge_t = 24; M->dblk = 0;
                    M->dodge_dir = (M->tick & 0x40) ? PAD_U : PAD_D;
                    if (pad & (PAD_U | PAD_D)) M->dodge_dir = (pad & PAD_U) ? PAD_D : PAD_U;
                    M->dodge_dir |= pad & (PAD_L | PAD_R);
                } else {
                    /* still stuck: give up on this goal for a while */
                    M->nstk = 0;
                    if (goal) { M->iign = OLOW(goal); M->iign_t = 240; }
                    else if (tgt) { M->ign = OLOW(tgt); M->ign_t = 180; }
                    else M->idle_t = 180;   /* nothing to fight, wall ahead: wait */
                }
            }
        } else M->stuck_n = 0;
    } else M->stuck_n = 0;
    M->lastx = mx; M->lasty = md;
    return pad;
}

/* entry point used by the assembly hook */
u8 ai_think(void) { return (u8)think(); }

void ai_reset(void)
{
    u8 *p = (u8 *)M;
    for (int i = 0; i < (int)sizeof(struct Mem); i++) p[i] = 0;
    M->magic = MEM_MAGIC;
}

/* the special move costs the CPU partner nothing */
int ai_free_special(s32 obj)
{
    return BY(0xFFFFFE10L) != 0 && obj == (s32)0xFFFFF000L;
}
