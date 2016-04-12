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

	.include "linkage.si"
	.include "g_msf.si"
	.include "debug.si"

	.data
	.extern	_frame_pointer

	.text
	.extern	_op_dmode

ENTRY	_opp_dmode
	putframe
	addq	$8, %rsp		# Burn return PC & 16 byte align stack
	CHKSTKALIGN			# Verify stack alignment
	call	_op_dmode
	getframe
	ret
