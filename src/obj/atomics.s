	.global replace_atomic
replace_atomic:
	LDREX	r2,[r0]
	STREX	r3,r1,[r0]
	CMP	r3,#0
	BNE	replace_atomic
	MOV	r0,r2
	MOV	pc,r14

	.global set_bit_atomic
set_bit_atomic:
	MOV	r2,#1
	LSL	r2,r2,r1
try_again:
	LDREX	r1,[r0]
	ORR	r1,r1,r2
	STREX	r3,r1,[r0]
	CMP	r3,#0
	BNE	try_again
	MOV	pc,r14
