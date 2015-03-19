/***********************************************************************/
/*
/#   Script File: core.h
/#
/#   Description:
/#
/#   Core modules for CLBT
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

#ifndef _CORE_H_
#define _CORE_H_

#ifdef __cplusplus
extern "C" {
#endif

	enum 
	{
		ERR_MEMORY = -1, 
		ERR_OK, 
		ERR_INVALID_OP, 
		ERR_FAIL,
	};


	/// <summary>
	/// Various options in execution
	/// </summary>
	typedef struct Options
	{
		int force;			//!< force operation without confirmation
		int verbose;		//!< print more logs
		int quiet;			//!< quiet mode, suppress any message, including error
		int recursive;		//!< search recursively in sub-directories

	}Options;

	//extern Options g_options;		//!< global options container

	/// <summary>
	/// Various tasks TODO
	/// </summary>
	typedef struct Tasks
	{
		int rename;			//!< rename files
		int list;			//!< show list of matched files

	}Tasks;

	typedef struct Path
	{
		int flag;			//!< status flag
		int length;			//!< length of path char* buffer
		char* path;			//!< char* buffer for this path
	}Path;


	typedef struct List
	{
		int flag;			//!< status flag
		int size;			//!< number of files/directories in list
		Path** paths;		//!< Path array, each row contains the path of one file/directory
		int capacity;		//!< capacity of allocated Path arrays

	}List;

	void options_set(int force, int verbose, int quiet, int recursive);
	void tasks_set(int list, int rename);
	void run();

	int q_printf(const char* format, ...);
	void path_init(Path* path);
	void path_resize(Path* path, int size);
	void path_destroy(Path* path);

#ifdef __cplusplus
}
#endif
#endif

