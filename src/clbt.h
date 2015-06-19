/***********************************************************************/
/*
*   Script File: clbt.h
*
*   Description:
*
*   Core header for CLBT
*
*
*   Author: Joshua Zhang (zzbhf@mail.missouri.edu)
*   Date since: Feb-2015
*
*   Copyright (c) <2015> <Joshua Z. ZHANG>	 - All Rights Reserved.
*
*	 Open source according to LGPLv3 License.
*	 No warrenty implied, use at your own risk.
*/
/***********************************************************************/

#ifndef _CLBT_HEADER_H_
#define _CLBT_HEADER_H_

#ifdef __cplusplus
extern "C" {
#endif

#define CLBT_MEMORY_ERR		-1
#define CLBT_OK				0
#define CLBT_INVALID_OP		1
#define CLBT_FAILURE_IO		2
#define CLBT_FAILURE_OS		3

/* Possible options for CLBT */
enum { CLBT_OPT_DEFAULT = 0, CLBT_OPT_QUIET = 1, CLBT_OPT_VERBOSE = 2, CLBT_OPT_RECURSIVE = 4, CLBT_OPT_FORCE = 8 };
/* Possible tasks for CLBT */
enum { CLBT_TASK_DEFAULT = 0, CLBT_TASK_LIST = 1, CLBT_TASK_RENAME = 2 };


/* CLBT functions */
int clbt_run(int options, int tasks);

#ifdef __cplusplus
}
#endif
#endif /* end _CLBT_HEADER_H_ */

