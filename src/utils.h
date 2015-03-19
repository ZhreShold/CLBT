/***********************************************************************/
/*
/#   Script File: utils.h
/#
/#   Description:
/#
/#   Utility functions for CLBT
/#
/#
/#   Author: Joshua Zhang (zzbhf@mail.missouri.edu)
/#   Date since: Feb-2015
/#
/#   Copyright (c) <2015> <Joshua Z. ZHANG>	 - All Rights Reserved.
/#
/#	 Open source according to LGPLv3 License.
/#	 No warrenty implied, use at your own risk.
*/
/***********************************************************************/

#ifndef _UTILS_H_
#define _UTILS_H_

#include "core.h"

#ifdef __cplusplus
extern "C" {
#endif

	int x_getcwd(Path* cwd);
	int waitkey(double ms);
	double get_real_time();
	double get_elapsed_time_ms(double lastTime);
	int get_key();
	void hold_screen();
	int kb_hit();

#ifdef __cplusplus
}
#endif
#endif

