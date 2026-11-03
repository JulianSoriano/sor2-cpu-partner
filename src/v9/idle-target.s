; v9: where the partner heads when there is nothing to fight (replaces ai.c's  gx = cam + 400).
; Runs in place of 14 bytes at $1FA5F0 (a jmp there); the stack frame is the caller's, so offsets are the same.
;   cam      = 60(a7)    camera x
;   ax       = 116(a7)   the human player's x
;   gx       = 124(a7)   goal x (written here)
; gx = min(cam + 230, ax + 150): head for the middle-right of the screen (not the edge) and never get further
; ahead of the human than 150, so the partner stops pushing into the side of the screen and stops dragging the
; camera away from the human.
	org	$1FAC00
	move.l	60(a7),d0
	add.l	#230,d0
	move.l	116(a7),d1
	add.l	#150,d1
	cmp.l	d1,d0
	ble.s	.ok
	move.l	d1,d0
.ok:
	move.l	d0,124(a7)
	jmp	$1FA5FE
