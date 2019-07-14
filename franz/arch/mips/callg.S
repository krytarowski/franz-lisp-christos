/* By David Holland */

#include <mips/regdef.h>

	.set noreorder
	.abicalls
	.text
	.globl callg
	.type callg,@function
	.ent callg
callg:
	/*
	 * On entry:
	 *
	 * a0 contains the function to call
	 * a1 contains the pointer to the args
	 * a2 contains the number of args
	 * a3 contains nothing
	 * v0, v1, t0-t9 can be destroyed
	 */

	move t9, a0		# move function pointer to t9 for ELF ABI
	move t8, a1		# put arg base pointer in t8

	/*
	 * Now:
	 *
	 * t9 contains the function to call
	 * t8 contains the pointer to the args
	 * a2 contains the number of args
	 * v0-1, a0-1, a3, t0-t7 can be destroyed
	 */

	sltiu t0, a2, 1	# check if 0 args
	bnez t0, 0f
	sltiu t1, a2, 2	# check if 1 arg
	bnez t1, 1f
	sltiu t2, a2, 3	# check if 2 args
	bnez t2, 2f
	sltiu t3, a2, 4	# check if 3 args
	bnez t3, 3f
	sltiu t4, a2, 5	# check if 4 args
	bnez t4, 4f

	move t7, a2		# put arg count in t7
	addu t6, t8, t6	# put current arg pointer in t6
	li t4, 4		# 4 is the number of args that go in registers

	/* base frame size: 16 + 8 */
	.frame s8,24,ra
	.mask 0xc0000000,-4
	.fmask 0,0
	addiu sp, sp, -24	# create stack frame
	sw ra, 20(sp)	# save return address
	sw s8, 16(sp)	# save old frame pointer
	move s8, sp		# establish frame pointer

	/*
	 * Now:
	 *
	 * we have a stack frame
	 * t9 contains the function to call
	 * t8 contains the argument base pointer
	 * t7 contains the argument count
	 * t6 contains the current argument pointer
	 * t4 contains the loop-termination value for t6
	 */

	andi v0, t7, 1	# set v0 to 1 if argument count is odd, else 0
	sll v0, v0, 2	# set v0 to 4 if argument count is odd, else 0
	subu sp, sp, v0	# ensure 8-byte alignment of stack

5:
	addiu t6, t6, -4	# address of arg to work on next
	lw v0, 0(t6)		# fetch it
	addiu t7, t7, -1	# number of arg to work on next
	addiu sp, sp, -4	# make slot on stack
	bne t7, t4, 5b	# if not down to 4 args, continue loop
	sw v0, 0(sp)		# store the arg (in delay slot)

	/*
	 * Now:
	 *
	 * we have a stack frame
	 * args 5+ have been copied
	 * t9 contains the function to call
	 * t8 contains the argument base pointer
	 */
	lw a3, 12(t8)	# 4th argument 
	lw a2, 8(t8)		# 3rd argument
	lw a1, 4(t8)		# 2nd argument
	jalr t9, ra		# go!
	lw a0, 0(t8)		# 1st argument (in delay slot)

	/*
	 * Now:
	 *
	 * we have a stack frame
	 * call has been made
	 * v0/v1 contain return values
	 */
	move sp, s8		# pop variable-sized portion of frame
	lw s8, 16(sp)	# restore s8
	lw ra, 20(sp)	# restore ra
	j ra			# return to caller
	addiu sp, sp, -24	# pop base portion of frame

	/*
	 * Here:
	 *
	 * we have no stack frame
	 * t9 contains the function to call
	 * t8 contains the pointer to the args
	 */
4:
	lw a3, 12(t8)	# 4th arg
3:
	lw a2, 8(t8)		# 3rd arg
2:
	lw a1, 4(t8)		# 2nd arg
1:
	jr t9		# go!
	lw a0, 0(t8)		# 1st arg (in delay slot)
0:
	jr t9		# go!
	nop			# delay slot

	.end callg
	.size callg, .-callg
