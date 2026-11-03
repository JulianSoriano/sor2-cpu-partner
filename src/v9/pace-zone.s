; v9: pace_for(), the first stage (stand back from the screen edge facing the enemy that is waiting off-screen).
; ai.c:  if (abs(hx - mx) > 6) walk toward hx;  else turn to face the enemy.
; Turning is a step of 2 px, which pushed her out of the 6 px zone, so she walked back, turned away, and so on
; every frame. Now, once she is facing the enemy the "close enough" zone is 24 px, so the turn sticks.
; Replaces  moveq #6,d1 / cmp.l d0,d1 / bge.s $1F9286  at $1F9264.   d0 = abs(hx - mx), d5 = enemy's screen x,
; a2 = the partner's object (face flag: bit 0 of byte 15, 1 = facing right).
	org	$1FAC80
	moveq	#6,d1
	btst	#0,15(a2)
	beq.s	.faceL
	tst.l	d5
	ble.s	.test			; facing right but the enemy is on the left
	bra.s	.near
.faceL:
	tst.l	d5
	bgt.s	.test			; facing left but the enemy is on the right
.near:
	moveq	#24,d1
.test:
	cmp.l	d0,d1
	blt.s	.far
	jmp	$1F9286
.far:
	jmp	$1F926A
