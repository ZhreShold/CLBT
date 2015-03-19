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
#include "core.h"
#include "argtable.h"
#include "utils.h"

#define HOLD_SCREEN 0
#define VERSION "0.0.1"

void hexit(int code)
{
#if HOLD_SCREEN
	hold_screen();
#endif
	exit(code);
}

void chkargs(int argc, char **argv)
{
	// use lower case for tasks, upper case letters for options
	struct arg_lit  *list = arg_lit0("l", "list", "list files");
	struct arg_lit  *recurse = arg_lit0("R", "recurse", "recurse through subdirectories");
	struct arg_lit  *help = arg_lit0(NULL, "help", "print this help and exit");
	struct arg_lit  *quiet = arg_lit0("Q", "quiet", "print nothing, even error information during task");
	struct arg_lit  *force = arg_lit0(NULL, "force", "force execution without confirmation");
	struct arg_lit  *verbose = arg_lit0("V", "verbose", "print debug information");
	struct arg_lit  *version = arg_lit0(NULL, "version", "print version information and exit");
	struct arg_lit  *rename = arg_lit0("r", "rename", "perform rename");
	struct arg_file *infile = arg_filen("i", NULL, NULL, 1, argc + 3, "input file format(s)");
	struct arg_file *outfile = arg_filen("o", NULL, NULL, 1, argc + 3, "output file format(s)");
	struct arg_end  *end = arg_end(20);

	void* argtable[1];
	const char* progname = argv[0];
	int nerrors;

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
	argtable[10] = outfile;
	argtable[11] = end;
	

	/* verify the argtable[] entries were allocated sucessfully */
	if (arg_nullcheck(argtable) != 0)
	{
		/* NULL entries were detected, some allocations must have failed */
		printf("%s: insufficient memory\n", progname);
		arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
		hexit(ERR_MEMORY);
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

		arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
		hexit(ERR_OK);
	}

	/* special case: '--version' takes precedence error reporting */
	if (version->count > 0)
	{
		printf("'%s' Command line batch tool, ver %s.\n", progname, VERSION);
		printf("Feb 2015, Joshua Zhang\n");
		arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
		hexit(ERR_OK);
	}

	/* If the parser returned any errors then display them and exit */
	if (nerrors > 0)
	{
		/* Display the error details contained in the arg_end struct.*/
		arg_print_errors(stdout, end, progname);
		printf("Try '%s --help' for more information.\n", progname);
		arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
		hexit(ERR_INVALID_OP);
	}

	/* special case: uname with no command line options induces brief help */
	if (argc == 1)
	{
		printf("Try '%s --help' for more information.\n", progname);
		arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
		hexit(ERR_OK);
	}

	/* set core routine options and tasks */
	options_set(force->count, verbose->count, quiet->count, recurse->count);
	tasks_set(list->count, rename->count);

	/* free argtable now */
	arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));

	/* start core routine */
	run();
}



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

int main(int argc, char **argv)
{
	chkargs(argc, argv);
	hold_screen();
	return ERR_OK;

}