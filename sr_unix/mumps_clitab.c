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
#include "cli.h"

static readonly CLI_PARM mumps_parm[] = {
{"INFILE", "What file: "}
};

static readonly CLI_ENTRY mumps_qual[] = {
{ "CROSS_REFERENCE", 0, 0, 0, VAL_DISALLOWED, 0, NON_NEG, VAL_N_A, 0},
{ "DEBUG", 0, 0, 0, VAL_DISALLOWED, 0, NON_NEG, VAL_N_A, 0},
{ "DIRECT_MODE", 0, 0, 0, VAL_DISALLOWED, 0, NON_NEG, VAL_N_A, 0},
{ "IGNORE", 0, 0, 0, VAL_DISALLOWED, 0, NEG, VAL_N_A, 0},
{ "LABELS", 0, 0, 0, VAL_REQ, 1, NON_NEG, VAL_STR, 0},
{ "LENGTH", 0, 0, 0, VAL_REQ, 1, NON_NEG, VAL_NUM, 0},
{ "LINE_ENTRY", 0, 0, 0, VAL_DISALLOWED, 0, NEG, VAL_N_A, 0},
{ "LIST", 0, 0, 0, VAL_NOT_REQ, 1, NEG, VAL_STR, 0},
{ "MACHINE_CODE", 0, 0, 0, VAL_DISALLOWED, 0, NON_NEG, VAL_N_A, 0},
{ "OBJECT", 0, 0, 0, VAL_NOT_REQ, 1, NEG, VAL_STR, 0},
{ "RUN", 0, 0, 0, VAL_DISALLOWED, 0, NON_NEG, VAL_N_A, 0},
{ "SPACE", 0, 0, 0, VAL_REQ, 1, NON_NEG, VAL_NUM, 0},
{ "WARNINGS", 0, 0, 0, VAL_DISALLOWED, 0, NEG, VAL_N_A, 0},
{ 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

GBLDEF CLI_ENTRY cmd_ary[] = {
{ "MUMPS", 0, mumps_qual, mumps_parm, VAL_DISALLOWED, 1, 0, VAL_LIST, 0},
{ 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

