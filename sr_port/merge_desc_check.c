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

#include "gtm_string.h"
#include "min_max.h"
#include "hashdef.h"
#include "lv_val.h"
#include "sbs_blk.h"
#include "subscript.h"
#include "gdsroot.h"
#include "gdskill.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "zshow.h"
#include "zwrite.h"
#include "filestruct.h"
#include "gdscc.h"
#include "copy.h"
#include "jnl.h"
#include "hashtab.h"
#include "buddy_list.h"
#include "tp.h"
#include "merge_def.h"
#include "gvname_info.h"
#include "lvname_info.h"
#include "op_merge.h"
#include "format_targ_key.h"

GBLREF int              merge_args;
GBLREF merge_glvn_ptr	mglvnp;

void merge_desc_check(void)
{
        unsigned char		buff1[MAX_STRLEN], buff2[MAX_STRLEN], *end1, *end2;
	enum db_acc_method	acc_meth1, acc_meth2;
	int			cmp_less;

	error_def(ERR_MERGEDESC);

	if (MARG1_IS_GBL(merge_args) && MARG2_IS_GBL(merge_args))
	{
		acc_meth1 = mglvnp->gblp[IND1]->s_gv_cur_region->dyn.addr->acc_meth;
		acc_meth2 = mglvnp->gblp[IND2]->s_gv_cur_region->dyn.addr->acc_meth;
		/* if ((both are bg/mm regions && dbs are same && same global) ||
		 *     (both are cm regions && on the same remote node && same region))
		 *   COMPARE GV_CURRKEYs
		 * else
		 *   NOT DESCENDANTS
		 * endif
		 */
		if (((dba_bg == acc_meth1 || dba_mm == acc_meth2) && (dba_bg == acc_meth1 || dba_mm == acc_meth2))
			&& REG_EQUAL(FILE_INFO(mglvnp->gblp[IND1]->s_gv_target->gd_reg), mglvnp->gblp[IND2]->s_gv_target->gd_reg)
			&& mglvnp->gblp[IND1]->s_gv_target->root == mglvnp->gblp[IND2]->s_gv_target->root)
			cmp_less = 1;
		else if (dba_cm == acc_meth1 && dba_cm == acc_meth2
				&& mglvnp->gblp[IND1]->s_gv_cur_region->dyn.addr->cm_blk ==
				   mglvnp->gblp[IND2]->s_gv_cur_region->dyn.addr->cm_blk
				&& mglvnp->gblp[IND1]->s_gv_cur_region->cmx_regnum ==
				   mglvnp->gblp[IND2]->s_gv_cur_region->cmx_regnum)
			cmp_less = 0;
		else
		{
			assert(dba_usr != acc_meth1 && dba_usr != acc_meth2); /* merge not ready for dba_usr yet */
			return;
		}
		if (0 == memcmp(mglvnp->gblp[IND1]->s_gv_currkey->base, mglvnp->gblp[IND2]->s_gv_currkey->base,
			        MIN(mglvnp->gblp[IND1]->s_gv_currkey->end - cmp_less, 
				    mglvnp->gblp[IND2]->s_gv_currkey->end - cmp_less)))
		{
			if (0 == (end1 = format_targ_key(buff1, MAX_STRLEN, mglvnp->gblp[IND1]->s_gv_currkey, TRUE)))
				end1 = &buff1[MAX_STRLEN - 1];
			if (0 == (end2 = format_targ_key(buff2, MAX_STRLEN, mglvnp->gblp[IND2]->s_gv_currkey, TRUE)))
				end2 = &buff2[MAX_STRLEN - 1];
			if (mglvnp->gblp[IND1]->s_gv_currkey->end > mglvnp->gblp[IND2]->s_gv_currkey->end)
				rts_error(VARLSTCNT(6) ERR_MERGEDESC, 4, end1 - buff1, buff1, end2 - buff2, buff2);
			else
				rts_error(VARLSTCNT(6) ERR_MERGEDESC, 4, end2 - buff2, buff2, end1 - buff1, buff1);
		}
	} else if (MARG1_IS_LCL(merge_args) && MARG2_IS_LCL(merge_args))
	{
		if (lcl_arg1_is_desc_of_arg2(mglvnp->lclp[IND1], mglvnp->lclp[IND2]))
		{
			end1 = format_key_lv_val(mglvnp->lclp[IND1], buff1, sizeof(buff1));
			end2 = format_key_lv_val(mglvnp->lclp[IND2], buff2, sizeof(buff2));
			rts_error(VARLSTCNT(6) ERR_MERGEDESC, 4, end1 - buff1, buff1, end2 - buff2, buff2);
		} else if (lcl_arg1_is_desc_of_arg2(mglvnp->lclp[IND2], mglvnp->lclp[IND1]))
		{
			end1 = format_key_lv_val(mglvnp->lclp[IND1], buff1, sizeof(buff1));
			end2 = format_key_lv_val(mglvnp->lclp[IND2], buff2, sizeof(buff2));
			rts_error(VARLSTCNT(6) ERR_MERGEDESC, 4, end2 - buff2, buff2, end1 - buff1, buff1);
		}
	}
}