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

/*
 * --------------------------------------------------------------
 * Parser tables.
 * Entries need to be made in sorted order (lexicographic) within
 * each table.
 * --------------------------------------------------------------
 */

#include "mdef.h"
#include "mlkdef.h"
#include "cmidef.h"
#include "cli.h"
#include "lke.h"
#include "util_spawn.h"

static readonly CLI_ENTRY clear_qual[] = {
	{ "ALL",          0, 0, 0, VAL_DISALLOWED, 0, NON_NEG, VAL_N_A, 0 },
	{ "INTERACTIVE",  0, 0, 0, VAL_DISALLOWED, 0, NEG,     VAL_N_A, 0 },
	{ "LOCK", 	  0, 0, 0, VAL_REQ,	   1, NON_NEG, VAL_STR, 0 },
	{ "OUTPUT", 	  0, 0, 0, VAL_REQ, 	   1, NON_NEG, VAL_STR, 0 },
	{ "PID", 	  0, 0, 0, VAL_REQ, 	   1, NON_NEG, VAL_NUM, VAL_DCM },
	{ "REGION", 	  0, 0, 0, VAL_REQ, 	   0, NON_NEG, VAL_STR, 0 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

static readonly CLI_ENTRY show_qual[] = {
	{ "ALL",      0, 0, 0, VAL_DISALLOWED, 0, NON_NEG, VAL_N_A, 0 },
	{ "CRIT",     0, 0, 0, VAL_DISALLOWED, 0, NEG,     VAL_N_A, 0 },
	{ "LOCK",     0, 0, 0, VAL_REQ,        1, NON_NEG, VAL_STR, 0 },
	{ "MEMORY",   0, 0, 0, VAL_DISALLOWED, 0, NON_NEG, VAL_N_A, 0 },
	{ "OUTPUT",   0, 0, 0, VAL_REQ,        1, NON_NEG, VAL_STR, 0 },
	{ "PID",      0, 0, 0, VAL_REQ,        1, NON_NEG, VAL_NUM, VAL_DCM },
	{ "REGION",   0, 0, 0, VAL_REQ,        0, NON_NEG, VAL_STR, 0 },
	{ "WAIT",     0, 0, 0, VAL_DISALLOWED, 0, NON_NEG, VAL_STR, 0 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

/*
 * Main command table.
 * It has to be names cmd_ary, because it is being referred to by the cli_parse module.
 *
 * This table contains the names of all commands, and corresponding functions to be
 * dispatched to, and the qualifier sub-tables, containing all legal qualifiers.
 */

GBLDEF CLI_ENTRY cmd_ary[] = {
	{ "CLEAR",  lke_clear,  clear_qual, 0, VAL_NOT_REQ,    2, 0, VAL_STR, 0},
	{ "EXIT",   lke_exit,   0,          0, VAL_DISALLOWED, 0, 0, 0,       0},
	{ "HELP",   lke_help,   0,          0, VAL_NOT_REQ,    5, 0, 0,       0},
	{ "SETGDR", lke_setgdr,	0,          0, VAL_REQ,        1, 0, 0,       0},
	{ "SHOW",   lke_show,   show_qual,  0, VAL_NOT_REQ,    1, 0, VAL_STR, 0},
	{ "SPAWN",  util_spawn,	0,          0, VAL_DISALLOWED, 0, 0, 0,       0},
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};