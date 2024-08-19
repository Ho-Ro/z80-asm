
	ORG $0

reset:
	nop
	ld a, $55
	ld c, $AA
	out (c), a
	out (c), 0
	defb $ed, $71 ; out (c), 0 (undocumented)
	in a, (c)
	in (c)
	defb $ed, $70 ; in (c) (undocumented)
	ld hl, reset
	ld (hl), $76
	jp reset
