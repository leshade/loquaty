
	.global	PrepareFPRegisterToCallx64

	.intel_syntax noprefix
	.text
	.align	16

PrepareFPRegisterToCallx64:

	cmp	rdx, 1
	jb	LabelExit
	movlps	xmm0, QWORD PTR [rcx]

	cmp	rdx, 2
	jb	LabelExit
	movlps	xmm1, QWORD PTR [rcx+8]

	cmp	rdx, 3
	jb	LabelExit
	movlps	xmm2, QWORD PTR [rcx+8*2]

	cmp	rdx, 4
	jb	LabelExit
	movlps	xmm3, QWORD PTR [rcx+8*3]

LabelExit:
	ret


