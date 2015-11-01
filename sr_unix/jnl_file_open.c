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
#include <sys/statvfs.h>

#include <errno.h>
#include "gtm_fcntl.h"
#include "gtm_unistd.h"
#include "gtm_stat.h"
#include "gtm_string.h"
#include "gtm_time.h"
#include <netinet/in.h> /* Required for gtmsource.h */
#include <arpa/inet.h>

#include "gdsroot.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "filestruct.h"
#include "jnl.h"
#include "copy.h"
#include "gtmio.h"
#include "interlock.h"
#include "lockconst.h"
#include "aswp.h"
#include "eintr_wrappers.h"
#include "repl_msg.h"
#include "gtmsource.h"
#include "is_file_identical.h"
#include "gtmmsg.h"

#define BACKPTR		ROUND_UP(JREC_PREFIX_SIZE + sizeof(struct_jrec_eof), JNL_REC_START_BNDRY)
#define	EOF_RECLEN	ROUND_UP(JREC_PREFIX_SIZE + sizeof(struct_jrec_eof) + JREC_SUFFIX_SIZE, JNL_REC_START_BNDRY)

#ifdef BIGENDIAN
# define EOF_SUFFIX	((BACKPTR << 8) + JNL_REC_TRAILER)
#else
# define EOF_SUFFIX	(BACKPTR + (JNL_REC_TRAILER << 24))
#endif

GBLREF	jnlpool_ctl_ptr_t	jnlpool_ctl;
GBLREF	boolean_t		pool_init;
GBLREF	uint4			process_id;

error_def(ERR_FILEIDMATCH);
error_def(ERR_JNLACCESS);
error_def(ERR_JNLALIGN);
error_def(ERR_JNLBADLABEL);
error_def(ERR_JNLDBERR);
error_def(ERR_JNLFILOPN);
error_def(ERR_JNLINVALID);
error_def(ERR_JNLMOVED);
error_def(ERR_JNLNAMLEN);
error_def(ERR_JNLOPNERR);
error_def(ERR_JNLRDERR);
error_def(ERR_JNLRECFMT);
error_def(ERR_JNLRECTYPE);
error_def(ERR_JNLTRANSGTR);
error_def(ERR_JNLTRANSLSS);
error_def(ERR_JNLWRERR);
error_def(ERR_IOEOF);

uint4 jnl_file_open(gd_region *reg, bool init, int4 dummy)	/* third argument for compatibility with VMS version */
{
	sgmnt_addrs		*csa;
	sgmnt_data_ptr_t	csd;
	jnl_create_info		create;
	jnl_process_vector	prc;
	jnl_private_control	*jpc;
	jnl_buffer_ptr_t     	jb;
	jnl_file_header		*header;
	jnl_record		*eof_record;
	struct stat		stat_buf;
	int			count, cwd_len, errno_save;
	uint4			save_time, sts, status;
	sm_uc_ptr_t		c, hdr_ptr, nameptr;
	char			*c1, cwdbuf[JNL_NAME_EXP_SIZE], eof_buffer[EOF_RECLEN], prev_jnl_fn[JNL_NAME_SIZE],
	  			hdr_buffer[ROUND_UP(sizeof(jnl_file_header) + (2 * sizeof(uint4)), 2 * sizeof(uint4))];
	int			stat_res, hdr_len;
	int			fstat_res;
	char			*getcwd_res;
	bool			retry;

	hdr_len = ROUND_UP(sizeof(jnl_file_header), sizeof(uint4) *2);
	csa = &FILE_INFO(reg)->s_addrs;
	csd = csa->hdr;
	jpc = csa->jnl;
	jb = jpc->jnl_buff;
	assert(NOJNL == jpc->channel);
	sts = 0;
	jpc->status = 0;
	nameptr = JNL_NAME_EXP_PTR(jb);
	if (init)
	{
		csd->jnl_file.u.inode = 0;
		csd->jnl_file.u.device = 0;
		for (retry = TRUE;  ;)
		{	/* this is stuctured as a loop, in a fashion analogous to the VMS logic,
			 * in order to permit creation of a new file in case the existing one is found wanting.
			 */
			if (0 != sts)
			{
				if (ERR_JNLFILOPN != sts)
					close(jpc->channel);
				jpc->channel = NOJNL;
				if (EACCES == jpc->status || EROFS == jpc->status)
					break;
				if (FALSE == retry)
					break;
				jnl_send_oper(jpc, sts);
				retry = FALSE;
				/* attempt to create a new journal file */
				memset(&create, 0, sizeof(create));
				create.prev_jnl = &prev_jnl_fn[0];
				set_jnl_info(reg, &create);
				if (0 != cre_jnl_file(&create))
				{
					jpc->status = create.status;
					sts = ERR_JNLINVALID;
					continue;
				} else
				{
					jpc->status = 0;
					sts = 0;
				}
			}
			if (csd->jnl_sync_io)
			{
				OPENFILE_SYNC((sm_c_ptr_t)csd->jnl_file_name, O_RDWR, jpc->channel);
				jpc->sync_io = TRUE;
			} else
			{
				OPENFILE((sm_c_ptr_t)csd->jnl_file_name, O_RDWR, jpc->channel);
				jpc->sync_io = FALSE;
			}

			if (-1 == jpc->channel)
			{
				jpc->status = errno;
				sts = ERR_JNLFILOPN;
				jpc->channel = NOJNL;
				continue;
			}
			hdr_ptr = (sm_uc_ptr_t)ROUND_UP((uint4)hdr_buffer, 2 * sizeof(uint4));
			/* Read the journal file header and check the label */
			LSEEKREAD(jpc->channel, 0, hdr_ptr, hdr_len, jpc->status);
			if (0 != jpc->status)
			{
				sts = (-1 == jpc->status) ? ERR_IOEOF : ERR_JNLRDERR;
				continue;
			}
			header = (jnl_file_header *)hdr_ptr;
			if (0 != memcmp(header->label, JNL_LABEL_TEXT, sizeof(JNL_LABEL_TEXT) - 1))
			{
				jpc->status = ERR_JNLBADLABEL;
				sts = ERR_JNLOPNERR;
				continue;
			}
			STAT_FILE(header->data_file_name, &stat_buf, stat_res);
			if (-1 == stat_res)
			{
				jpc->status = errno;
				sts = ERR_JNLDBERR;
				continue;
			}
			if (!is_gdid_stat_identical(&FILE_INFO(reg)->fileid, &stat_buf))
			{
				jpc->status = ERR_FILEIDMATCH;
				sts = ERR_JNLOPNERR;
				continue;
			}
			/* header->end_of_data is the byte offset of the last record;  this
			   should be an EOF record, and it should be aligned on a disk block.
			   If it isn't, we discard this journal file;  if it is, the EOF record
			   will get overwritten by new records */

			LSEEKREAD(jpc->channel,
				  header->end_of_data,
				  (sm_uc_ptr_t)eof_buffer,
				  sizeof(eof_buffer),
				  jpc->status);
			if (0 != jpc->status)
			{
				sts = (-1 == jpc->status) ? ERR_IOEOF : ERR_JNLRDERR;
				continue;
			}
			eof_record = (jnl_record *)eof_buffer;
			if ((header->end_of_data & ~JNL_WRT_START_MASK) != 0)
			{
				jpc->status = ERR_JNLALIGN;
				sts = ERR_JNLOPNERR;
				continue;
			}
			if (JRT_EOF != REF_CHAR(&eof_record->jrec_type))
			{
				jpc->status = ERR_JNLRECTYPE;
				sts = ERR_JNLOPNERR;
				continue;
			}
			if (eof_record->val.jrec_eof.tn != csa->ti->header_open_tn)
			{
				if (eof_record->val.jrec_eof.tn < csa->ti->header_open_tn)
				  jpc->status = ERR_JNLTRANSLSS;
				else
				  jpc->status = ERR_JNLTRANSGTR;
				sts = ERR_JNLOPNERR;
				continue;
			}
			if (EOF_SUFFIX != *(int4 *)((char *)eof_record + BACKPTR + sizeof(int4)))   /* int4 - for filler_suffix */
			{
				jpc->status = ERR_JNLRECFMT;
				sts = ERR_JNLOPNERR;
				continue;
			}
			jb->size = csd->jnl_buffer_size * DISK_BLOCK_SIZE;
			jb->freeaddr = jb->dskaddr = jb->fsync_dskaddr
				     = header->end_of_data;
			jb->lastaddr = header->end_of_data - eof_record->jrec_backpointer;
			/* The following is to make sure that the data in jnl_buffer is aligned with the data in the
			 * disk file on an IO_BLOCK_SIZE boundary. Since we assert that jb->size is a multiple of IO_BLOCK_SIZE,
			 * alignment with respect to jb->size implies alignment with IO_BLOCK_SIZE.
			 */
			assert(0 == jb->size % IO_BLOCK_SIZE);
			jb->free = jb->dsk = header->end_of_data % jb->size;
			SET_LATCH_GLOBAL(&jb->fsync_in_prog_latch, LOCK_AVAILABLE);
			SET_LATCH_GLOBAL(&jb->io_in_prog_latch, LOCK_AVAILABLE);
			FSTAT_FILE(jpc->channel, &stat_buf, fstat_res);
			if (-1 == fstat_res)
			{
				jpc->status = errno;
				sts = ERR_JNLRDERR;
				retry = FALSE;
				continue;
			}
			jb->filesize = stat_buf.st_size;
			jb->min_write_size = JNL_MIN_WRITE;
			jb->max_write_size = JNL_MAX_WRITE;
			jb->before_images = header->before_images;
			jb->epoch_tn = eof_record->val.jrec_eof.tn;
			jb->alignsize = header->alignsize;
			jb->autoswitchlimit = header->autoswitchlimit;
			jb->epoch_interval = header->epoch_interval;
			JNL_SHORT_TIME(jb->next_epoch_time);
			save_time = jb->next_epoch_time;
			jb->next_epoch_time += jb->epoch_interval;
			jnl_prc_vector(&prc);
			if ((ROUND_UP(sizeof(jnl_file_header), DISK_BLOCK_SIZE)) == header->end_of_data)
				header->eov_timestamp = save_time;
			assert(CMP_JNL_PROC_TIME(header->eov_timestamp, header->bov_timestamp) >= 0);
			memcpy(&header->who_opened, &prc, sizeof(header->who_opened));
			assert(header->eov_tn >= header->bov_tn);
			header->eov_tn = (trans_num)-1;		/* this gets reset for normal shutdown else stays =~ infinite */
			header->crash = TRUE;
			if (REPL_ENABLED(csd) && pool_init)
				header->update_disabled = jnlpool_ctl->upd_disabled;
			LSEEKWRITE(jpc->channel, 0, hdr_ptr, hdr_len, jpc->status);
			if (0 != jpc->status)
			{
				sts = ERR_JNLWRERR;
				continue;
			}
			c = (sm_uc_ptr_t)&csd->jnl_file_name[0];
			if ('/' == *c)
			{
				memcpy(nameptr, c, csd->jnl_file_len);
				nameptr[csd->jnl_file_len] = 0;
			}
			else
			{
				if (NULL == GETCWD(cwdbuf, sizeof(cwdbuf), getcwd_res))
				{
					jpc->status = errno;
					sts = ERR_JNLOPNERR;
					retry = FALSE;
					continue;
				}
				cwd_len = strlen(cwdbuf);
				if (('.' == *c)  &&  ('.' == *(c + 1)))
				{
					for (count = 1;  ;  ++count)
					{
						c += 2;
						if (('.' != *(c + 1))  ||  ('.' != *(c + 2)))
							break;
						++c;
					}
					for (c1 = &cwdbuf[cwd_len - 1];  count > 0;  --count)
					{
						while ('/' != *c1)
							--c1;
					}
					if ((c1 - cwdbuf) + (int)csd->jnl_file_len + 1 - (c - (sm_uc_ptr_t)&csd->jnl_file_name[0]) >
						JNL_NAME_EXP_SIZE)
					{
						jpc->status = status = JNL_NAME_EXP_SIZE;
						sts = ERR_JNLNAMLEN;
						/*jnl_send_oper(jpc, sts);*/
						jpc->status = status;
						retry = FALSE;
						continue;
					}
					memcpy(nameptr, cwdbuf, c1 - cwdbuf);
					memcpy(nameptr + (c1 - cwdbuf), c,
					       csd->jnl_file_len + 1 - (c - (sm_uc_ptr_t)csd->jnl_file_name));
				} else
				{
					if ('.' == *c)
						c += 2;
					if (cwd_len + (int)csd->jnl_file_len + 1 - (c - (sm_uc_ptr_t)csd->jnl_file_name)
						> JNL_NAME_EXP_SIZE)
					{
						jpc->status = status = JNL_NAME_EXP_SIZE;
						sts = ERR_JNLNAMLEN;
						/* jnl_send_oper(jpc, sts);*/
						jpc->status = status;
						retry = FALSE;
						continue;
					}
					memcpy(nameptr, cwdbuf, cwd_len);
					nameptr[cwd_len] = '/';
					memcpy(nameptr + cwd_len + 1, c,
					       csd->jnl_file_len + 1 - (c - (sm_uc_ptr_t)csd->jnl_file_name));
				}
			}   /* normalize path */
			break;	/* out of retry loop */
		}   /* for retry */
	} else   /* not init */
	{
		assert(0 != csd->jnl_file.u.inode);
		if (csd->jnl_sync_io)
		{
			OPENFILE_SYNC((sm_c_ptr_t)nameptr, O_RDWR, jpc->channel);
			jpc->sync_io = TRUE;
		} else
		{
			OPENFILE((sm_c_ptr_t)nameptr, O_RDWR, jpc->channel);
			jpc->sync_io = FALSE;
		}
		if (-1 == jpc->channel)
		{
			jpc->status = errno;
			sts = ERR_JNLFILOPN;
			jpc->channel = NOJNL;
		}
	}   /* if init */
	if (0 == sts)
	{
		STAT_FILE((sm_c_ptr_t)nameptr, &stat_buf, stat_res);
		if (0 == stat_res)
		{
			if (init || is_gdid_stat_identical(&csd->jnl_file.u, &stat_buf))
			{
				if (jnl_closed == csd->jnl_state)
				{	/* Operator close came in while opening */
					close(jpc->channel);
					jpc->channel = NOJNL;
					jpc->fileid.inode = 0;
					jpc->fileid.device = 0;
					jpc->lastwrite = 0;
					jpc->regnum = 0;
					jpc->pini_addr = 0;
				} else
				{
					if (init)
					{
						/* Stash the file id in the header for subsequent users */
						set_gdid_from_stat(&csd->jnl_file.u, &stat_buf);
					}
					/* Stash the file id in the process-private area,
				 	  to detect any later change of jnl file "on the fly" */
					set_gdid_from_stat(&jpc->fileid, &stat_buf);
				}  /* if jnl_state */
			} else
				{  /* not init and file moved */
					jpc->status = ERR_JNLMOVED;
					sts = ERR_JNLOPNERR;
				}
		} else
		{	/* stat failed */
			jpc->status = errno;
			sts = ERR_JNLFILOPN;
		}
	}  /* sts == 0 when we started */
	if (0 != sts)
	{
		if (NOJNL != jpc->channel)
			close(jpc->channel);
		jpc->channel = NOJNL;
		status = jpc->status;
		jnl_send_oper(jpc, sts);
		gtm_putmsg(VARLSTCNT(7) sts, 4, csd->jnl_file_len,
			csd->jnl_file_name, reg->dyn.addr->fname_len,
			reg->dyn.addr->fname, status);
	}
	return sts;
}
