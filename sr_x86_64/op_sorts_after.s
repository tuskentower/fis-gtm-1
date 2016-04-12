#################################################################
#								#
# Copyright (c) 2007-2015 Fidelity National Information 	#
# Services, Inc. and/or its subsidiaries. All rights reserved.	#
#								#
#	This source code contains the intellectual property	#
#	of its copyright holder(s), and is made available	#
#	under a license.  If you do not know the terms of	#
#	the license, please stop and do not read further.	#
#								#
#################################################################

	.include "g_msf.si"
	.include "linkage.si"
	.include "debug.si"
#
# op_sorts_after(mval *mval1, *mval2)
#
# Call sorts_after() to determine whether mval1 comes after mval2
# in sorting order.  Use alternate local collation sequence if
# present.
#
# entry:
#   rax	mval *mval1
#   rdx	mval *mval2
#
# Sets condition flags and returns in eax:
#   1	mval1 > mval2
#   0	mval1 = mval2
#   -1	mval1 < mval2
#
	.text
	.extern	_sorts_after

ENTRY	_op_sorts_after
	subq	$8, %rsp				# Bump stack for 16 byte alignment
	CHKSTKALIGN					# Verify stack alignment
	movq	%r10, %rsi
	movq	%rax, %rdi
	call	_sorts_after
	addq	$8, %rsp				# Remove stack alignment bump
	cmpl	$0, %eax				# Set flags according to result from
	ret
