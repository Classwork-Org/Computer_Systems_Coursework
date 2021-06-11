/* file:        vector.S
 * description: define a memory region we can put 16 8-byte pointers in
 *              (we'll only use 3 of them for now...)
 * Peter Desnoyers, Northeastern Khoury College, 2020
 */
	.global	vector
	.section .vector,"aw",@progbits /* after much Googling... */
vector:	.fill 16,8,0
