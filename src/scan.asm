; ---------------------------------------------------------------------------
; Hand-written 68000 versions of the AI's two per-object scans.
; Behaviour is defined by ai_threats() / ai_select() in ai.c; the native test
; build runs those C versions and difftest.py checks this code against them.
; Called from vbcc code: arguments are longs on the stack, d0-d1/a0-a1 scratch.
; ---------------------------------------------------------------------------
        include "mem.inc"

ABSW    macro                           ; \2 = |\1| (16-bit, read as unsigned)
        move.w  \1,\2
        bpl.s   .abs\@
        neg.w   \2
.abs\@:
        endm

; void ai_threats(long mx, long md)
; nearest fast mover / attacking enemy / thrown object heading at us
; -> M.tdx, M.tdd (dx, depth difference; tdx = $7FFF if none), M.tnc (crowd)
_ai_threats:
        movem.l d2-d7/a2-a4,-(sp)
.MX     equ     36+4+2
.MD     equ     36+8+2
        move.b  M_tick.w,d0
        move.b  d0,d1
        sub.b   M_vt.w,d1
        move.b  d0,M_vt.w
        suba.l  a3,a3                   ; a3: velocities valid (VDT frames apart)
        cmpi.b  #VDT,d1
        bne.s   .nv
        addq.w  #1,a3
.nv:    move.w  #$7FFF,d2               ; d2 danger dx
        moveq   #0,d3                   ; d3 danger depth
        moveq   #0,d4                   ; d4 crowd count
        movea.w #$7FFF,a2               ; a2 |danger dx|
        lea     $FFFFF100.w,a0
        lea     M_ex.w,a1
.el:    move.w  2(a0),d0                ; x
        move.w  d0,d1
        sub.w   (a1),d1                 ; d1 vx
        move.w  d0,(a1)+
        move.w  (a0),d5                 ; state
        beq     .enx
        cmpi.w  #6,d5
        beq     .enx
        cmpi.w  #8,d5
        beq     .enx
        cmpi.w  #10,d5
        beq     .enx
        tst.w   $80(a0)                 ; hp > 0
        ble     .enx
        sub.w   .MX(sp),d0              ; d0 dx
        ABSW    d0,d7                   ; d7 |dx|
        cmpi.w  #120,d7                 ; every test below needs |dx| < 120
        bcc     .enx
        move.w  6(a0),d6
        sub.w   $5E(a0),d6
        sub.w   .MD(sp),d6              ; d6 dd
        ABSW    d6,d5                   ; d5 |dd|
        cmpi.w  #14,d5                  ; ...and |dd| < 14
        bcc     .enx
        cmpi.w  #40,d7                  ; crowding us?
        bcc.s   .nc
        cmpi.w  #12,d5
        bcc.s   .nc
        tst.b   $49(a0)
        bpl.s   .nc
        addq.w  #1,d4
.nc:    cmpi.w  #24,(a0)                ; mid-attack (state 24..59 but not 36/38),
        blt.s   .nat                    ;   facing us, in reach?
        cmpi.w  #60,(a0)
        bge.s   .nat
        cmpi.w  #36,(a0)
        beq.s   .nat
        cmpi.w  #38,(a0)
        beq.s   .nat
        btst    #0,$0F(a0)
        beq.s   .fl
        tst.w   d0                      ; facing right: it is left of us (dx < 0)
        bpl.s   .nat
        bra.s   .fu
.fl:    tst.w   d0                      ; facing left: dx > 0
        ble.s   .nat
.fu:    cmpi.w  #12,d5
        bcc.s   .nat
        cmpi.w  #24,$0C(a0)             ; jet: long reach
        bne.s   .r72
        cmpi.w  #112,d7
        bcc.s   .nat
        bra.s   .rok
.r72:   cmpi.w  #72,d7
        bcc.s   .nat
.rok:   cmp.w   a2,d7
        bcc.s   .nat
        move.w  d0,d2
        move.w  d6,d3
        movea.w d7,a2
.nat:   cmpa.w  #0,a3                   ; fast mover heading at us?
        beq.s   .enx
        cmpi.w  #14,d5
        bcc.s   .enx
        ABSW    d1,d5                   ; d5 |vx|
        cmpi.w  #VMAX,d5
        bhi.s   .enx
        cmpi.w  #VMIN,d5
        bcs.s   .enx
        cmpi.w  #-40,$5E(a0)
        ble.s   .enx
        tst.w   d1
        bmi.s   .vn
        tst.w   d0                      ; moving right: must be left of us
        bpl.s   .enx
        bra.s   .vd
.vn:    tst.w   d0                      ; moving left: must be right of us
        ble.s   .enx
.vd:    cmpi.w  #120,d7
        bcc.s   .enx
        cmp.w   a2,d7
        bcc.s   .enx
        move.w  d0,d2
        move.w  d6,d3
        movea.w d7,a2
.enx:   lea     $100(a0),a0
        cmpa.w  #$F700,a0
        bne     .el

        lea     $FFFFF700.w,a0          ; thrown objects
        lea     M_ix.w,a1
.il:    move.w  (a0),d5
        beq.s   .inx
        move.w  2(a0),d0
        move.w  d0,d1
        sub.w   (a1),d1                 ; vx
        move.w  d0,(a1)
        cmpi.w  #2,d5
        beq.s   .inx
        tst.b   $32(a0)
        bpl.s   .inx
        cmpa.w  #0,a3
        beq.s   .inx
        ABSW    d1,d5
        cmpi.w  #VMIN,d5
        bcs.s   .inx
        cmpi.w  #VMAX,d5
        bhi.s   .inx
        sub.w   .MX(sp),d0              ; dx
        move.w  6(a0),d6
        sub.w   .MD(sp),d6              ; dd
        ABSW    d6,d5
        cmpi.w  #16,d5
        bcc.s   .inx
        tst.w   d1
        bmi.s   .ivn
        tst.w   d0
        bpl.s   .inx
        bra.s   .ivd
.ivn:   tst.w   d0
        ble.s   .inx
.ivd:   ABSW    d0,d7
        cmpi.w  #120,d7
        bcc.s   .inx
        cmp.w   a2,d7
        bcc.s   .inx
        move.w  d0,d2
        move.w  d6,d3
        movea.w d7,a2
.inx:   addq.w  #2,a1
        lea     $80(a0),a0
        cmpa.w  #$FA00,a0
        bne     .il

        move.w  d2,M_tdx.w
        move.w  d3,M_tdd.w
        move.b  d4,M_tnc.w
        movem.l (sp)+,d2-d7/a2-a4
        rts

; void ai_select(long cam, long mx, long md, long hp)
; nearest enemy worth fighting -> M.ctgt; best pickup -> M.citem, M.urgent
_ai_select:
        movem.l d2-d7/a2-a4,-(sp)
.CAM    equ     36+4+2
.MX     equ     36+8+2
.MD     equ     36+12+2
.HP     equ     36+16+2
        move.w  .CAM(sp),d6             ; d6 camera x
        suba.l  a2,a2                   ; a2 target
        move.w  #$7FFF,d2               ; d2 best score
        lea     $FFFFF100.w,a0
.tl:    tst.w   (a0)
        beq.s   .tnx
        tst.b   $49(a0)                 ; hittable
        bpl.s   .tnx
        tst.w   $50(a0)                 ; not invulnerable
        bne.s   .tnx
        tst.w   $80(a0)
        ble.s   .tnx
        tst.b   M_ign_t.w               ; the one we gave up on
        beq.s   .ti
        move.w  a0,d0
        cmp.w   M_ign.w,d0
        beq.s   .tnx
.ti:    move.w  2(a0),d0
        move.w  d0,d1
        sub.w   d6,d1                   ; d1 screen x
        cmpi.w  #-48,d1
        blt.s   .tnx
        cmpi.w  #368,d1
        bgt.s   .tnx
        sub.w   .MX(sp),d0
        ABSW    d0,d3
        move.w  6(a0),d0
        sub.w   $5E(a0),d0
        sub.w   .MD(sp),d0
        ABSW    d0,d4
        add.w   d4,d4
        add.w   d4,d3                   ; d3 = |dx| + 2|dd|
        cmpi.w  #4,d1
        blt.s   .tp
        cmpi.w  #316,d1
        ble.s   .tq
.tp:    addi.w  #64,d3                  ; near the screen edge
.tq:    cmp.w   d2,d3
        bge.s   .tnx
        move.w  d3,d2
        movea.l a0,a2
.tnx:   lea     $100(a0),a0
        cmpa.w  #$F700,a0
        bne.s   .tl

        suba.l  a3,a3                   ; a3 item
        move.w  #$7FFF,d2
        moveq   #0,d5                   ; d5 urgent
        move.w  .HP(sp),d7
.il:    cmpi.w  #2,(a0)
        bne     .inx
        move.b  $32(a0),d0
        andi.b  #$C1,d0
        cmpi.b  #$80,d0                 ; a pickup, not a weapon or crate
        bne     .inx
        move.w  $0C(a0),d0
        moveq   #0,d4
        cmpi.w  #140,d0                 ; money
        beq.s   .iw
        cmpi.w  #142,d0                 ; gold
        beq.s   .iw
        cmpi.w  #144,d0                 ; 1UP: always, urgently
        bne.s   .ia
        moveq   #1,d4
        bra.s   .iw
.ia:    cmpi.w  #146,d0                 ; apple when below half health
        bne.s   .it
        cmpi.w  #52,d7
        bge.s   .inx
        moveq   #1,d4
        bra.s   .iw
.it:    cmpi.w  #148,d0                 ; turkey when below a quarter
        bne.s   .inx
        cmpi.w  #26,d7
        bge.s   .inx
        moveq   #1,d4
.iw:    move.w  2(a0),d0
        move.w  d0,d1
        sub.w   d6,d1
        blt.s   .inx
        cmpi.w  #320,d1
        bgt.s   .inx
        sub.w   .MX(sp),d0
        ABSW    d0,d3
        move.w  6(a0),d0
        sub.w   .MD(sp),d0
        ABSW    d0,d1
        add.w   d1,d1
        add.w   d1,d3
        tst.b   d4
        beq.s   .iu
        subi.w  #400,d3
.iu:    cmp.w   d2,d3
        bge.s   .inx
        move.w  d3,d2
        movea.l a0,a3
        move.b  d4,d5
.inx:   lea     $80(a0),a0
        cmpa.w  #$FA00,a0
        bne     .il

        move.w  a2,M_ctgt.w
        move.w  a3,M_citem.w
        move.b  d5,M_urgent.w
        movem.l (sp)+,d2-d7/a2-a4
        rts
