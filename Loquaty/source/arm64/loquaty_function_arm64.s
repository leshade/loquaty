
	.global	PrepareFPRegisterToCallArm64

	.text
	.align	16

PrepareFPRegisterToCallArm64:

	cmp		w1, 1
	b.lo	LabelExit
	ldr		d0, [x0]

	cmp		w1, 2
	b.lo	LabelExit
	ldr		d1, [x0, #8]

	cmp		w1, 3
	b.lo	LabelExit
	ldr		d2, [x0, #16]

	cmp		w1, 4
	b.lo	LabelExit
	ldr		d3, [x0, #24]

	cmp		w1, 5
	b.lo	LabelExit
	ldr		d4, [x0, #32]

	cmp		w1, 6
	b.lo	LabelExit
	ldr		d5, [x0, #40]

	cmp		w1, 7
	b.lo	LabelExit
	ldr		d6, [x0, #48]

	cmp		w1, 8
	b.lo	LabelExit
	ldr		d7, [x0, #56]

LabelExit:
	ret


