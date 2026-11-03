; v9e: a floating "2P" tag over the CPU partner, so she can be told apart from the human whatever she looks like.
; Drawn with the HUD's own font tiles ("2" = tile $792, "P" = tile $7B0, the same tiles the health bar uses), as two
; extra sprites added to the end of the sprite list after the game has built it for the frame.
; Only when the CPU partner mode is on ($FFFE10 non-zero) and she is on the screen, and only if the sprite list
; has room (the Mega Drive shows 80 per frame; the tag needs 2).
;   Her position on screen is worked out the way the game does it for its own sprites, but from her position only,
;   never from the current animation frame, so the tag does not move when her pose changes (her Y already includes
;   jumps, so it follows her up and down):
;     x = X + $80 + scroll ($FFFC22)      y = Y + $80 - $FFFC64 - $FFFC26
;   the tag sits a fixed 92 px above her feet, centred over her.
; Hook: at $001A1A  jsr $020BDC (build the sprite list)  becomes  jsr $1FAF00; this code calls the builder first.
	org	$1FAF00
	jsr	$20BDC			; what the original call did: build the sprite list
	tst.b	$FFFFFE10.w		; CPU partner mode?
	beq.w	.done
	cmpi.w	#$34,$FFFFF000.w	; her state: 0 = not there, $34 and up = the menus (the character select
	bcc.w	.done			;   screen and so on), where a tag would only get in the way
	tst.w	$FFFFF000.w
	beq.w	.done
	movem.l	d0-d4/a3,-(a7)
	move.w	$FFFFFC14.w,d0		; sprites so far
	cmpi.w	#78,d0			; room for 2 more?
	bcc.s	.out
	move.w	$FFFFF006.w,d1		; her Y
	addi.w	#$80,d1
	sub.w	$FFFFFC64.w,d1
	sub.w	$FFFFFC26.w,d1
	subi.w	#92,d1			; a fixed 92 px above her feet: just over her head
	cmpi.w	#$88,d1
	bcs.s	.out
	cmpi.w	#$160,d1
	bcc.s	.out
	move.w	$FFFFF002.w,d2		; her X
	addi.w	#$80,d2
	add.w	$FFFFFC22.w,d2
	subq.w	#8,d2			; the tag is 16 px wide: centre it on her
	cmpi.w	#$88,d2
	bcs.s	.out
	cmpi.w	#$1B0,d2
	bcc.s	.out
	lea	$FFFFEC80.w,a3
	move.w	d0,d3
	lsl.w	#3,d3
	adda.w	d3,a3			; the next free sprite slot
	move.w	d0,d4
	addq.w	#1,d4			; link to the slot after it
	move.w	d1,(a3)+		; "2": y
	move.w	d4,(a3)+		;      size 8x8, link
	move.w	#$8792,(a3)+		;      tile $792, in front of the scenery
	move.w	d2,(a3)+		;      x
	addq.w	#1,d4
	move.w	d1,(a3)+		; "P"
	move.w	d4,(a3)+
	move.w	#$87B0,(a3)+
	addq.w	#8,d2
	move.w	d2,(a3)+
	addq.w	#2,$FFFFFC14.w		; two more sprites; the game ends the list after its count
.out:	movem.l	(a7)+,d0-d4/a3
.done:	rts
