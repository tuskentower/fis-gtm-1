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

#include "gtm_stdio.h"
#include "error.h"
#include "min_max.h"
#include "stringpool.h"
#include "mlkdef.h"
#include "zshow.h"
#include "rtnhdr.h"
#include "stack_frame.h"
#include "mvalconv.h"
#include "error_trap.h"

#define OUT_BUFF_SIZE	2048

GBLREF mval		dollar_zstatus;
GBLREF mval		dollar_zerror;
GBLREF stack_frame	*zyerr_frame;
GBLREF char		*util_outptr, util_outbuff[OUT_BUFF_SIZE];

char *set_zstatus(mstr *src, int arg, unsigned char **ctxtp)
{
	char	*b_line;	/* beginning of line (used to restart line) */
	mval	val;		/* pointer to dollar_zstatus */
	char	sev, zstatus_buff[OUT_BUFF_SIZE];
	char	*zstatus_bptr;
	int	len, util_len, tmp_len;
	mval	*status_loc;

	b_line = 0;
	/* get the line address of the last "known" MUMPS code that was executed.  MUMPS indirection
	 * consitutes MUMPS code that is "unknown" is the sense that there is no line address for it.
	 */
	MV_FORCE_MVAL(&val, arg);
	n2s(&val);
	src->len = get_symb_line((uchar_ptr_t)src->addr, (unsigned char **)&b_line, (unsigned char **) ctxtp)
			- (uchar_ptr_t)src->addr;
	memcpy(zstatus_buff, val.str.addr, val.str.len);
	zstatus_bptr = zstatus_buff + val.str.len;
	*zstatus_bptr++ = ',';
	if (0 != b_line)
	{
		memcpy(zstatus_bptr, src->addr, src->len);
		zstatus_bptr += src->len;
		*zstatus_bptr++ = ',';
	}
	*util_outptr = 0;
	util_len = util_outptr  - util_outbuff;
	len = MIN(OUT_BUFF_SIZE - util_len, util_len);
	memcpy(zstatus_bptr, util_outbuff, len);
	for (tmp_len = 0 ; tmp_len < len; zstatus_bptr++, tmp_len++)
	{
		if ('\n' == *zstatus_bptr)
		{
			*zstatus_bptr = ',';
		}
	}
	status_loc = (NULL == zyerr_frame) ? &dollar_zstatus : &dollar_zerror;
	status_loc->str.len = zstatus_bptr - zstatus_buff;
	status_loc->str.addr = zstatus_buff;
	s2pool(&status_loc->str);
	status_loc->mvtype = MV_STR;
	set_ecode(arg);
	return (b_line);
}