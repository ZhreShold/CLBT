/***********************************************************************/
/*
/#   Script File: main.c
/#
/#   Description:
/#
/#   Main entrance for CLBT
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
#include <stdlib.h>
#include "clbt.h"
#include "argtable.h"

#define VERSION "0.0.1"
#define AUTHOR "Joshua Z. Zhang - Feb 2015"


void chkargs(int argc, char **argv)
{
	/* use lower case for tasks, upper case letters for options */
	struct arg_lit  *list = arg_lit0("l", "list", "list files");
	struct arg_lit  *recurse = arg_lit0("R", "recurse", "recurse through subdirectories");
	struct arg_lit  *help = arg_lit0(NULL, "help", "print this help and exit");
	struct arg_lit  *quiet = arg_lit0("Q", "quiet", "print nothing, even error information during task");
	struct arg_lit  *force = arg_lit0(NULL, "force", "force execution without confirmation");
	struct arg_lit  *verbose = arg_lit0("V", "verbose", "print debug information");
	struct arg_lit  *version = arg_lit0(NULL, "version", "print version information and exit");
	struct arg_lit  *rename = arg_lit0("r", "rename", "perform rename");
	struct arg_str	*infile = arg_strn("i", "infile", "filename", 0, argc + 2, "input filename pattern");
	struct arg_end  *end = arg_end(20);

	void* argtable[10];
	const char* progname = argv[0];
	int nerrors;
	int clbtOptions = CLBT_OPT_DEFAULT;
	int clbtTasks = CLBT_TASK_DEFAULT;

	/* initialize the argtable array with ptrs to the arg_xxx structures constructed above */
	argtable[0] = list;
	argtable[1] = recurse;
	argtable[2] = help;
	argtable[3] = quiet;
	argtable[4] = force;
	argtable[5] = verbose;
	argtable[6] = version;
	argtable[7] = rename;
	argtable[8] = infile;
	argtable[9] = end;
	

	/* verify the argtable[] entries were allocated sucessfully */
	if (arg_nullcheck(argtable) != 0)
	{
		/* NULL entries were detected, some allocations must have failed */
		printf("%s: insufficient memory\n", progname);
		arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
		exit(CLBT_MEMORY_ERR);
	}

	/* set any command line default values prior to parsing */

	/* Parse the command line as defined by argtable[] */
	nerrors = arg_parse(argc, argv, argtable);

	/* special case: '--help' takes precedence over error reporting */
	if (help->count > 0)
	{
		printf("Usage: %s", progname);
		arg_print_syntax(stdout, argtable, "\n");
		printf("This program aim to provide easier way to batch executions.\n");
		printf("Cross-platform supported.\n");
		arg_print_glossary(stdout, argtable, "  %-25s %s\n");
		printf("\n\n");

		arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
		exit(CLBT_OK);
	}

	/* special case: '--version' takes precedence error reporting */
	if (version->count > 0)
	{
		printf("'%s' Command line batch tool, ver %s.\n", progname, VERSION);
		printf("%s\n", AUTHOR);
		arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
		exit(CLBT_OK);
	}

	/* If the parser returned any errors then display them and exit */
	if (nerrors > 0)
	{
		/* Display the error details contained in the arg_end struct.*/
		arg_print_errors(stdout, end, progname);
		printf("Try '%s --help' for more information.\n", progname);
		arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
		exit(CLBT_INVALID_OP);
	}

	/* special case: uname with no command line options induces brief help */
	if (argc == 1)
	{
		printf("Try '%s --help' for more information.\n", progname);
		arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
		exit(CLBT_OK);
	}

	/* set core routine options */
	if (force->count) clbtOptions |= CLBT_OPT_FORCE;
	if (verbose->count) clbtOptions |= CLBT_OPT_VERBOSE;
	if (quiet->count) clbtOptions |= CLBT_OPT_QUIET;
	if (recurse->count) clbtOptions |= CLBT_OPT_RECURSIVE;
	
	/* set core routine tasks */
	if (list->count) clbtTasks |= CLBT_TASK_LIST;
	if (rename->count) clbtTasks |= CLBT_TASK_RENAME;

	/* free argtable now */
	arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));

	/* start core routine */
	clbt_run(clbtOptions, clbtTasks);
}


/*
void test0()
{
	Path p;
	path_init(&p);
	int code = x_getcwd(&p);
	printf("Code: %d\n", code);
	printf("path: %s\n", p.path);
	printf("path length: %d\n", p.length);
	path_destroy(&p);

	hold_screen();
	exit(0);
}
*/

int main(int argc, char **argv)
{
	chkargs(argc, argv);
	return CLBT_OK;
}