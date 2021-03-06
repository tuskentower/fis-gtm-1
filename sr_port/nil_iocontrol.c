/****************************************************************
 *								*
 *	Copyright 2001, 2014 Fidelity Information Services, Inc	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

#include "mdef.h"
#include "io.h"

void nil_iocontrol(mstr *mn, int4 argcnt, va_list args)
{
	/**** SHOULD BE 'NOT SUPPORTED FOR THIS DEVICE TYPE ****/
	return;
}

void nil_dlr_device(mstr *d)
{
	d->len = 0;
	return;
}

void nil_dlr_key(mstr *d)
{
	d->len = 0;
	return;
}

void nil_dlr_zkey(mstr *d)
{
	d->len = 0;
	return;
}
