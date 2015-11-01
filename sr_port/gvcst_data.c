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
#include "cdb_sc.h"
#include "gdsroot.h"
#include "gdsblk.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "copy.h"
#include "t_end.h"
#include "t_retry.h"
#include "t_begin.h"
#include "tp_grab_crit.h"
#include "gvcst_rtsib.h"
#include "gvcst_search.h"
#include "gvcst_search_blk.h"
#include "gvcst_data.h"

GBLREF gv_key		*gv_currkey;
GBLREF gv_namehead	*gv_target;
GBLREF sgmnt_addrs	*cs_addrs;
GBLREF gd_region	*gv_cur_region;
GBLREF short		dollar_tlevel;
GBLREF uint4		t_err;
GBLREF unsigned int	t_tries;

mint	gvcst_data(void)
{
	blk_hdr_ptr_t	bp;
	enum cdb_sc	status;
	mint		val;
	rec_hdr_ptr_t	rp;
	unsigned short	rec_size;
	srch_blk_status *bh;
	srch_hist	*rt_history;
	sm_uc_ptr_t	b_top;
	error_def(ERR_GVDATAFAIL);

	assert((gv_target->root < cs_addrs->ti->total_blks) || (0 < dollar_tlevel));
	if (0 == dollar_tlevel)
		t_begin(ERR_GVDATAFAIL, FALSE);
	else
		t_err = ERR_GVDATAFAIL;
	if (!((t_tries < CDB_STAGNATE) || cs_addrs->now_crit))	/* Final retry and this region not locked down */
	{
		if (0 == dollar_tlevel)				/* Verify tp */
			GTMASSERT;
		if (FALSE == tp_grab_crit(gv_cur_region))	/* Attempt lockdown now */
		{
			status = cdb_sc_needcrit;		/* avoid deadlock -- restart transaction */
			t_retry(status);
		}
	}
	for (;;)
	{
		rt_history = gv_target->alt_hist;
		rt_history->h[0].blk_num = 0;
		if ((status = gvcst_search(gv_currkey, NULL)) != cdb_sc_normal)
		{
			t_retry(status);
			continue;
		}
		bh = gv_target->hist.h;
		bp = (blk_hdr_ptr_t)bh->buffaddr;
		rp = (rec_hdr_ptr_t)(bh->buffaddr + bh->curr_rec.offset);
		b_top = bh->buffaddr + bp->bsiz;
		val = 0;
		if (gv_currkey->end + 1 == bh->curr_rec.match)
			val = 1;
		else if (bh->curr_rec.match >= gv_currkey->end)
			val = 10;
		if (1 == val  ||  rp == (rec_hdr_ptr_t)b_top)
		{
			GET_USHORT(rec_size, &rp->rsiz);
			if (rp == (rec_hdr_ptr_t)b_top  ||  (sm_uc_ptr_t)rp + rec_size == b_top)
			{
				if (cdb_sc_endtree != (status = gvcst_rtsib(rt_history, 0)))
				{
					if ((cdb_sc_normal != status)
						|| (cdb_sc_normal != (status = gvcst_search_blk(gv_currkey, rt_history->h))))
					{
						t_retry(status);
						continue;
					}
					if (rt_history->h[0].curr_rec.match >= gv_currkey->end)
						val += 10;
				}
			} else
			{
				if ((sm_uc_ptr_t)rp + rec_size > b_top)
				{
					t_retry(cdb_sc_rmisalign);
					continue;
				}
				rp = (rec_hdr_ptr_t)((sm_uc_ptr_t)rp + rec_size);
				if (rp->cmpc >= gv_currkey->end)
					val += 10;
			}
		}
		if (0 == dollar_tlevel)
		{
			if (0 == t_end(&gv_target->hist, 0 == rt_history->h[0].blk_num ? NULL : rt_history))
				continue;
		} else
		{
			status = tp_hist(0 == rt_history->h[0].blk_num ? NULL : rt_history);
			if (cdb_sc_normal != status)
			{
				t_retry(status);
				continue;
			}
		}
		if (cs_addrs->read_write)
			cs_addrs->hdr->n_data++;
		return val;
	}
}