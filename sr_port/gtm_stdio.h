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

/* gtm_stdio.h - gtm interface to stdio.h */

#ifndef GTM_STDIOH
#define GTM_STDIOH

#include <stdio.h>

#define FDOPEN				fdopen
#define FGETS(strg,n,strm,fgets_res)	(fgets_res = fgets(strg,n,strm))
#define Fopen				fopen
#define GETS(buffer,gets_res)		syntax error
#define PERROR				perror
#define	POPEN				popen
#define TEMPNAM				tempnam
#define RENAME				rename
#define SETVBUF				setvbuf
#define FPRINTF         		fprintf
#define FSCANF         			fscanf
#define PRINTF         			printf
#define SCANF          			scanf
#define SSCANF         			sscanf
#define SPRINTF       			sprintf
#define VFPRINTF       			vfprintf
#define VPRINTF        			vprintf
#define VSPRINTF       			vsprintf

#define SPRINTF_ENV_NUM(ENV_VAR, ENV_VAL)	sprintf(c1, "%s=%d", ENV_VAR, ENV_VAL)
#define SPRINTF_ENV_STR(ENV_VAR, ENV_VAL)	sprintf(c1, "%s=%s", ENV_VAR, ENV_VAL)

#endif