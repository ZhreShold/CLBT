/***********************************************************************/
/*
/#   Script File: core.c
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


#include "core.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>


static Options g_options = { 0, 0, 0, 0 };
static Tasks g_tasks = { 0, 0 };

/// <summary>
/// Quite printf, print only when quite option is disabled
/// </summary>
/// <param name="format">The format.</param>
/// <param name="">The variable arguments.</param>
/// <returns>Number of printed characters.</returns>
int q_printf(const char* format, ...)
{
	int c = 0;
	va_list args;

	if (g_options.quiet != 0) return c;

	
	va_start(args, format);
	c = vprintf(format, args);
	va_end(args);

	return c;
}

/// <summary>
/// Reset global options.
/// </summary>
void options_reset()
{
	g_options.force = 0;
	g_options.recursive = 0;
	g_options.verbose = 0;
	g_options.quiet = 0;
}

void tasks_reset()
{
	g_tasks.list = 0;
	g_tasks.rename = 0;
}

void options_set(int force, int verbose, int quiet, int recursive)
{
	g_options.force = force > 0 ? 1: 0;
	g_options.verbose = verbose > 0 ? 1 : 0;
	g_options.quiet = quiet > 0 ? 1 : 0;
	g_options.recursive = recursive > 0 ? 1 : 0;
}

void tasks_set(int list, int rename)
{
	g_tasks.list = list > 0 ? 1 : 0;
	g_tasks.rename = rename > 0 ? 1 : 0;
}

void run()
{
	q_printf("Start execution...\n");
}


/// <summary>
/// Init a List instance.
/// </summary>
/// <returns>Pointer to the List instance</returns>
void list_init(List* list)
{
	if (list == NULL)
	{
		q_printf("Error: accessing NULL pointer!");
		exit(ERR_INVALID_OP);
	}

	int i = 0;

	if (list->flag == 0x1)
	{
		q_printf("Warning: list already initialized, do nothing here!");
		return;
	}

	list->flag = 0x1;
	list->size = 0;
	list->capacity = 100;

	list->paths = malloc(sizeof(Path*)* list->capacity);

	if (list->paths == NULL)
	{
		q_printf("Error: unable to allocate memory for paths");
		exit(ERR_MEMORY);
	}

	// NULL fill pointers
	for (i = 0; i < list->capacity; i++)
	{
		list->paths[i] = NULL;
	}

}

/// <summary>
/// Increase the capacity of specified list by 2x.
/// </summary>
/// <param name="list">The list.</param>
void list_increase(List* list)
{
	if (list == NULL)
	{
		q_printf("Error: accessing NULL pointer!");
		exit(ERR_INVALID_OP);
	}

	int i = 0;
	Path** buf = NULL;

	if (list->flag != 0x1)
	{
		q_printf("Warning: list not initialized!");
		list_init(list);
	}

	if (list->size != list->capacity)
	{
		q_printf("Error: not supposed to happen! Who triggered this function?");
		exit(ERR_INVALID_OP);
	}

	if (list->capacity <= 0)
	{
		q_printf("Error: trying to increase the size of empty list!");
		exit(ERR_INVALID_OP);
	}

	list->capacity *= 2;
	buf = realloc(list->paths, list->capacity * sizeof(Path*));

	if (buf == NULL)
	{
		q_printf("Error: unable to reallocate memory for paths");
		exit(ERR_MEMORY);
	}
	else
	{
		// ok
		list->paths = buf;
		buf = NULL;
	}

	// NULL fill new pointers
	for (i = list->size; i < list->capacity; i++)
	{
		list->paths[i] = NULL;
	}
}

/// <summary>
/// Destroy the specified list.
/// </summary>
/// <param name="list">The list.</param>
void list_destroy(List* list)
{
	if (list == NULL)
	{
		return;
	}

	if (list->flag != 0x1)
		return;

	int i = 0;
	for (i = 0; i < list->capacity; i++)
	{
		path_destroy(list->paths[i]);
	}
	free(list->paths);
	list->paths = NULL;
	list->size = 0;
	list->capacity = 0;
	list->flag = 0;
}

/// <summary>
/// Init the specified path.
/// </summary>
/// <param name="path">The path.</param>
void path_init(Path* path)
{
	if (path == NULL)
	{
		q_printf("Error: accessing NULL pointer!");
		exit(ERR_INVALID_OP);
	}

	if (path->flag == 0x1)
	{
		q_printf("Warning: path already initialized, do nothing here!");
		return;
	}

	path->flag = 0x1;
	path->length = 260;
	path->path = malloc(path->length);

	if (path->path == NULL)
	{
		q_printf("Error: can't allocate memory for path!");
		exit(ERR_MEMORY);
	}
	

	
}

/// <summary>
/// Resizes the specified path.
/// </summary>
/// <param name="path">The path structure.</param>
/// <param name="size">The new size.</param>
void path_resize(Path* path, int size)
{
	if (path == NULL)
	{
		q_printf("Error: accessing NULL pointer!");
		exit(ERR_INVALID_OP);
	}

	if (size < 1)
	{
		q_printf("Error: trying to resize to a non-positive size!");
		exit(ERR_INVALID_OP);
	}

	if (path->flag != 0x1)
	{
		
		path->path = malloc(size);
		if (path->path == NULL)
		{
			q_printf("Error: can't allocate memory for path!");
			exit(ERR_MEMORY);
		}
		path->length = size;
	}
	else
	{
		char* buf = realloc(path->path, size);
		if (buf == NULL)
		{
			q_printf("Error: can't reallocate memory for path!");
			exit(ERR_MEMORY);
		}
		path->path = buf;
		path->length = size;
	}


	
}

/// <summary>
/// Destroy the specified path.
/// </summary>
/// <param name="path">The path.</param>
void path_destroy(Path* path)
{
	if (path == NULL)
	{
		return;
	}

	if (path->flag != 0x1)
		return;

	path->length = 0;
	path->flag = 0;
	free(path->path);
	path->path = NULL;
}



