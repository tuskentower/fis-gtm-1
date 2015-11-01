/****************************************************************
 *								*
 *	Copyright 2001 Sanchez Computer Associates, Inc.	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

#include "mdef.h"
#include "io.h"
#include "iousdef.h"

short ious_open(io_log_name *dev, mval *pp, int file_des, mval *mspace, int4 timeout)
{
	io_desc *iod;
	dev_dispatch_struct *fgn_driver;
	error_def(ERR_USRIOINIT);

	iod = dev->iod;
	if (iod->state == dev_never_opened)
	{
		iod->dev_sp = (void *)malloc(sizeof(d_us_struct));
		memset(iod->dev_sp, 0, sizeof(d_us_struct));
		iod->state = dev_closed;
	}

	if (iod->state != dev_open)
	{
		if (mspace && mspace->str.len)
		{	fgn_driver = io_get_fgn_driver(&mspace->str);
			((d_us_struct*)(iod->dev_sp))->disp = fgn_driver;
		}
		else if (!(((d_us_struct*)(iod->dev_sp))->disp->open))
		{	rts_error(VARLSTCNT(1) ERR_USRIOINIT);
			return FALSE;
		}

		((void(*)())(((d_us_struct*)(iod->dev_sp))->disp->open))();
		iod->state = dev_open;
	}
	return TRUE;
}