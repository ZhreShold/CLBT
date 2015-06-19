/***********************************************************************/
/*
 *   Script File: clbt.c
 *
 *   Description:
 *
 *   Core modules for CLBT
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

#if defined(_MSC_VER) && _MSC_VER >= 1400
#define _CRT_SECURE_NO_WARNINGS /* suppress warnings about fopen() and similar "unsafe" functions defined by MS */
#endif

#include "clbt.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#ifndef CLBT_OS
#if defined(unix)        || defined(__unix)      || defined(__unix__) \
	|| defined(linux) || defined(__linux) || defined(__linux__) \
	|| defined(sun) || defined(__sun) \
	|| defined(BSD) || defined(__OpenBSD__) || defined(__NetBSD__) \
	|| defined(__FreeBSD__) || defined (__DragonFly__) \
	|| defined(sgi) || defined(__sgi) \
	|| (defined(__MACOSX__) || defined(__APPLE__)) \
	|| defined(__CYGWIN__) || defined(__MINGW32__)
#define CLBT_OS 1
#elif defined(_MSC_VER) || defined(WIN32)  || defined(_WIN32) || defined(__WIN32__) \
	|| defined(WIN64) || defined(_WIN64) || defined(__WIN64__)
#define CLBT_OS 0
#else
#error Unable to support this unknown OS.
#endif
#elif !(CLBT_OS==0 || CLBT_OS==1)
#error CLBT: Invalid configuration variable 'CLBT_OS'.
#error (correct values are '0 = Microsoft Windows', '1 = Unix-like OS').
#endif

/* OS specific headers */
#if CLBT_OS == 0
#include <Windows.h>
#include <direct.h>
#elif CLBT_OS == 1

/* Apple Mac_OS_X specific */
#if defined(__MACH__) || defined(__APPLE__)	
#include <mach/mach.h>
#include <mach/mach_time.h>
#endif

#include <unistd.h>	/* POSIX flags */
#include <time.h>	/* clock_gettime(), time() */
#include <sys/time.h>	/* gethrtime(), gettimeofday() */
#include <sys/stat.h>
#include <dirent.h>
#include <termios.h>
#include <fcntl.h>


#endif


struct ClbtPath
{
	int flag;			/* status flag */
	int length;			/* length of path char* buffer */
	char* path;			/* char* buffer for this path */
};


struct ClbtList
{
	int flag;					/* status flag */
	int size;					/* number of files/directories in list */
	int capacity;				/* capacity of allocated Path arrays */
	struct ClbtPath** paths;	/* Path array, each row contains the path of one file/directory */
};

typedef struct ClbtPath CP;
typedef struct ClbtList CL;

/*------------------------------------------------------------------------------------------------------*/
static FILE* stdOut = NULL;
static FILE* stdErr = NULL;
/*------------------------------------------------------------------------------------------------------*/

static void clbt_unused(const char* dull){ dull++; }

static int clbt_system(const char *const command, const char *const moduleName)
{
#if CLBT_OS==1
	const unsigned int l = strlen(command);
	clbt_unused(moduleName);
	if (l) 
	{
		int out_val = -1;
		char *const ncommand = (char*)malloc(l+16);
		strncpy(ncommand, command, l);
		strcpy(ncommand + l, " 2> /dev/null"); /* Make command silent. */
		out_val = system(ncommand);
		free(ncommand);
		return out_val;
	}
	else return -1;
#elif CLBT_OS==0
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	BOOL res;
	memset(&pi, 0, sizeof(PROCESS_INFORMATION));
	memset(&si, 0, sizeof(STARTUPINFO));
	GetStartupInfoA(&si);
	si.cb = sizeof(si);
	si.wShowWindow = SW_HIDE;
	si.dwFlags |= SW_HIDE | STARTF_USESHOWWINDOW;
	res = CreateProcessA((LPCTSTR)moduleName, (LPTSTR)command, 0, 0, FALSE, 0, 0, 0, &si, &pi);
	if (res) {
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
		return 0;
	}
	else return system(command);
#endif
}


static void clbt_enter_quiet_mode()
{
	/* open a null device, means disable printing any message */
	stdOut = fopen("nul", "w");
	if (stdOut == NULL)
	{
		fprintf(stderr, "[Warn] - Failed to enter quite mode.\n");
		stdOut = stdout;
		stdErr = stderr;
	}
	else
	{
		/* ok */
		stdErr = stdOut;
	}
}

static void clbt_exit_quiet_mode()
{
	if (stdOut != stdout && stdOut != NULL)
	{
		fclose(stdOut);
	}
	stdOut = stdout;
	stdErr = stderr;
}

static int clbt_end_with_newline(const char* str)
{
	int ret = 0;
	int len = strlen(str);
	if (len > 1)
	{
		if (str[len - 1] == 'n' && str[len - 2] == '\\') ret = 1;
	}
	return ret;
}

static void clbt_error(const char* format, ...)
{
	va_list args;

	assert(stdErr != NULL);
	fprintf(stdErr, "[Error] - ");
	va_start(args, format);
	vfprintf(stdErr, format, args);
	va_end(args);

	/* make it newline if not */
	if (!clbt_end_with_newline(format))
		fprintf(stdErr, "\n");
}

static void clbt_warning(const char* format, ...)
{
	va_list args;

	assert(stdErr != NULL);
	fprintf(stdErr, "[Warn] - ");
	va_start(args, format);
	vfprintf(stdErr, format, args);
	va_end(args);

	/* make it newline if not */
	if (!clbt_end_with_newline(format))
		fprintf(stdErr, "\n");
}

static void clbt_print(const char* format, ...)
{
	va_list args;

	assert(stdOut != NULL);
	va_start(args, format);
	vfprintf(stdOut, format, args);
	va_end(args);
}

static void clbt_println(const char* format, ...)
{
	va_list args;

	assert(stdOut != NULL);
	va_start(args, format);
	vfprintf(stdOut, format, args);
	va_end(args);

	/* make it newline if not */
	if (!clbt_end_with_newline(format))
		fprintf(stdOut, "\n");
}

/*
 * Init the specified path.
 */
static void clbt_path_init(CP* path)
{
	assert(path != NULL);

	path->flag = 0x1;
	path->length = 260;
	path->path = (char*)malloc(path->length);

	if (path->path == NULL)
	{
		clbt_error("Can't allocate memory for path!");
		exit(CLBT_MEMORY_ERR);
	}
}

/*
 * Destroy the specified path.
 */
static void clbt_path_destroy(CP* path)
{
	assert(path != NULL);
	assert(path->flag & 0x1);

	path->length = 0;
	path->flag = 0;
	free(path->path);
	path->path = NULL;
}

static void clbt_path_clear(CP* path)
{
	assert(path != NULL);

	if (path->flag & 0x1)
	{
		clbt_path_destroy(path);
	}

	clbt_path_init(path);
}

/*
 * Resizes the specified path.
 */
static void clbt_path_resize(CP* path, int size)
{
	assert(path != NULL);
	assert(size >= 1);

	if (path->flag != 0x1)
	{
		path->path = (char*)malloc(size);
		if (path->path == NULL)
		{
			clbt_error("Can't allocate memory for path!");
			exit(CLBT_MEMORY_ERR);
		}
		path->length = size;
	}
	else
	{
		char* buf = (char*)realloc(path->path, size);
		if (buf == NULL)
		{
			clbt_error("Can't reallocate memory for path!");
			exit(CLBT_MEMORY_ERR);
		}
		path->path = buf;
		path->length = size;
	}
}

/*
 * Init a List instance.
 */
static void clbt_list_init(CL* list)
{
	int i = 0;

	assert(list != NULL);

	list->flag = 0x1;
	list->size = 0;
	list->capacity = 100;
	list->paths = (CP**)malloc(sizeof(CP*)* list->capacity);

	if (list->paths == NULL)
	{
		clbt_error("Unable to allocate memory for paths in list");
		exit(CLBT_MEMORY_ERR);
	}

	/* NULL fill pointers */
	for (i = 0; i < list->capacity; i++)
	{
		list->paths[i] = NULL;
	}
}

/*
 * Destroy the specified list.
 */
static void clbt_list_destroy(CL* list)
{
	int i = 0;

	assert(list != NULL);
	assert(list->flag & 0x1);

	for (i = 0; i < list->size; i++)
	{
		clbt_path_destroy(list->paths[i]);
	}

	free(list->paths);
	list->paths = NULL;
	list->size = 0;
	list->capacity = 0;
	list->flag = 0;
}

/*
 * Increase the capacity of specified list by 2x.
 */
static void clbt_list_increase(CL* list)
{
	int i = 0;
	CP** buf = NULL;

	assert(list != NULL);
	assert(list->flag & 0x1);
	assert(list->size == list->capacity);
	assert(list->capacity > 0);

	list->capacity *= 2;
	buf = (CP**)realloc(list->paths, list->capacity * sizeof(CP*));

	if (buf == NULL)
	{
		clbt_error("Unable to reallocate memory for paths in list!");
		exit(CLBT_MEMORY_ERR);
	}
	else
	{
		/* ok */
		list->paths = buf;
		buf = NULL;
	}

	/* NULL fill new pointers */
	for (i = list->size; i < list->capacity; i++)
	{
		list->paths[i] = NULL;
	}
}

static void clbt_list_insert(CL* list, CP* path)
{
	if (list->size == list->capacity)
	{
		clbt_list_increase(list);
	}

	list->paths[list->size++] = path;
}


static int clbt_getcwd(CP* cwd)
{
	char* buffer = NULL;
	int size = 100;

	clbt_path_destroy(cwd);
	clbt_path_init(cwd);

#if defined(_WIN32) || defined(WIN32)
	if ((buffer = _getcwd(NULL, 0)) == NULL)
	{
		return CLBT_FAILURE_OS;
	}
	else
	{
		cwd->path = buffer;
		cwd->length = strlen(buffer);
		return CLBT_OK;
	}
#else
	while (1)
	{
		buffer = (char*)malloc(size);
		if (buffer == NULL)
			return CLBT_MEMORY_ERR;
		if (getcwd(buffer, size) == buffer)
			break;
		free(buffer);
		if (errno != ERANGE)
			return CLBT_FAILURE_OS;
		size *= 2;
	}

	cwd->path = buffer;
	cwd->length = size;
	return CLBT_OK;
#endif
}




/* 
 * main entrance for clbt tasks
 */
int clbt_run(int options, int tasks)
{
	if (options & CLBT_OPT_QUIET)
		clbt_enter_quiet_mode();
	else
		clbt_exit_quiet_mode();

	clbt_println("Start execution...");

	{
		CP cwd;
		clbt_path_init(&cwd);
		clbt_getcwd(&cwd);
		clbt_println("%s", cwd.path);
		clbt_path_destroy(&cwd);
	}

	clbt_exit_quiet_mode();
	return CLBT_OK;
}







