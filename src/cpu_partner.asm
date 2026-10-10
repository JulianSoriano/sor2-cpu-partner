; ---------------------------------------------------------------------------
; Streets of Rage 2 (USA) - CPU partner mod, v7
; Assembly glue: menu additions, input hook, player<->player filters.
; The AI itself is C (ai.c), compiled for the 68000 and linked at C_BASE.
; Assembles with vasm (mot syntax):  vasmm68k_mot -Fbin -m68000 -no-opt
; ---------------------------------------------------------------------------

        include "c_syms.inc"

; title screen credit line (row 26, centred; the font has no lower case)
CREDITCMD   equ $4D0E0003

AIFLAG      equ $FFFFFE10       ; byte: 1 = CPU partner mode active (2 = test)
AIPREV      equ $FFFFFE11       ; byte: previous AI pad byte (for edge detect)
SELPREV     equ $FFFFFE12       ; byte: P1 was locked in on char select last frame
STAGESEL    equ $FFFFFE13       ; byte: starting stage chosen on the main menu (0-7)
SAVESP      equ $FFFFFE14       ; long: game stack pointer while the AI runs
AISTACK     equ $FFFFE7A0       ; private stack for the AI (RAM the game never reads; was $FFFF9F00, which stage 6 uses for its scroll table)
LASTVB      equ $FFFFFE18       ; byte: VBlank count ($FC11) at the last AI run
THR         equ $FFFFFE19       ; byte: AI runs still to skip after a dropped frame
        ifnd THROTTLE_N
THROTTLE_N  equ 1
        endif
FINISHV     equ $FFFFFE1A       ; word: VDP H/V counter when the game's main loop finished
        ifnd VSKIP
VSKIP       equ $C8             ; ...on or after this scanline there is no time for the AI
        endif

        org $1F8000

; ----- fixed entry points (patched into the original code) -----------------
        jmp AiHook          ; 1F8000  from $0007CC
        jmp MenuDraw        ; 1F8006  from $006768
        jmp MenuNav         ; 1F800C  from $0067E2
        jmp MenuStart       ; 1F8012  from $006890
        jmp SelectP2        ; 1F8018  from $006A02
        jmp FfHit           ; 1F801E  from $004CD2
        jmp FfGrab          ; 1F8024  from $0052BC
        jmp FfThrown        ; 1F802A  from $004BA6
        jmp MenuFix         ; 1F8030  from $006726
        jmp FfBody          ; 1F8036  from $005162
        jmp SpecTrap        ; 1F803C  TRAP #15 vector (8 special-move routines)
        jmp OptUp           ; 1F8042  from $00F95E
        jmp OptDown         ; 1F8048  from $00F98A
        jmp CamX            ; 1F804E  from $00393E
        jmp CamY            ; 1F8054  from $0038BC
        jmp FrameDone       ; 1F805A  from $001A40
        jmp ContSame        ; 1F8060  from $00B784
        jmp OptNavUp        ; 1F8066  from $00F896
        jmp OptNavDown      ; 1F806C  from $00F8CA
        jmp TitleText       ; 1F8072  from $005C8A
        jmp VbEntry         ; 1F8078  from $0004AA
        jmp VbGate5         ; 1F807E  from $000554 / $0005D4
        jmp VbGateP1        ; 1F8084  from $0004B2
        jmp VbGate54        ; 1F808A  from $0006BE
        jmp VbGate6         ; 1F8090  from $000730 / $000762

; ---------------------------------------------------------------------------
; Per-frame gameplay input hook (replaces move.w $FC08.w,$FC6E.w)
; ---------------------------------------------------------------------------
AiHook:
        move.w  $FFFFFC08.w,$FFFFFC6E.w ; displaced instruction
        cmpi.b  #1,AIFLAG.w             ; (2 = filters only, for testing)
        bne     .out
        tst.b   $FFFFFD10.w             ; attract-mode demo playing?
        bne     .out
        tst.b   $FFFFFC3F.w             ; paused: release P2's pad
        beq.s   .run
        cmpi.w  #$3C,$FFFFF000.w        ; ...unless P2 is entering its name (the
        beq.s   .run                    ;   ending freezes the game for that)
        clr.w   $FFFFFC0A.w
        clr.b   AIPREV.w
        bra     .out
.run:
        movem.l d0-d1/a0-a1,-(sp)           ; C code saves the rest itself
        ; The game dropped a frame since our last run ($FC11 ticks every
        ; VBlank, this hook only runs when the game kept up): give the CPU
        ; time back by holding the last pad for a couple of frames.
        move.b  $FFFFFC11.w,d0
        move.b  d0,d1
        sub.b   LASTVB.w,d1
        move.b  d0,LASTVB.w
        cmpi.b  #1,d1
        bls.s   .nolag
        move.b  #THROTTLE_N,THR.w
.nolag: tst.b   THR.w
        bne.s   .skip
        ; the last frame's game logic ended late: no room for the AI now
        cmpi.b  #VSKIP,FINISHV.w
        bcs.s   .think
        bra.s   .hold
.skip:  subq.b  #1,THR.w
.hold:
        move.b  AIPREV.w,$FFFFFC0A.w
        clr.b   $FFFFFC0B.w
        bra.s   .done
.think:
        ifd BUSY
        move.w  #BUSY,d0
.busy:  dbra    d0,.busy
        endif
        move.l  sp,SAVESP.w
        lea     AISTACK.w,sp
        ifd DEBUGSNAP
        lea     $FFFFEF00.w,a0          ; test build: snapshot what the AI sees
        lea     $FFFF8000.w,a1
        move.w  #$1000/4-1,d0
.cp:    move.l  (a0)+,(a1)+
        dbra    d0,.cp
        endif
        jsr     ai_think
        ifd DEBUGSNAP
        move.b  d0,$FFFFFE1F.w
        endif
        movea.l SAVESP.w,sp
        move.b  AIPREV.w,d1
        not.b   d1
        and.b   d0,d1                   ; newly pressed buttons
        move.b  d0,$FFFFFC0A.w
        move.b  d1,$FFFFFC0B.w
        move.b  d0,AIPREV.w
.done:  movem.l (sp)+,d0-d1/a0-a1
.out:   rts

; special moves: the CPU partner pays no health. The 8 routines' closing
; "subq.w #8,$80(a2)" is now "trap #15 / nop", keeping their rts in place.
SpecTrap:
        tst.b   AIFLAG.w
        beq.s   .pay
        cmpa.l  #$FFFFF000,a2
        bne.s   .pay
        rte
.pay:   subq.w  #8,$80(a2)
        move    sr,-(sp)                ; return with the flags the subq set
        move.b  1(sp),3(sp)
        addq.l  #2,sp
        rte

; Screen scrolling normally waits until BOTH players reach the middle of the
; screen. With the CPU partner in play, the CPU alone can push it forward.
; (replaces tst.w $82(a1) / bmi.b skip, a1 = player 1)
CamSkipP1:                              ; Z=0 -> ignore player 1 for scrolling
        tst.w   $82(a1)
        bmi.s   .skip
        cmpi.b  #1,AIFLAG.w
        bne.s   .keep
        tst.w   $82(a2)                 ; CPU partner must be in play
        bmi.s   .keep
        cmpi.w  #$30,(a2)
        bcc.s   .keep
.skip:  moveq   #1,d1
        rts
.keep:  moveq   #0,d1
        rts
CamX:   bsr.s   CamSkipP1
        beq.s   .chk
        move.l  #$3950,(sp)
        rts
.chk:   move.l  #$3944,(sp)
        rts
CamY:   bsr.s   CamSkipP1
        beq.s   .chk
        move.l  #$38CE,(sp)
        rts
.chk:   move.l  #$38C2,(sp)
        rts

; options LEVEL: always MANIA (10); left/right do nothing
LVMIN       equ 10
OptUp:
OptDown:
        move.w  #LVMIN,$FFFFFD04.w
        rts

; options cursor ($A900 = row): row 1 (PLAYERS, the lives setting) is gone;
; row 5 (ROUND) only exists with the game's cheat ($A902)
OptNavUp:
.l:     subq.w  #1,$FFFFA900.w
        bpl.s   .c
        move.w  #6,$FFFFA900.w
.c:     bsr.s   OptSkip
        beq.s   .l
        rts
OptNavDown:
.l:     addq.w  #1,$FFFFA900.w
        cmpi.w  #7,$FFFFA900.w
        bcs.s   .c
        clr.w   $FFFFA900.w
.c:     bsr.s   OptSkip
        beq.s   .l
        rts
OptSkip:                                ; Z=1 -> this row does not exist
        cmpi.w  #1,$FFFFA900.w
        beq.s   .r
        cmpi.w  #5,$FFFFA900.w
        bne.s   .r                      ; (Z=0)
        tst.w   $FFFFA902.w             ; ROUND with the cheat: Z=0
        bne.s   .y
        cmpi.w  #5,$FFFFA900.w          ; Z=1
.r:     rts
.y:     andi    #$FB,ccr
        rts

; ---------------------------------------------------------------------------
; Title screen: one more line under the copyright notices
; (replaces lea MUSIC(pc),a0 / jsr $1AF2)
TitleText:
        lea     $5D1A.l,a0              ; "MUSIC (C) YUZO KOSHIRO"
        jsr     $1AF2.l
        lea     CreditText(pc),a0
        jmp     $1AF2.l
CreditText:
        dc.l    CREDITCMD
        dc.b    "2P CPU MOD BY RAMENPHOLIFE",0
        even

; ---------------------------------------------------------------------------
; VBlank: when it arrives while the main loop is still busy (the game is
; lagging: $FC10 = 0), do no VDP work at all. The handler's sprite-tile and
; palette uploads otherwise land in the middle of a main-loop VDP write and
; redirect it - the HUD then writes into tile 0, which is the blank tile the
; sky in stage 7's last area and stage 8 are made of (a vanilla race that
; the extra CPU load made frequent). Uploads that are skipped stay flagged
; and happen at the next VBlank. Pads are still read every VBlank.
VBIDLE      equ $FFFFFE1C               ; byte: main loop was waiting at this VBlank
VbEntry:                                ; replaces bsr $19AE / lea $EF00.w,a5
        jsr     $19AE.l                 ; read the pads
        lea     $FFFFEF00.w,a5
        tst.b   $FFFFFC10.w
        sne     VBIDLE.w
        bne.s   .r
        sf      $FFFFFC4B.w             ; busy: also skips the DMA at $191C
.r:     rts
VbGateP1:                               ; replaces btst #5,$1E(a5) (Z=1: skip)
VbGate5:
        tst.b   VBIDLE.w
        beq.s   .r
        btst    #5,$1E(a5)
.r:     rts
VbGate6:
        tst.b   VBIDLE.w
        beq.s   .r
        btst    #6,$1E(a5)
.r:     rts
VbGate54:
        tst.b   VBIDLE.w
        beq.s   .r
        btst    #0,$FFFFFC54.w
.r:     rts

; ---------------------------------------------------------------------------
; Main menu
; ---------------------------------------------------------------------------
MenuDraw:                               ; replaces jsr $1AF2 (draws OPTIONS)
        jsr     $1AF2.l
        lea     MenuText(pc),a0
        jsr     $1AF2.l
        lea     StageText(pc),a0        ; "STAGE n", n drawn right after
        jsr     $1AF2.l
        moveq   #0,d0
        move.b  STAGESEL.w,d0
        andi.w  #7,d0
        addi.w  #'1',d0
        add.w   $FFFFFC12.w,d0
        move.w  d0,$C00000
        rts

MenuText:
        dc.l    $69A00003               ; VRAM write, row below OPTIONS
        dc.b    "CPU PARTNER",0
        even
StageText:
        dc.l    $6BA00003               ; two rows further down
        dc.b    "STAGE ",0
        even

MenuFix:                                ; replaces "if P2 pad absent and
        cmpi.w  #1,$FFFFFC18.w              ;  cursor not on OPTIONS: reset"
        beq.s   .clr
        cmpi.w  #2,$FFFFFC18.w
        bne.s   .ok
.clr:   clr.w   $FFFFFC18.w
.ok:    rts

MenuNav:                                ; P1 up/down on the main menu
        movem.l d0-d1,-(sp)
        move.b  $FFFFFC09.w,d0
        btst    #1,d0
        beq.s   .up
        bsr.w   MenuSfx
        move.w  $FFFFFC18.w,d1
        addq.w  #1,d1
        cmpi.w  #5,d1
        bcs.s   .d1
        moveq   #0,d1
.d1:    tst.b   $FFFFFC6D.w                 ; no 2nd pad: skip 2P and DUEL
        beq.s   .dset
        cmpi.w  #1,d1
        beq.s   .d3
        cmpi.w  #2,d1
        bne.s   .dset
.d3:    moveq   #3,d1
.dset:  move.w  d1,$FFFFFC18.w
.up:    btst    #0,d0
        beq.s   .done
        bsr.w   MenuSfx
        move.w  $FFFFFC18.w,d1
        subq.w  #1,d1
        bpl.s   .u1
        moveq   #4,d1
.u1:    tst.b   $FFFFFC6D.w
        beq.s   .uset
        cmpi.w  #1,d1
        beq.s   .u0
        cmpi.w  #2,d1
        bne.s   .uset
.u0:    moveq   #0,d1
.uset:  move.w  d1,$FFFFFC18.w
.done:  ; left / right: starting stage
        btst    #2,d0
        beq.s   .nl
        bsr.w   MenuSfx
        subq.b  #1,STAGESEL.w
        bpl.s   .nl
        move.b  #7,STAGESEL.w
.nl:    btst    #3,d0
        beq.s   .nr
        bsr.w   MenuSfx
        addq.b  #1,STAGESEL.w
        cmpi.b  #8,STAGESEL.w
        bcs.s   .nr
        clr.b   STAGESEL.w
.nr:    movem.l (sp)+,d0-d1
        rts

MenuSfx:
        movem.l d0-d7/a0-a6,-(sp)
        move.l  #$437,d0
        moveq   #0,d1
        jsr     $40008.l
        movem.l (sp)+,d0-d7/a0-a6
        rts

MenuStart:                              ; replaces cmpi.w #3,$FFFFFC18.w
        clr.b   AIFLAG.w
        clr.b   AIPREV.w
        clr.b   SELPREV.w
        movem.l d0-d7/a0-a6,-(sp)
        jsr     ai_reset
        movem.l (sp)+,d0-d7/a0-a6
        moveq   #0,d0                   ; starting stage from the menu
        move.b  STAGESEL.w,d0
        andi.w  #7,d0
        move.w  d0,$FFFFFD0E.w
        move.w  #LVMIN,$FFFFFD04.w      ; always MANIA
    cmpi.b  #1,$FFFFFD11.w          ; our "same character" marker from a
        bne.s   .nm                     ;   CPU game ($FF = the game's own cheat)
        sf      $FFFFFD11.w
.nm:    cmpi.w  #4,$FFFFFC18.w
        bne.s   .x
        move.b  #1,AIFLAG.w
        move.w  #1,$FFFFFC18.w              ; play as a normal 2-player game
        tst.b   $FFFFFD11.w             ; CPU game: you and the CPU may pick
        bne.s   .x                      ;   the same character
        move.b  #1,$FFFFFD11.w
.x:     cmpi.w  #3,$FFFFFC18.w
        rts

; ---------------------------------------------------------------------------
; Character select: once P1 has locked in, P1's pad picks the partner.
; replaces: move.b $FFFFFC0B.w,d7 / bsr.b $6A0C
; ---------------------------------------------------------------------------
SelectP2:
        move.b  $FFFFFC0B.w,d7
        tst.b   AIFLAG.w
        beq.s   .call
        moveq   #0,d7
        tst.b   $FFFFEF3A.w
        beq.s   .clr
        tst.b   SELPREV.w
        beq.s   .set
        move.b  $FFFFFC09.w,d7
.set:   st      SELPREV.w
        bra.s   .call
.clr:   sf      SELPREV.w
.call:  jmp     $6A0C.l

; ---------------------------------------------------------------------------
; Player <-> player interaction filters (only while AIFLAG is set)
; a2 = acting object, a3 = candidate object
; ---------------------------------------------------------------------------
; Z=1 -> skip this candidate
FfCheck:
        cmpa.l  a2,a3
        beq.s   .r                      ; self: skip (Z=1)
        cmpa.w  #$F000,a3               ; an enemy or object is involved:
        bhi.s   .r                      ;   keep (bhi taken => Z=0)
        cmpa.w  #$F000,a2
        bhi.s   .r
        tst.b   AIFLAG.w                ; player vs player: skip in CPU mode
        beq.s   .keep
        tst.b   $FFFFFD10.w             ; attract-mode demo: vanilla rules
        bne.s   .keep
        move    #4,ccr
.r:     rts
.keep:  move    #0,ccr
        rts

FfHit:                                  ; attack hit-test loop
        bsr.s   FfCheck
        bne.s   .ok
        move.l  #$4E8E,(sp)
.ok:    rts

FfGrab:                                 ; walk-into-grab loop
        bsr.s   FfCheck
        bne.s   .ok
        move.l  #$5348,(sp)
.ok:    rts

FfThrown:                               ; thrown body / object hit loop
        cmpa.l  $34(a2),a3
        beq.s   .skip
        tst.b   AIFLAG.w
        beq.s   .ok
        tst.b   $FFFFFD10.w
        bne.s   .ok
        cmpa.l  #$FFFFF000,a3
        bhi.s   .ok
        move.l  d0,-(sp)
        move.l  $34(a2),d0
        cmpi.l  #$FFFFEF00,d0
        bcs.s   .ok2
        cmpi.l  #$FFFFF000,d0
        bhi.s   .ok2
        move.l  (sp)+,d0
.skip:  move.l  #$4CB4,(sp)
        rts
.ok2:   move.l  (sp)+,d0
.ok:    rts

FfBody:                                 ; thrown enemy body hitting others
        cmpa.l  $8A(a2),a3              ; never hits its own thrower
        beq.s   .skip
        tst.b   AIFLAG.w
        beq.s   .ok
        tst.b   $FFFFFD10.w
        bne.s   .ok
        cmpa.l  #$FFFFF000,a3
        bhi.s   .ok
        move.l  d0,-(sp)
        move.l  $8A(a2),d0              ; thrown by a player?
        cmpi.l  #$FFFFEF00,d0
        bcs.s   .ok2
        cmpi.l  #$FFFFF000,d0
        bhi.s   .ok2
        move.l  (sp)+,d0
.skip:  move.l  #$5210,(sp)
        rts
.ok2:   move.l  (sp)+,d0
.ok:    rts

; ---------------------------------------------------------------------------
; The main loop has finished a frame and is about to wait for VBlank:
; remember how far down the screen the beam is, i.e. how busy the game is.
; (replaces move.b #3,$FC10.w; interrupts are masked here)
; ---------------------------------------------------------------------------
FrameDone:
        move.b  #3,$FFFFFC10.w
        move.w  $C00008,FINISHV.w
        rts

; ---------------------------------------------------------------------------
; Continuing player's character: the game bumps it to the next one when it
; matches the other player's. Skip that when duplicates are allowed ($FD11).
; replaces cmpi.w #$36,$0(a1); the following bcc skips the bump when C=0
; ---------------------------------------------------------------------------
ContSame:
        tst.b   $FFFFFD11.w
        beq.s   .orig
        move    #0,ccr
        rts
.orig:  cmpi.w  #$36,(a1)
        rts
