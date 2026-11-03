; v9: a last line of defence against twitching.  Whatever the AI decides, a left<->right (or up<->down) reversal
; is only let through if the previous direction has been held for at least 8 frames; before that she keeps going
; the way she was going.  One-frame flip-flops (a goal or an enemy flickering from one side to the other) turn into
; a smooth walk; a real change of mind still happens, 8 frames (about 0.13 s) late.
; Only during play: not on the continue / character / name-entry screens, which press left and right deliberately.
; The jump stub at $1F9002 (ai_think) now points here; the real brain is still at $1FA8A0.
;   $FFFEA2 last horizontal direction (0, 4 = L, 8 = R)    $FFFEA3 frames it has been held
;   $FFFEA4 last vertical direction   (0, 1 = U, 2 = D)    $FFFEA5 frames it has been held
	org	$1FAD00
	jsr	$1FA8A0			; the brain: d0.b = pad
	cmpi.w	#$34,$FFFFF000.w	; partner's state: 0x34 and up are the menus
	bcc.s	.done
	move.b	d0,d1
	andi.b	#$0C,d1			; horizontal bits
	lea	$FFFFFEA2.w,a0
	bsr.s	.axis
	andi.b	#$F3,d0
	or.b	d1,d0
	move.b	d0,d1
	andi.b	#$03,d1			; vertical bits
	lea	$FFFFFEA4.w,a0
	bsr.s	.axis
	andi.b	#$FC,d0
	or.b	d1,d0
.done:
	rts
; d1 = the direction wanted on this axis, (a0) = last direction, 1(a0) = frames held.  Returns d1 = direction to use.
.axis:
	cmpi.b	#255,1(a0)
	beq.s	.nocount
	addq.b	#1,1(a0)
.nocount:
	tst.b	d1
	beq.s	.out			; not pressing this axis: nothing to do
	cmp.b	(a0),d1
	beq.s	.out			; the same direction as before
	tst.b	(a0)
	beq.s	.take			; first direction after standing still
	cmpi.b	#8,1(a0)
	bcs.s	.keep			; reversed too soon: keep the old direction
.take:
	move.b	d1,(a0)
	clr.b	1(a0)
.out:
	rts
.keep:
	move.b	(a0),d1
	rts
