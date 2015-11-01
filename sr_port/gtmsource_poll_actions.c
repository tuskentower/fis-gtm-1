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

#include "gtm_time.h"
#include "gtm_string.h"
#include "gtm_unistd.h"
#include <netinet/in.h> /* Required for gtmsource.h */
#include <arpa/inet.h>
#ifdef VMS
#include <descrip.h> /* Required for gtmsource.h */
#endif

#include "gdsroot.h"
#include "gdsblk.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "filestruct.h"
#include "repl_msg.h"
#include "gtmsource.h"
#include "repl_dbg.h"
#include "repl_log.h"
#include "gtm_stdio.h"
#include "iosp.h"
#include "repl_shutdcode.h"
#include "gtmsource_heartbeat.h"
#include "jnl.h"
#include "repl_filter.h"
#include "util.h"
#include "repl_comm.h"

GBLREF	jnlpool_addrs		jnlpool;
GBLREF	int			gtmsource_sock_fd;
GBLREF	gtmsource_state_t	gtmsource_state;
GBLREF	boolean_t		gtmsource_logstats;
GBLREF	int			gtmsource_log_fd;
GBLREF 	FILE			*gtmsource_log_fp;
GBLREF	int			gtmsource_statslog_fd;
GBLREF 	FILE			*gtmsource_statslog_fp;
GBLREF	int			gtmsource_filter;

int gtmsource_poll_actions(boolean_t poll_secondary)
{
	/* This function should be called only in active mode, but cannot assert for it */

	gtmsource_local_ptr_t	gtmsource_local;
	time_t			now;
	repl_heartbeat_msg_t	overdue_heartbeat;
	char			seq_num_str[32], *seq_num_ptr;
	char			*time_ptr;
	char			time_str[CTIME_BEFORE_NL + 1];

	gtmsource_local = jnlpool.gtmsource_local;
	if (SHUTDOWN == gtmsource_local->shutdown)
	{
		repl_log(gtmsource_log_fp, TRUE, TRUE, "Shutdown signalled\n");
		gtmsource_end(); /* Won't return */
	}
	if (GTMSOURCE_CHANGING_MODE !=  gtmsource_state &&
	    GTMSOURCE_MODE_PASSIVE == gtmsource_local->mode)
	{
		repl_log(gtmsource_log_fp, TRUE, TRUE, "Changing mode from ACTIVE to PASSIVE\n");
		gtmsource_state = GTMSOURCE_CHANGING_MODE;
		return (SS_NORMAL);
	}

	if (poll_secondary && GTMSOURCE_CHANGING_MODE != gtmsource_state &&
	    GTMSOURCE_WAITING_FOR_CONNECTION != gtmsource_state)
	{
		now = time(NULL);
		if (gtmsource_is_heartbeat_overdue(&now, &overdue_heartbeat))
		{
			time_ptr = GTM_CTIME((time_t *)&overdue_heartbeat.ack_time[0]);
			memcpy(time_str, time_ptr, CTIME_BEFORE_NL);
			time_str[CTIME_BEFORE_NL] = '\0';
			repl_log(gtmsource_log_fp, TRUE, TRUE, "No response received for heartbeat sent at %s with SEQNO "
				 INT8_FMT" in %d seconds. Closing connection\n", time_str,
				 INT8_PRINT(*(seq_num *)&overdue_heartbeat.ack_seqno[0]),
				 gtmsource_local->connect_parms[GTMSOURCE_CONN_HEARTBEAT_MAX_WAIT]);
			repl_close(&gtmsource_sock_fd);
			SHORT_SLEEP(GTMSOURCE_WAIT_FOR_RECEIVER_CLOSE_CONN);
			gtmsource_state = GTMSOURCE_WAITING_FOR_CONNECTION;
			return (SS_NORMAL);
		}

		if (gtmsource_is_heartbeat_due(&now) && !gtmsource_is_heartbeat_stalled)
		{
			gtmsource_send_heartbeat(&now);
			if (GTMSOURCE_WAITING_FOR_CONNECTION == gtmsource_state ||
		    	    GTMSOURCE_CHANGING_MODE == gtmsource_state)
				return (SS_NORMAL);
		}
	}

	if (gtmsource_local->changelog)
	{
		repl_log(gtmsource_log_fp, TRUE, TRUE, "Changing log file to %s\n", gtmsource_local->log_file);
#ifdef UNIX
		repl_log_init(REPL_GENERAL_LOG, &gtmsource_log_fd, NULL, gtmsource_local->log_file, NULL);
		repl_log_fd2fp(&gtmsource_log_fp, gtmsource_log_fd);
#elif defined(VMS)
		util_log_open(STR_AND_LEN(gtmsource_local));
#else
#error unsupported platform
#endif
		gtmsource_local->changelog = FALSE;
	}
	if (!gtmsource_logstats && gtmsource_local->statslog)
	{
#ifdef UNIX
		gtmsource_logstats = TRUE;
		repl_log_init(REPL_STATISTICS_LOG, &gtmsource_log_fd, &gtmsource_statslog_fd, gtmsource_local->log_file,
			      gtmsource_local->statslog_file);
		repl_log_fd2fp(&gtmsource_statslog_fp, gtmsource_statslog_fd);
		repl_log(gtmsource_log_fp, TRUE, TRUE, "Starting stats log to %s\n", gtmsource_local->statslog_file);
#else
		repl_log(gtmsource_log_fp, TRUE, TRUE, "Stats logging tobe done on VMS\n", gtmsource_local->statslog_file);
#endif

	} else if (gtmsource_logstats && !gtmsource_local->statslog)
	{
		gtmsource_logstats = FALSE;
		repl_log(gtmsource_log_fp, TRUE, TRUE, "Stopping stats log\n");
		close(gtmsource_statslog_fd);
		gtmsource_statslog_fd = -1;
	}
	if ((gtmsource_filter & EXTERNAL_FILTER) && ('\0' == gtmsource_local->filter_cmd[0]))
	{
		repl_log(gtmsource_log_fp, TRUE, TRUE, "Stopping filter\n");
		repl_stop_filter();
		gtmsource_filter &= ~EXTERNAL_FILTER;
	}
	return (SS_NORMAL);
}