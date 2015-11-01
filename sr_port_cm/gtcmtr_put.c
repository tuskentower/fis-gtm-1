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
#include "copy.h"

#include "gdsroot.h"
#include "gtm_facility.h"
#include "gtm_string.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "gdsblk.h"
#include "filestruct.h"
#include "jnl.h"
#include "cmidef.h"
#include "hashdef.h"
#include "cmmdef.h"
#include "gtcmtr_put.h"
#include "format_targ_key.h"
#include "gtcm_find_region.h"
#include "gtcm_bind_name.h"
#include "gvcst_put.h"

#define LCL_BUF_SIZE 256

GBLREF gd_region	*gv_cur_region;
GBLREF sgmnt_addrs	*cs_addrs;
GBLREF gv_key		*gv_currkey;
GBLREF connection_struct *curr_entry;
GBLREF jnl_process_vector *originator_prc_vec;

bool gtcmtr_put(void)
{
	cm_region_list	*reg_ref;
	mval		v;
	unsigned char	buff[LCL_BUF_SIZE], *end;
	unsigned char	*ptr, regnum;
	short		n;
	unsigned short	top, len;
	static readonly gds_file_id file;

	error_def(ERR_KEY2BIG);
	error_def(ERR_REC2BIG);
	error_def(ERR_GVIS);
	error_def(ERR_DBPRIVERR);

	ptr = curr_entry->clb_ptr->mbf;
	assert(*ptr == CMMS_Q_PUT);
	ptr++;
	GET_USHORT(len, ptr);
	ptr += sizeof(unsigned short);
	regnum = *ptr++;
	reg_ref = gtcm_find_region(curr_entry,regnum);
	len--; /* subtract size of regnum */
	CM_GET_GVCURRKEY(ptr, len);
	gtcm_bind_name(reg_ref->reghead, TRUE);
	if (gv_cur_region->read_only)
		rts_error(VARLSTCNT(4) ERR_DBPRIVERR, 2, DB_LEN_STR(gv_cur_region));
	if (JNL_ENABLED(cs_addrs->hdr))
	{
		originator_prc_vec = curr_entry->pvec;
		cs_addrs->jnl->pini_addr = reg_ref->pini_addr;
	}
	GET_USHORT(len, ptr);
	ptr += sizeof(unsigned short);
	v.mvtype = MV_STR;
	v.str.len = len;
	v.str.addr = (char *)ptr;
	if ((n = gv_currkey->end + 1) > gv_cur_region->max_key_size)
	{
		if ((end = format_targ_key(&buff[0], LCL_BUF_SIZE, gv_currkey, TRUE)) == 0)
			end = &buff[LCL_BUF_SIZE - 1];
		rts_error(VARLSTCNT(11) ERR_KEY2BIG, 4, n, (int4)gv_cur_region->max_key_size,
			REG_LEN_STR(gv_cur_region), 0, ERR_GVIS, 2, end - buff, buff);
	}
	if (n + v.str.len + sizeof(rec_hdr) > gv_cur_region->max_rec_size)
	{
		if ((end = format_targ_key(&buff[0], LCL_BUF_SIZE, gv_currkey, TRUE)) == 0)
			end = &buff[LCL_BUF_SIZE - 1];
		rts_error(VARLSTCNT(11) ERR_REC2BIG, 4, n + v.str.len + sizeof(rec_hdr),
			(int4)gv_cur_region->max_rec_size, REG_LEN_STR(gv_cur_region),
			0, ERR_GVIS, 2, end - buff, buff);
	}
	gvcst_put(&v);
	if (JNL_ENABLED(cs_addrs->hdr))
		reg_ref->pini_addr = cs_addrs->jnl->pini_addr; /* In case  journal switch occurred */
	ptr = curr_entry->clb_ptr->mbf;
	*ptr++ = CMMS_R_PUT;
	curr_entry->clb_ptr->cbl = S_HDRSIZE;
	return TRUE;
}