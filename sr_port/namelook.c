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
#include "compiler.h"
#include "opcode.h"
#include "toktyp.h"
#include "nametabtyp.h"
#include "gtm_caseconv.h"
#include "namelook.h"

int namelook(unsigned char  offset_tab[],nametabent *name_tab,char *str)
{
	unsigned char	temp[20], x;
	int4		strlength;
	nametabent	*top, *i;

	if ((strlength = mid_len((mident *)str)) > 20)
	{	return -1;
	}
	lower_to_upper(&temp[0], (uchar_ptr_t)str, strlength);
	if ((x = temp[0]) == '%')
	{	return -1;
	}
	i = name_tab + offset_tab[x -= 'A'];
	top = name_tab + offset_tab[++x];
	assert(i == top || i->name[0] >= temp[0]);
	for (;i<top;i++)
	{
		if ((strlength == i->len) || ((strlength > i->len) && (i->name[i->len] == '*')))
		{
			if (!memcmp(&temp[0], i->name, (int4)(i->len)))
			{	return i-name_tab;
			}
		}
	}
	return -1;
}