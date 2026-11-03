; v9: breaking containers (barrels, bar tables and chairs).  ai.c:  if (abs(e1) <= 5 && abs(e2) <= 3) attack
; where e1 = how far she is from her chosen spot 30 px beside the container.  A bar table is wider than that:
; it blocks her about 38 px away, so she stood pressing into it, 8 px short of her spot, and never swung.
; Now, if she did not move last frame (she is blocked by the thing) she attacks from up to 24 px short of her spot.
; Replaces  moveq #5,d1 / cmp.l d0,d1 / blt.s $1FA062  at $1FA028.   d0 = abs(e1), d4 = her x,
; $FFFE38 = M->lastx (her x on the previous frame).
	org	$1FACC0
	moveq	#5,d1
	cmp.l	d0,d1
	bge.s	.ok			; close enough
	moveq	#24,d1
	cmp.l	d0,d1
	blt.s	.far			; too far
	cmp.w	$FFFFFE38.w,d4
	bne.s	.far			; still walking: keep walking
.ok:
	jmp	$1FA02E
.far:
	jmp	$1FA062
