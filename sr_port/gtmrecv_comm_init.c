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

#include "gtm_socket.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <errno.h>
#include <fcntl.h>
#include "gtm_unistd.h"
#ifdef VMS
#include <descrip.h> /* Required for gtmrecv.h */
#endif

#include "gdsroot.h"
#include "gdsblk.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "filestruct.h"
#include "gtmrecv.h"
#include "repl_sp.h"

GBLDEF	int			gtmrecv_listen_sock_fd = -1;

int gtmrecv_comm_init(in_port_t port)
{
	/* Initialize communication stuff */

	struct sockaddr_in	secondary_addr;

	const	int	enable_reuseaddr = 1;
	const   int    	disable_keepalive = 1;
	struct  linger  disable_linger = {0, 0};

	error_def(ERR_REPLCOMM);
	error_def(ERR_TEXT);

	if (-1 != gtmrecv_listen_sock_fd) /* Initialization done already */
		return (0);

	/* Create the socket used for communicating with primary */
	if (-1 == (gtmrecv_listen_sock_fd = socket(AF_INET, SOCK_STREAM, 0)))
	{
		rts_error(VARLSTCNT(7) ERR_REPLCOMM, 0, ERR_TEXT, 2,
				RTS_ERROR_LITERAL("Error with receiver server socket create"), ERRNO);
		return (-1);
	}

	if (0 > setsockopt(gtmrecv_listen_sock_fd, SOL_SOCKET, SO_LINGER, (const void *)&disable_linger, sizeof(disable_linger)))
		rts_error(VARLSTCNT(7) ERR_REPLCOMM, 0, ERR_TEXT, 2,
				RTS_ERROR_LITERAL("Error with receiver server listen socket disable linger"), ERRNO);

#ifdef REPL_DISABLE_KEEPALIVE
	if (0 > setsockopt(gtmrecv_listen_sock_fd, SOL_SOCKET, SO_KEEPALIVE, (const void *)&disable_keepalive,
			sizeof(disable_keepalive)))
	{
		/* Till SIGPIPE is handled properly */
		rts_error(VARLSTCNT(7) ERR_REPLCOMM, 0, ERR_TEXT, 2,
				RTS_ERROR_LITERAL("Error with receiver server listen socket disable keepalive"), ERRNO);
	}
#endif

	if (0 > setsockopt(gtmrecv_listen_sock_fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&enable_reuseaddr,
			sizeof(enable_reuseaddr)))
	{
		rts_error(VARLSTCNT(7) ERR_REPLCOMM, 0, ERR_TEXT, 2,
				RTS_ERROR_LITERAL("Error with receiver server listen socket enable reuseaddr"), ERRNO);
	}

	/* Make it known to the world that you are ready for a Source Server */
	memset((char *)&secondary_addr, 0, sizeof(secondary_addr));
	secondary_addr.sin_family = AF_INET;
	secondary_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	secondary_addr.sin_port = htons(port);

	if (0 > BIND(gtmrecv_listen_sock_fd, (struct sockaddr *)&secondary_addr, sizeof(secondary_addr)))
	{
		rts_error(VARLSTCNT(7) ERR_REPLCOMM, 0, ERR_TEXT, 2, RTS_ERROR_LITERAL("Could not bind local address"), ERRNO);
		close(gtmrecv_listen_sock_fd);
		return (-1);
	}

	if (0 > listen(gtmrecv_listen_sock_fd, 5))
	{
		rts_error(VARLSTCNT(7) ERR_REPLCOMM, 0, ERR_TEXT, 2, RTS_ERROR_LITERAL("Could not listen"), ERRNO);
		close(gtmrecv_listen_sock_fd);
		return (-1);
	}

	return (0);
}