/*
 *  file:        call-vector.s
 *  description: defines symbols for OS trap vectors
 *
 *    CS5600, Computer Systems, Northeastern Khoury College
 *    Peter Desnoyers, 2019
 */

/*
 * Note that these indexes must match those used in setting up the
 * vector table in part-2.c
 */

	.text

		_readline = 0x05000000 + (0 * 8)
		_print    = 0x05000000 + (1 * 8)
		_getarg   = 0x05000000 + (2 * 8)
		_yield12  = 0x05000000 + (3 * 8)
		_yield21  = 0x05000000 + (4 * 8)
		_uexit    = 0x05000000 + (5 * 8)

	.align	16
readline:	movq	_readline,%rax
		jmp 	*%rax

	.align	16
print:		movq	_print,%rax
		jmp 	*%rax

	.align	16
getarg:		movq	_getarg,%rax
		jmp 	*%rax

	.align	16
yield12:	movq	_yield12,%rax
		jmp	*%rax

	.align	16
yield21:	movq	_yield21,%rax
		jmp	*%rax

	.align	16
uexit:		movq	_uexit,%rax
		jmp	*%rax
	
/*
 * Export them as global symbols:
 */

        .globl readline
	.type readline,@function
	.globl print
	.type print,@function
	.globl getarg
	.type getarg,@function
	.globl yield12
	.type yield12,@function
	.globl yield21
	.type yield21,@function
	.globl uexit
	.type uexit,@function
