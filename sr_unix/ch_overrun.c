/****************************************************************
 *								*
 *	Copyright 2001, 2013 Fidelity Information Services, Inc	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

/* ch_overrun -- condition handler overrun -- default condition handler for Unix -- no normal return from this module */
#include "mdef.h"

#include "gtm_stdlib.h"	/* for exit() usage in MUMPS_EXIT macro */

#include "error.h"
#include "send_msg.h"
#include "gtmmsg.h"

GBLREF int		mumps_status;
GBLREF boolean_t	exit_handler_active;

error_def(ERR_NOCHLEFT);

void ch_overrun(void)
{
	PRN_ERROR;
	gtm_putmsg_csa(CSA_ARG(NULL) VARLSTCNT(1) ERR_NOCHLEFT);
	send_msg_csa(CSA_ARG(NULL) VARLSTCNT(1) ERR_NOCHLEFT);

	/* If exit handler is already active, we will just core and die */
	if (exit_handler_active)
		gtm_dump_core();
	else
	{	/* Otherwise, we generate a core and exit to drive the condition handler */
		gtm_fork_n_core();
		MUMPS_EXIT;
	}
}
