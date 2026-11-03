; v9: which side of its target the partner stands on.  ai.c:  gx = tx + (mx < tx ? -ar : ar)
; When she is level with the enemy that test flips every frame and she steps left and right (the "spaz").
; Now the side only changes once she is more than 16 pixels past him; inside that band she keeps the last side.
; Replaces  cmp.l 116(a7),d4 / bge.s $1FA15C  at $1FA14E.   116(a7) = tx, d4 = mx.  Flag byte at $FFFEA0 (free RAM).
	org	$1FAC40
	move.l	d4,d0
	sub.l	116(a7),d0
	cmp.l	#16,d0
	bgt.s	.right
	cmp.l	#-16,d0
	blt.s	.left
	tst.b	$FFFFEA0.w
	bne.s	.right
.left:
	clr.b	$FFFFEA0.w
	jmp	$1FA154
.right:
	move.b	#1,$FFFFEA0.w
	jmp	$1FA15C
