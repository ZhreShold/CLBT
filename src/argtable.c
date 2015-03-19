/*********************************************************************
This file is part of the argtable2 library.
Copyright (C) 1998-2001,2003-2011 Stewart Heitmann
sheitmann@users.sourceforge.net

The argtable2 library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

This software is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
USA.
**********************************************************************/

/**********************************************************************/
#include "argtable.h"

#if defined(_WIN32) || defined(WIN32)
#define STDC_HEADERS
#elif __APPLE__
#define STDC_HEADERS
#define HAVE_STRINGS_H
#define HAVE_UNISTD_H
#elif __linux
// linux
#define STDC_HEADERS
#define HAVE_UNISTD_H
#elif __unix // all unices not caught above
// Unix
#define STDC_HEADERS
#elif __posix
// POSIX
#define STDC_HEADERS
#endif
/**********************************************************************/

/* config.h must be included before anything else */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#endif

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
//#include "./getopt.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "argtable.h"
//#include "./getopt.h"

static
void arg_register_error(struct arg_end *end, void *parent, int error, const char *argval)
{
	/* printf("arg_register_error(%p,%p,%d,%s)\n",end,parent,error,argval); */
	if (end->count < end->hdr.maxcount)
	{
		end->error[end->count] = error;
		end->parent[end->count] = parent;
		end->argval[end->count] = argval;
		end->count++;
	}
	else
	{
		end->error[end->hdr.maxcount - 1] = ARG_ELIMIT;
		end->parent[end->hdr.maxcount - 1] = end;
		end->argval[end->hdr.maxcount - 1] = NULL;
	}
}


/*
* Return index of first table entry with a matching short option
* or -1 if no match was found.
*/
static
int find_shortoption(struct arg_hdr **table, char shortopt)
{
	int tabindex;
	for (tabindex = 0; !(table[tabindex]->flag&ARG_TERMINATOR); tabindex++)
	{
		if (table[tabindex]->shortopts && strchr(table[tabindex]->shortopts, shortopt))
			return tabindex;
	}
	return -1;
}


struct longoptions
{
	int getoptval;
	int noptions;
	struct option *options;
};

#ifndef NDEBUG
static
void dump_longoptions(struct longoptions* longoptions)
{
	int i;
	printf("getoptval = %d\n", longoptions->getoptval);
	printf("noptions  = %d\n", longoptions->noptions);
	for (i = 0; i<longoptions->noptions; i++)
	{
		printf("options[%d].name    = \"%s\"\n", i, longoptions->options[i].name);
		printf("options[%d].has_arg = %d\n", i, longoptions->options[i].has_arg);
		printf("options[%d].flag    = %p\n", i, longoptions->options[i].flag);
		printf("options[%d].val     = %d\n", i, longoptions->options[i].val);
	}
}
#endif

static
struct longoptions* alloc_longoptions(struct arg_hdr **table)
{
	struct longoptions *result;
	size_t nbytes;
	int noptions = 1;
	size_t longoptlen = 0;
	int tabindex;

	/*
	* Determine the total number of option structs required
	* by counting the number of comma separated long options
	* in all table entries and return the count in noptions.
	* note: noptions starts at 1 not 0 because we getoptlong
	* requires a NULL option entry to terminate the option array.
	* While we are at it, count the number of chars required
	* to store private copies of all the longoption strings
	* and return that count in logoptlen.
	*/
	tabindex = 0;
	do
	{
		const char *longopts = table[tabindex]->longopts;
		longoptlen += (longopts ? strlen(longopts) : 0) + 1;
		while (longopts)
		{
			noptions++;
			longopts = strchr(longopts + 1, ',');
		}
	} while (!(table[tabindex++]->flag&ARG_TERMINATOR));
	/*printf("%d long options consuming %d chars in total\n",noptions,longoptlen);*/


	/* allocate storage for return data structure as: */
	/* (struct longoptions) + (struct options)[noptions] + char[longoptlen] */
	nbytes = sizeof(struct longoptions)
		+ sizeof(struct option)*noptions
		+ longoptlen;
	result = (struct longoptions*)malloc(nbytes);
	if (result)
	{
		int option_index = 0;
		char *store;

		result->getoptval = 0;
		result->noptions = noptions;
		result->options = (struct option*)(result + 1);
		store = (char*)(result->options + noptions);

		for (tabindex = 0; !(table[tabindex]->flag&ARG_TERMINATOR); tabindex++)
		{
			const char *longopts = table[tabindex]->longopts;

			while (longopts && *longopts)
			{
				char *storestart = store;

				/* copy progressive longopt strings into the store */
				while (*longopts != 0 && *longopts != ',')
					*store++ = *longopts++;
				*store++ = 0;
				if (*longopts == ',')
					longopts++;
				/*fprintf(stderr,"storestart=\"%s\"\n",storestart);*/

				result->options[option_index].name = storestart;
				result->options[option_index].flag = &(result->getoptval);
				result->options[option_index].val = tabindex;
				if (table[tabindex]->flag & ARG_HASOPTVALUE)
					result->options[option_index].has_arg = 2;
				else if (table[tabindex]->flag & ARG_HASVALUE)
					result->options[option_index].has_arg = 1;
				else
					result->options[option_index].has_arg = 0;

				option_index++;
			}
		}
		/* terminate the options array with a zero-filled entry */
		result->options[option_index].name = 0;
		result->options[option_index].has_arg = 0;
		result->options[option_index].flag = 0;
		result->options[option_index].val = 0;
	}

	/*dump_longoptions(result);*/
	return result;
}

static
char* alloc_shortoptions(struct arg_hdr **table)
{
	char *result;
	size_t len = 2;
	int tabindex;

	/* determine the total number of option chars required */
	for (tabindex = 0; !(table[tabindex]->flag&ARG_TERMINATOR); tabindex++)
	{
		struct arg_hdr *hdr = table[tabindex];
		len += 3 * (hdr->shortopts ? strlen(hdr->shortopts) : 0);
	}

	result = malloc(len);
	if (result)
	{
		char *res = result;

		/* add a leading ':' so getopt return codes distinguish    */
		/* unrecognised option and options missing argument values */
		*res++ = ':';

		for (tabindex = 0; !(table[tabindex]->flag&ARG_TERMINATOR); tabindex++)
		{
			struct arg_hdr *hdr = table[tabindex];
			const char *shortopts = hdr->shortopts;
			while (shortopts && *shortopts)
			{
				*res++ = *shortopts++;
				if (hdr->flag & ARG_HASVALUE)
					*res++ = ':';
				if (hdr->flag & ARG_HASOPTVALUE)
					*res++ = ':';
			}
		}
		/* null terminate the string */
		*res = 0;
	}

	/*printf("alloc_shortoptions() returns \"%s\"\n",(result?result:"NULL"));*/
	return result;
}


/* return index of the table terminator entry */
static
int arg_endindex(struct arg_hdr **table)
{
	int tabindex = 0;
	while (!(table[tabindex]->flag&ARG_TERMINATOR))
		tabindex++;
	return tabindex;
}


static
void arg_parse_tagged(int argc, char **argv, struct arg_hdr **table, struct arg_end *endtable)
{
	struct longoptions *longoptions;
	char *shortoptions;
	int copt;

	/*printf("arg_parse_tagged(%d,%p,%p,%p)\n",argc,argv,table,endtable);*/

	/* allocate short and long option arrays for the given opttable[].   */
	/* if the allocs fail then put an error msg in the last table entry. */
	longoptions = alloc_longoptions(table);
	shortoptions = alloc_shortoptions(table);
	if (!longoptions || !shortoptions)
	{
		/* one or both memory allocs failed */
		arg_register_error(endtable, endtable, ARG_EMALLOC, NULL);
		/* free anything that was allocated (this is null safe) */
		free(shortoptions);
		free(longoptions);
		return;
	}

	/*dump_longoptions(longoptions);*/

	/* reset getopts internal option-index to zero, and disable error reporting */
	optind = 0;
	opterr = 0;

	/* fetch and process args using getopt_long */
	while ((copt = getopt_long(argc, argv, shortoptions, longoptions->options, NULL)) != -1)
	{
		/*
		printf("optarg='%s'\n",optarg);
		printf("optind=%d\n",optind);
		printf("copt=%c\n",(char)copt);
		printf("optopt=%c (%d)\n",optopt, (int)(optopt));
		*/
		switch (copt)
		{
		case 0:
		{
				  int tabindex = longoptions->getoptval;
				  void *parent = table[tabindex]->parent;
				  /*printf("long option detected from argtable[%d]\n", tabindex);*/
				  if (optarg && optarg[0] == 0 && (table[tabindex]->flag & ARG_HASVALUE))
				  {
					  /* printf(": long option %s requires an argument\n",argv[optind-1]); */
					  arg_register_error(endtable, endtable, ARG_EMISSARG, argv[optind - 1]);
					  /* continue to scan the (empty) argument value to enforce argument count checking */
				  }
				  if (table[tabindex]->scanfn)
				  {
					  int errorcode = table[tabindex]->scanfn(parent, optarg);
					  if (errorcode != 0)
						  arg_register_error(endtable, parent, errorcode, optarg);
				  }
		}
			break;

		case '?':
			/*
			* getopt_long() found an unrecognised short option.
			* if it was a short option its value is in optopt
			* if it was a long option then optopt=0
			*/
			switch (optopt)
			{
			case 0:
				/*printf("?0 unrecognised long option %s\n",argv[optind-1]);*/
				arg_register_error(endtable, endtable, ARG_ELONGOPT, argv[optind - 1]);
				break;
			default:
				/*printf("?* unrecognised short option '%c'\n",optopt);*/
				arg_register_error(endtable, endtable, optopt, NULL);
				break;
			}
			break;

		case':':
			/*
			* getopt_long() found an option with its argument missing.
			*/
			/*printf(": option %s requires an argument\n",argv[optind-1]); */
			arg_register_error(endtable, endtable, ARG_EMISSARG, argv[optind - 1]);
			break;

		default:
		{
				   /* getopt_long() found a valid short option */
				   int tabindex = find_shortoption(table, (char)copt);
				   /*printf("short option detected from argtable[%d]\n", tabindex);*/
				   if (tabindex == -1)
				   {
					   /* should never get here - but handle it just in case */
					   /*printf("unrecognised short option %d\n",copt);*/
					   arg_register_error(endtable, endtable, copt, NULL);
				   }
				   else
				   {
					   if (table[tabindex]->scanfn)
					   {
						   void *parent = table[tabindex]->parent;
						   int errorcode = table[tabindex]->scanfn(parent, optarg);
						   if (errorcode != 0)
							   arg_register_error(endtable, parent, errorcode, optarg);
					   }
				   }
				   break;
		}
		}
	}

	free(shortoptions);
	free(longoptions);
}


static
void arg_parse_untagged(int argc, char **argv, struct arg_hdr **table, struct arg_end *endtable)
{
	int tabindex = 0;
	int errorlast = 0;
	const char *optarglast = NULL;
	void *parentlast = NULL;

	/*printf("arg_parse_untagged(%d,%p,%p,%p)\n",argc,argv,table,endtable);*/
	while (!(table[tabindex]->flag&ARG_TERMINATOR))
	{
		void *parent;
		int errorcode;

		/* if we have exhausted our argv[optind] entries then we have finished */
		if (optind >= argc)
		{
			/*printf("arg_parse_untagged(): argv[] exhausted\n");*/
			return;
		}

		/* skip table entries with non-null long or short options (they are not untagged entries) */
		if (table[tabindex]->longopts || table[tabindex]->shortopts)
		{
			/*printf("arg_parse_untagged(): skipping argtable[%d] (tagged argument)\n",tabindex);*/
			tabindex++;
			continue;
		}

		/* skip table entries with NULL scanfn */
		if (!(table[tabindex]->scanfn))
		{
			/*printf("arg_parse_untagged(): skipping argtable[%d] (NULL scanfn)\n",tabindex);*/
			tabindex++;
			continue;
		}

		/* attempt to scan the current argv[optind] with the current     */
		/* table[tabindex] entry. If it succeeds then keep it, otherwise */
		/* try again with the next table[] entry.                        */
		parent = table[tabindex]->parent;
		errorcode = table[tabindex]->scanfn(parent, argv[optind]);
		if (errorcode == 0)
		{
			/* success, move onto next argv[optind] but stay with same table[tabindex] */
			/*printf("arg_parse_untagged(): argtable[%d] successfully matched\n",tabindex);*/
			optind++;

			/* clear the last tentative error */
			errorlast = 0;
		}
		else
		{
			/* failure, try same argv[optind] with next table[tabindex] entry */
			/*printf("arg_parse_untagged(): argtable[%d] failed match\n",tabindex);*/
			tabindex++;

			/* remember this as a tentative error we may wish to reinstate later */
			errorlast = errorcode;
			optarglast = argv[optind];
			parentlast = parent;
		}

	}

	/* if a tenative error still remains at this point then register it as a proper error */
	if (errorlast)
	{
		arg_register_error(endtable, parentlast, errorlast, optarglast);
		optind++;
	}

	/* only get here when not all argv[] entries were consumed */
	/* register an error for each unused argv[] entry */
	while (optind<argc)
	{
		/*printf("arg_parse_untagged(): argv[%d]=\"%s\" not consumed\n",optind,argv[optind]);*/
		arg_register_error(endtable, endtable, ARG_ENOMATCH, argv[optind++]);
	}

	return;
}


static
void arg_parse_check(struct arg_hdr **table, struct arg_end *endtable)
{
	int tabindex = 0;
	/* printf("arg_parse_check()\n"); */
	do
	{
		if (table[tabindex]->checkfn)
		{
			void *parent = table[tabindex]->parent;
			int errorcode = table[tabindex]->checkfn(parent);
			if (errorcode != 0)
				arg_register_error(endtable, parent, errorcode, NULL);
		}
	} while (!(table[tabindex++]->flag&ARG_TERMINATOR));
}


static
void arg_reset(void **argtable)
{
	struct arg_hdr **table = (struct arg_hdr**)argtable;
	int tabindex = 0;
	/*printf("arg_reset(%p)\n",argtable);*/
	do
	{
		if (table[tabindex]->resetfn)
			table[tabindex]->resetfn(table[tabindex]->parent);
	} while (!(table[tabindex++]->flag&ARG_TERMINATOR));
}


int arg_parse(int argc, char **argv, void **argtable)
{
	struct arg_hdr **table = (struct arg_hdr **)argtable;
	struct arg_end *endtable;
	int endindex;
	char **argvcopy = NULL;

	/*printf("arg_parse(%d,%p,%p)\n",argc,argv,argtable);*/

	/* reset any argtable data from previous invocations */
	arg_reset(argtable);

	/* locate the first end-of-table marker within the array */
	endindex = arg_endindex(table);
	endtable = (struct arg_end*)table[endindex];

	/* Special case of argc==0.  This can occur on Texas Instruments DSP. */
	/* Failure to trap this case results in an unwanted NULL result from  */
	/* the malloc for argvcopy (next code block).                         */
	if (argc == 0)
	{
		/* We must still perform post-parse checks despite the absence of command line arguments */
		arg_parse_check(table, endtable);

		/* Now we are finished */
		return endtable->count;
	}

	argvcopy = malloc(sizeof(char *)* argc);
	if (argvcopy)
	{
		int i;

		/*
		Fill in the local copy of argv[]. We need a local copy
		because getopt rearranges argv[] which adversely affects
		susbsequent parsing attempts.
		*/
		for (i = 0; i<argc; i++)
			argvcopy[i] = argv[i];

		/* parse the command line (local copy) for tagged options */
		arg_parse_tagged(argc, argvcopy, table, endtable);

		/* parse the command line (local copy) for untagged options */
		arg_parse_untagged(argc, argvcopy, table, endtable);

		/* if no errors so far then perform post-parse checks otherwise dont bother */
		if (endtable->count == 0)
			arg_parse_check(table, endtable);

		/* release the local copt of argv[] */
		free(argvcopy);
	}
	else
	{
		/* memory alloc failed */
		arg_register_error(endtable, endtable, ARG_EMALLOC, NULL);
	}

	return endtable->count;
}


/*
* Concatenate contents of src[] string onto *pdest[] string.
* The *pdest pointer is altered to point to the end of the
* target string and *pndest is decremented by the same number
* of chars.
* Does not append more than *pndest chars into *pdest[]
* so as to prevent buffer overruns.
* Its something like strncat() but more efficient for repeated
* calls on the same destination string.
* Example of use:
*   char dest[30] = "good"
*   size_t ndest = sizeof(dest);
*   char *pdest = dest;
*   arg_char(&pdest,"bye ",&ndest);
*   arg_char(&pdest,"cruel ",&ndest);
*   arg_char(&pdest,"world!",&ndest);
* Results in:
*   dest[] == "goodbye cruel world!"
*   ndest  == 10
*/
static
void arg_cat(char **pdest, const char *src, size_t *pndest)
{
	char *dest = *pdest;
	char *end = dest + *pndest;

	/*locate null terminator of dest string */
	while (dest<end && *dest != 0)
		dest++;

	/* concat src string to dest string */
	while (dest<end && *src != 0)
		*dest++ = *src++;

	/* null terminate dest string */
	*dest = 0;

	/* update *pdest and *pndest */
	*pndest = end - dest;
	*pdest = dest;
}


static
void arg_cat_option(char *dest, size_t ndest, const char *shortopts, const char *longopts, const char *datatype, int optvalue)
{
	if (shortopts)
	{
		char option[3];

		/* note: option array[] is initialiazed dynamically here to satisfy   */
		/* a deficiency in the watcom compiler wrt static array initializers. */
		option[0] = '-';
		option[1] = shortopts[0];
		option[2] = 0;

		arg_cat(&dest, option, &ndest);
		if (datatype)
		{
			arg_cat(&dest, " ", &ndest);
			if (optvalue)
			{
				arg_cat(&dest, "[", &ndest);
				arg_cat(&dest, datatype, &ndest);
				arg_cat(&dest, "]", &ndest);
			}
			else
				arg_cat(&dest, datatype, &ndest);
		}
	}
	else if (longopts)
	{
		size_t ncspn;

		/* add "--" tag prefix */
		arg_cat(&dest, "--", &ndest);

		/* add comma separated option tag */
		ncspn = strcspn(longopts, ",");
		strncat(dest, longopts, (ncspn<ndest) ? ncspn : ndest);

		if (datatype)
		{
			arg_cat(&dest, "=", &ndest);
			if (optvalue)
			{
				arg_cat(&dest, "[", &ndest);
				arg_cat(&dest, datatype, &ndest);
				arg_cat(&dest, "]", &ndest);
			}
			else
				arg_cat(&dest, datatype, &ndest);
		}
	}
	else if (datatype)
	{
		if (optvalue)
		{
			arg_cat(&dest, "[", &ndest);
			arg_cat(&dest, datatype, &ndest);
			arg_cat(&dest, "]", &ndest);
		}
		else
			arg_cat(&dest, datatype, &ndest);
	}
}

static
void arg_cat_optionv(char *dest, size_t ndest, const char *shortopts, const char *longopts, const char *datatype, int optvalue, const char *separator)
{
	separator = separator ? separator : "";

	if (shortopts)
	{
		const char *c = shortopts;
		while (*c)
		{
			/* "-a|-b|-c" */
			char shortopt[3];

			/* note: shortopt array[] is initialiazed dynamically here to satisfy */
			/* a deficiency in the watcom compiler wrt static array initializers. */
			shortopt[0] = '-';
			shortopt[1] = *c;
			shortopt[2] = 0;

			arg_cat(&dest, shortopt, &ndest);
			if (*++c)
				arg_cat(&dest, separator, &ndest);
		}
	}

	/* put separator between long opts and short opts */
	if (shortopts && longopts)
		arg_cat(&dest, separator, &ndest);

	if (longopts)
	{
		const char *c = longopts;
		while (*c)
		{
			size_t ncspn;

			/* add "--" tag prefix */
			arg_cat(&dest, "--", &ndest);

			/* add comma separated option tag */
			ncspn = strcspn(c, ",");
			strncat(dest, c, (ncspn<ndest) ? ncspn : ndest);
			c += ncspn;

			/* add given separator in place of comma */
			if (*c == ',')
			{
				arg_cat(&dest, separator, &ndest);
				c++;
			}
		}
	}

	if (datatype)
	{
		if (longopts)
			arg_cat(&dest, "=", &ndest);
		else if (shortopts)
			arg_cat(&dest, " ", &ndest);

		if (optvalue)
		{
			arg_cat(&dest, "[", &ndest);
			arg_cat(&dest, datatype, &ndest);
			arg_cat(&dest, "]", &ndest);
		}
		else
			arg_cat(&dest, datatype, &ndest);
	}
}


/* this function should be deprecated because it doesnt consider optional argument values (ARG_HASOPTVALUE) */
void arg_print_option(FILE *fp, const char *shortopts, const char *longopts, const char *datatype, const char *suffix)
{
	char syntax[200] = "";
	suffix = suffix ? suffix : "";

	/* there is no way of passing the proper optvalue for optional argument values here, so we must ignore it */
	arg_cat_optionv(syntax, sizeof(syntax), shortopts, longopts, datatype, 0, "|");

	fputs(syntax, fp);
	fputs(suffix, fp);
}


/*
* Print a GNU style [OPTION] string in which all short options that
* do not take argument values are presented in abbreviated form, as
* in: -xvfsd, or -xvf[sd], or [-xvsfd]
*/
static
void arg_print_gnuswitch(FILE *fp, struct arg_hdr **table)
{
	int tabindex;
	char *format1 = " -%c";
	char *format2 = " [-%c";
	char *suffix = "";

	/* print all mandatory switches that are without argument values */
	for (tabindex = 0; table[tabindex] && !(table[tabindex]->flag&ARG_TERMINATOR); tabindex++)
	{
		/* skip optional options */
		if (table[tabindex]->mincount<1)
			continue;

		/* skip non-short options */
		if (table[tabindex]->shortopts == NULL)
			continue;

		/* skip options that take argument values */
		if (table[tabindex]->flag&ARG_HASVALUE)
			continue;

		/* print the short option (only the first short option char, ignore multiple choices)*/
		fprintf(fp, format1, table[tabindex]->shortopts[0]);
		format1 = "%c";
		format2 = "[%c";
	}

	/* print all optional switches that are without argument values */
	for (tabindex = 0; table[tabindex] && !(table[tabindex]->flag&ARG_TERMINATOR); tabindex++)
	{
		/* skip mandatory args */
		if (table[tabindex]->mincount>0)
			continue;

		/* skip args without short options */
		if (table[tabindex]->shortopts == NULL)
			continue;

		/* skip args with values */
		if (table[tabindex]->flag&ARG_HASVALUE)
			continue;

		/* print first short option */
		fprintf(fp, format2, table[tabindex]->shortopts[0]);
		format2 = "%c";
		suffix = "]";
	}

	fprintf(fp, "%s", suffix);
}


void arg_print_syntax(FILE *fp, void **argtable, const char *suffix)
{
	struct arg_hdr **table = (struct arg_hdr**)argtable;
	int i, tabindex;

	/* print GNU style [OPTION] string */
	arg_print_gnuswitch(fp, table);

	/* print remaining options in abbreviated style */
	for (tabindex = 0; table[tabindex] && !(table[tabindex]->flag&ARG_TERMINATOR); tabindex++)
	{
		char syntax[200] = "";
		const char *shortopts, *longopts, *datatype;

		/* skip short options without arg values (they were printed by arg_print_gnu_switch) */
		if (table[tabindex]->shortopts && !(table[tabindex]->flag&ARG_HASVALUE))
			continue;

		shortopts = table[tabindex]->shortopts;
		longopts = table[tabindex]->longopts;
		datatype = table[tabindex]->datatype;
		arg_cat_option(syntax, sizeof(syntax), shortopts, longopts, datatype, table[tabindex]->flag&ARG_HASOPTVALUE);

		if (strlen(syntax)>0)
		{
			/* print mandatory instances of this option */
			for (i = 0; i<table[tabindex]->mincount; i++)
				fprintf(fp, " %s", syntax);

			/* print optional instances enclosed in "[..]" */
			switch (table[tabindex]->maxcount - table[tabindex]->mincount)
			{
			case 0:
				break;
			case 1:
				fprintf(fp, " [%s]", syntax);
				break;
			case 2:
				fprintf(fp, " [%s] [%s]", syntax, syntax);
				break;
			default:
				fprintf(fp, " [%s]...", syntax);
				break;
			}
		}
	}

	if (suffix)
		fprintf(fp, "%s", suffix);
}


void arg_print_syntaxv(FILE *fp, void **argtable, const char *suffix)
{
	struct arg_hdr **table = (struct arg_hdr**)argtable;
	int i, tabindex;

	/* print remaining options in abbreviated style */
	for (tabindex = 0; table[tabindex] && !(table[tabindex]->flag&ARG_TERMINATOR); tabindex++)
	{
		char syntax[200] = "";
		const char *shortopts, *longopts, *datatype;

		shortopts = table[tabindex]->shortopts;
		longopts = table[tabindex]->longopts;
		datatype = table[tabindex]->datatype;
		arg_cat_optionv(syntax, sizeof(syntax), shortopts, longopts, datatype, table[tabindex]->flag&ARG_HASOPTVALUE, "|");

		/* print mandatory options */
		for (i = 0; i<table[tabindex]->mincount; i++)
			fprintf(fp, " %s", syntax);

		/* print optional args enclosed in "[..]" */
		switch (table[tabindex]->maxcount - table[tabindex]->mincount)
		{
		case 0:
			break;
		case 1:
			fprintf(fp, " [%s]", syntax);
			break;
		case 2:
			fprintf(fp, " [%s] [%s]", syntax, syntax);
			break;
		default:
			fprintf(fp, " [%s]...", syntax);
			break;
		}
	}

	if (suffix)
		fprintf(fp, "%s", suffix);
}


void arg_print_glossary(FILE *fp, void **argtable, const char *format)
{
	struct arg_hdr **table = (struct arg_hdr**)argtable;
	int tabindex;

	format = format ? format : "  %-20s %s\n";
	for (tabindex = 0; !(table[tabindex]->flag&ARG_TERMINATOR); tabindex++)
	{
		if (table[tabindex]->glossary)
		{
			char syntax[200] = "";
			const char *shortopts = table[tabindex]->shortopts;
			const char *longopts = table[tabindex]->longopts;
			const char *datatype = table[tabindex]->datatype;
			const char *glossary = table[tabindex]->glossary;
			arg_cat_optionv(syntax, sizeof(syntax), shortopts, longopts, datatype, table[tabindex]->flag&ARG_HASOPTVALUE, ", ");
			fprintf(fp, format, syntax, glossary);
		}
	}
}


/**
* Print a piece of text formatted, which means in a column with a
* left and a right margin. The lines are wrapped at whitspaces next
* to right margin. The function does not indent the first line, but
* only the following ones.
*
* Example:
* arg_print_formatted( fp, 0, 5, "Some text that doesn't fit." )
* will result in the following output:
*
* Some
* text
* that
* doesn'
* t fit.
*
* Too long lines will be wrapped in the middle of a word.
*
* arg_print_formatted( fp, 2, 7, "Some text that doesn't fit." )
* will result in the following output:
*
* Some
*   text
*   that
*   doesn'
*   t fit.
*
* As you see, the first line is not indented. This enables output of
* lines, which start in a line where output already happened.
*
* Author: Uli Fouquet
*/
static
void arg_print_formatted(FILE *fp, const unsigned lmargin, const unsigned rmargin, const char *text)
{
	const unsigned textlen = strlen(text);
	unsigned line_start = 0;
	unsigned line_end = textlen + 1;
	const unsigned colwidth = (rmargin - lmargin) + 1;

	/* Someone doesn't like us... */
	if (line_end < line_start)
	{
		fprintf(fp, "%s\n", text);
	}

	while (line_end - 1 > line_start)
	{
		/* Eat leading whitespaces. This is essential because while
		wrapping lines, there will often be a whitespace at beginning
		of line  */
		while (isspace(*(text + line_start)))
		{
			line_start++;
		}

		if ((line_end - line_start) > colwidth)
		{
			line_end = line_start + colwidth;
		}

		/* Find last whitespace, that fits into line */
		while ((line_end > line_start)
			&& (line_end - line_start > colwidth)
			&& !isspace(*(text + line_end)))
		{
			line_end--;
		}

		/* Do not print trailing whitespace. If this text
		has got only one line, line_end now points to the
		last char due to initialization. */
		line_end--;

		/* Output line of text */
		while (line_start < line_end)
		{
			fputc(*(text + line_start), fp);
			line_start++;
		}
		fputc('\n', fp);

		/* Initialize another line */
		if (line_end + 1 < textlen)
		{
			unsigned i;

			for (i = 0; i < lmargin; i++)
			{
				fputc(' ', fp);
			}

			line_end = textlen;
		}

		/* If we have to print another line, get also the last char. */
		line_end++;

	} /* lines of text */
}

/**
* Prints the glossary in strict GNU format.
* Differences to arg_print_glossary() are:
*  - wraps lines after 80 chars
*  - indents lines without shortops
*  - does not accept formatstrings
*
* Contributed by Uli Fouquet
*/
void arg_print_glossary_gnu(FILE *fp, void **argtable)
{
	struct arg_hdr **table = (struct arg_hdr**)argtable;
	int tabindex;

	for (tabindex = 0; !(table[tabindex]->flag&ARG_TERMINATOR); tabindex++)
	{
		if (table[tabindex]->glossary)
		{
			char syntax[200] = "";
			const char *shortopts = table[tabindex]->shortopts;
			const char *longopts = table[tabindex]->longopts;
			const char *datatype = table[tabindex]->datatype;
			const char *glossary = table[tabindex]->glossary;

			if (!shortopts && longopts)
			{
				/* Indent trailing line by 4 spaces... */
				memset(syntax, ' ', 4);
				*(syntax + 4) = '\0';
			}

			arg_cat_optionv(syntax, sizeof(syntax), shortopts, longopts, datatype, table[tabindex]->flag&ARG_HASOPTVALUE, ", ");

			/* If syntax fits not into column, print glossary in new line... */
			if (strlen(syntax) > 25)
			{
				fprintf(fp, "  %-25s %s\n", syntax, "");
				*syntax = '\0';
			}

			fprintf(fp, "  %-25s ", syntax);
			arg_print_formatted(fp, 28, 79, glossary);
		}
	} /* for each table entry */

	fputc('\n', fp);
}


/**
* Checks the argtable[] array for NULL entries and returns 1
* if any are found, zero otherwise.
*/
int arg_nullcheck(void **argtable)
{
	struct arg_hdr **table = (struct arg_hdr **)argtable;
	int tabindex;
	/*printf("arg_nullcheck(%p)\n",argtable);*/

	if (!table)
		return 1;

	tabindex = 0;
	do
	{
		/*printf("argtable[%d]=%p\n",tabindex,argtable[tabindex]);*/
		if (!table[tabindex])
			return 1;
	} while (!(table[tabindex++]->flag&ARG_TERMINATOR));

	return 0;
}


/*
* arg_free() is deprecated in favour of arg_freetable() due to a flaw in its design.
* The flaw results in memory leak in the (very rare) case that an intermediate
* entry in the argtable array failed its memory allocation while others following
* that entry were still allocated ok. Those subsequent allocations will not be
* deallocated by arg_free().
* Despite the unlikeliness of the problem occurring, and the even unlikelier event
* that it has any deliterious effect, it is fixed regardless by replacing arg_free()
* with the newer arg_freetable() function.
* We still keep arg_free() for backwards compatibility.
*/
void arg_free(void **argtable)
{
	struct arg_hdr **table = (struct arg_hdr**)argtable;
	int tabindex = 0;
	int flag;
	/*printf("arg_free(%p)\n",argtable);*/
	do
	{
		/*
		if we encounter a NULL entry then somewhat incorrectly we presume
		we have come to the end of the array. It isnt strictly true because
		an intermediate entry could be NULL with other non-NULL entries to follow.
		The subsequent argtable entries would then not be freed as they should.
		*/
		if (table[tabindex] == NULL)
			break;

		flag = table[tabindex]->flag;
		free(table[tabindex]);
		table[tabindex++] = NULL;

	} while (!(flag&ARG_TERMINATOR));
}

/* frees each non-NULL element of argtable[], where n is the size of the number of entries in the array */
void arg_freetable(void **argtable, size_t n)
{
	struct arg_hdr **table = (struct arg_hdr**)argtable;
	size_t tabindex = 0;
	/*printf("arg_freetable(%p)\n",argtable);*/
	for (tabindex = 0; tabindex<n; tabindex++)
	{
		if (table[tabindex] == NULL)
			continue;

		free(table[tabindex]);
		table[tabindex] = NULL;
	};
}

/*********************************************************************
MODULE: arg_dbl
This file is part of the argtable2 library.
Copyright (C) 1998-2001,2003-2011 Stewart Heitmann
sheitmann@users.sourceforge.net

The argtable2 library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

This software is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
USA.
**********************************************************************/


/* local error codes */
enum { EMINCOUNT_DBL = 1, EMAXCOUNT_DBL, EBADDOUBLE };

static void resetfn_dbl(struct arg_dbl *parent)
{
	/*printf("%s:resetfn(%p)\n",__FILE__,parent);*/
	parent->count = 0;
}

static int scanfn_dbl(struct arg_dbl *parent, const char *argval)
{
	int errorcode = 0;

	if (parent->count == parent->hdr.maxcount)
	{
		/* maximum number of arguments exceeded */
		errorcode = EMAXCOUNT_DBL;
	}
	else if (!argval)
	{
		/* a valid argument with no argument value was given. */
		/* This happens when an optional argument value was invoked. */
		/* leave parent arguiment value unaltered but still count the argument. */
		parent->count++;
	}
	else
	{
		double val;
		char *end;

		/* extract double from argval into val */
		val = strtod(argval, &end);

		/* if success then store result in parent->dval[] array otherwise return error*/
		if (*end == 0)
			parent->dval[parent->count++] = val;
		else
			errorcode = EBADDOUBLE;
	}

	/*printf("%s:scanfn(%p) returns %d\n",__FILE__,parent,errorcode);*/
	return errorcode;
}

static int checkfn_dbl(struct arg_dbl *parent)
{
	int errorcode = (parent->count < parent->hdr.mincount) ? EMINCOUNT_DBL : 0;
	/*printf("%s:checkfn(%p) returns %d\n",__FILE__,parent,errorcode);*/
	return errorcode;
}

static void errorfn_dbl(struct arg_dbl *parent, FILE *fp, int errorcode, const char *argval, const char *progname)
{
	const char *shortopts = parent->hdr.shortopts;
	const char *longopts = parent->hdr.longopts;
	const char *datatype = parent->hdr.datatype;

	/* make argval NULL safe */
	argval = argval ? argval : "";

	fprintf(fp, "%s: ", progname);
	switch (errorcode)
	{
	case EMINCOUNT_DBL:
		fputs("missing option ", fp);
		arg_print_option(fp, shortopts, longopts, datatype, "\n");
		break;

	case EMAXCOUNT_DBL:
		fputs("excess option ", fp);
		arg_print_option(fp, shortopts, longopts, argval, "\n");
		break;

	case EBADDOUBLE:
		fprintf(fp, "invalid argument \"%s\" to option ", argval);
		arg_print_option(fp, shortopts, longopts, datatype, "\n");
		break;
	}
}


struct arg_dbl* arg_dbl0(const char* shortopts,
	const char* longopts,
	const char *datatype,
	const char *glossary)
{
	return arg_dbln(shortopts, longopts, datatype, 0, 1, glossary);
}

struct arg_dbl* arg_dbl1(const char* shortopts,
	const char* longopts,
	const char *datatype,
	const char *glossary)
{
	return arg_dbln(shortopts, longopts, datatype, 1, 1, glossary);
}


struct arg_dbl* arg_dbln(const char* shortopts,
	const char* longopts,
	const char *datatype,
	int mincount,
	int maxcount,
	const char *glossary)
{
	size_t nbytes;
	struct arg_dbl *result;

	/* foolproof things by ensuring maxcount is not less than mincount */
	maxcount = (maxcount<mincount) ? mincount : maxcount;

	nbytes = sizeof(struct arg_dbl)     /* storage for struct arg_dbl */
		+ (maxcount + 1) * sizeof(double);  /* storage for dval[maxcount] array plus one extra for padding to memory boundary */

	result = (struct arg_dbl*)malloc(nbytes);
	if (result)
	{
		size_t addr;
		size_t rem;

		/* init the arg_hdr struct */
		result->hdr.flag = ARG_HASVALUE;
		result->hdr.shortopts = shortopts;
		result->hdr.longopts = longopts;
		result->hdr.datatype = datatype ? datatype : "<double>";
		result->hdr.glossary = glossary;
		result->hdr.mincount = mincount;
		result->hdr.maxcount = maxcount;
		result->hdr.parent = result;
		result->hdr.resetfn = (arg_resetfn*)resetfn_dbl;
		result->hdr.scanfn = (arg_scanfn*)scanfn_dbl;
		result->hdr.checkfn = (arg_checkfn*)checkfn_dbl;
		result->hdr.errorfn = (arg_errorfn*)errorfn_dbl;

		/* Store the dval[maxcount] array on the first double boundary that immediately follows the arg_dbl struct. */
		/* We do the memory alignment purely for SPARC and Motorola systems. They require floats and doubles to be  */
		/* aligned on natural boundaries */
		addr = (size_t)(result + 1);
		rem = addr % sizeof(double);
		result->dval = (double*)(addr + sizeof(double)-rem);
		/* printf("addr=%p, dval=%p, sizeof(double)=%d rem=%d\n", addr, result->dval, (int)sizeof(double), (int)rem); */

		result->count = 0;
	}
	/*printf("arg_dbln() returns %p\n",result);*/
	return result;
}



/*********************************************************************
Module: arg_end
This file is part of the argtable2 library.
Copyright (C) 1998-2001,2003-2011 Stewart Heitmann
sheitmann@users.sourceforge.net

The argtable2 library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

This software is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
USA.
**********************************************************************/


static void resetfn_end(struct arg_end *parent)
{
	/*printf("%s:resetfn(%p)\n",__FILE__,parent);*/
	parent->count = 0;
}

static void errorfn_end(void *parent, FILE *fp, int error, const char *argval, const char *progname)
{
	progname = progname ? progname : "";
	argval = argval ? argval : "";

	fprintf(fp, "%s: ", progname);
	switch (error)
	{
	case ARG_ELIMIT:
		fputs("too many errors to display", fp);
		break;
	case ARG_EMALLOC:
		fputs("insufficent memory", fp);
		break;
	case ARG_ENOMATCH:
		fprintf(fp, "unexpected argument \"%s\"", argval);
		break;
	case ARG_EMISSARG:
		fprintf(fp, "option \"%s\" requires an argument", argval);
		break;
	case ARG_ELONGOPT:
		fprintf(fp, "invalid option \"%s\"", argval);
		break;
	default:
		fprintf(fp, "invalid option \"-%c\"", error);
		break;
	}
	fputc('\n', fp);
}


struct arg_end* arg_end(int maxcount)
{
	size_t nbytes;
	struct arg_end *result;

	nbytes = sizeof(struct arg_end)
		+ maxcount * sizeof(int)             /* storage for int error[maxcount] array*/
		+maxcount * sizeof(void*)           /* storage for void* parent[maxcount] array */
		+maxcount * sizeof(char*);          /* storage for char* argval[maxcount] array */

	result = (struct arg_end*)malloc(nbytes);
	if (result)
	{
		/* init the arg_hdr struct */
		result->hdr.flag = ARG_TERMINATOR;
		result->hdr.shortopts = NULL;
		result->hdr.longopts = NULL;
		result->hdr.datatype = NULL;
		result->hdr.glossary = NULL;
		result->hdr.mincount = 1;
		result->hdr.maxcount = maxcount;
		result->hdr.parent = result;
		result->hdr.resetfn = (arg_resetfn*)resetfn_end;
		result->hdr.scanfn = NULL;
		result->hdr.checkfn = NULL;
		result->hdr.errorfn = (arg_errorfn*)errorfn_end;

		/* store error[maxcount] array immediately after struct arg_end */
		result->error = (int*)(result + 1);

		/* store parent[maxcount] array immediately after error[] array */
		result->parent = (void**)(result->error + maxcount);

		/* store argval[maxcount] array immediately after parent[] array */
		result->argval = (const char**)(result->parent + maxcount);
	}

	/*printf("arg_end(%d) returns %p\n",maxcount,result);*/
	return result;
}


void arg_print_errors(FILE* fp, struct arg_end* end, const char* progname)
{
	int i;
	/*printf("arg_errors()\n");*/
	for (i = 0; i<end->count; i++)
	{
		struct arg_hdr *errorparent = (struct arg_hdr *)(end->parent[i]);
		if (errorparent->errorfn)
			errorparent->errorfn(end->parent[i], fp, end->error[i], end->argval[i], progname);
	}
}


/*********************************************************************
MODULE: arg_file
This file is part of the argtable2 library.
Copyright (C) 1998-2001,2003-2011 Stewart Heitmann
sheitmann@users.sourceforge.net

The argtable2 library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

This software is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
USA.
**********************************************************************/

#if defined(_WIN32) || defined(WIN32)
# define FILESEPARATOR1 '\\'
# define FILESEPARATOR2 '/'
#else
# define FILESEPARATOR1 '/'
# define FILESEPARATOR2 '/'
#endif

/* local error codes */
enum { EMINCOUNT_FILE = 1, EMAXCOUNT_FILE };


static void resetfn_file(struct arg_file *parent)
{
	/*printf("%s:resetfn(%p)\n",__FILE__,parent);*/
	parent->count = 0;
}


/* Returns ptr to the base filename within *filename */
static const char* arg_basename(const char *filename)
{
	const char *result = NULL, *result1, *result2;

	/* Find the last occurrence of eother file separator character. */
	/* Two alternative file separator chars are supported as legal  */
	/* file separators but not both together in the same filename.  */
	result1 = (filename ? strrchr(filename, FILESEPARATOR1) : NULL);
	result2 = (filename ? strrchr(filename, FILESEPARATOR2) : NULL);

	if (result2)
		result = result2 + 1;   /* using FILESEPARATOR2 (the alternative file separator) */

	if (result1)
		result = result1 + 1;   /* using FILESEPARATOR1 (the preferred file separator) */

	if (!result)
		result = filename;  /* neither file separator was found so basename is the whole filename */

	/* special cases of "." and ".." are not considered basenames */
	if (result && (strcmp(".", result) == 0 || strcmp("..", result) == 0))
		result = filename + strlen(filename);

	return result;
}


/* Returns ptr to the file extension within *basename */
static const char* arg_extension(const char *basename)
{
	/* find the last occurrence of '.' in basename */
	const char *result = (basename ? strrchr(basename, '.') : NULL);

	/* if no '.' was found then return pointer to end of basename */
	if (basename && !result)
		result = basename + strlen(basename);

	/* special case: basenames with a single leading dot (eg ".foo") are not considered as true extensions */
	if (basename && result == basename)
		result = basename + strlen(basename);

	/* special case: empty extensions (eg "foo.","foo..") are not considered as true extensions */
	if (basename && result && result[1] == '\0')
		result = basename + strlen(basename);

	return result;
}


static int scanfn_file(struct arg_file *parent, const char *argval)
{
	int errorcode = 0;

	if (parent->count == parent->hdr.maxcount)
	{
		/* maximum number of arguments exceeded */
		errorcode = EMAXCOUNT_FILE;
	}
	else if (!argval)
	{
		/* a valid argument with no argument value was given. */
		/* This happens when an optional argument value was invoked. */
		/* leave parent arguiment value unaltered but still count the argument. */
		parent->count++;
	}
	else
	{
		parent->filename[parent->count] = argval;
		parent->basename[parent->count] = arg_basename(argval);
		parent->extension[parent->count] = arg_extension(parent->basename[parent->count]); /* only seek extensions within the basename (not the file path)*/
		parent->count++;
	}

	/*printf("%s:scanfn(%p) returns %d\n",__FILE__,parent,errorcode);*/
	return errorcode;
}


static int checkfn_file(struct arg_file *parent)
{
	int errorcode = (parent->count < parent->hdr.mincount) ? EMINCOUNT_FILE : 0;
	/*printf("%s:checkfn(%p) returns %d\n",__FILE__,parent,errorcode);*/
	return errorcode;
}


static void errorfn_file(struct arg_file *parent, FILE *fp, int errorcode, const char *argval, const char *progname)
{
	const char *shortopts = parent->hdr.shortopts;
	const char *longopts = parent->hdr.longopts;
	const char *datatype = parent->hdr.datatype;

	/* make argval NULL safe */
	argval = argval ? argval : "";

	fprintf(fp, "%s: ", progname);
	switch (errorcode)
	{
	case EMINCOUNT_FILE:
		fputs("missing option ", fp);
		arg_print_option(fp, shortopts, longopts, datatype, "\n");
		break;

	case EMAXCOUNT_FILE:
		fputs("excess option ", fp);
		arg_print_option(fp, shortopts, longopts, argval, "\n");
		break;

	default:
		fprintf(fp, "unknown error at \"%s\"\n", argval);
	}
}


struct arg_file* arg_file0(const char* shortopts,
	const char* longopts,
	const char *datatype,
	const char *glossary)
{
	return arg_filen(shortopts, longopts, datatype, 0, 1, glossary);
}


struct arg_file* arg_file1(const char* shortopts,
	const char* longopts,
	const char *datatype,
	const char *glossary)
{
	return arg_filen(shortopts, longopts, datatype, 1, 1, glossary);
}


struct arg_file* arg_filen(const char* shortopts,
	const char* longopts,
	const char *datatype,
	int mincount,
	int maxcount,
	const char *glossary)
{
	size_t nbytes;
	struct arg_file *result;

	/* foolproof things by ensuring maxcount is not less than mincount */
	maxcount = (maxcount<mincount) ? mincount : maxcount;

	nbytes = sizeof(struct arg_file)     /* storage for struct arg_file */
		+ sizeof(char*)* maxcount    /* storage for filename[maxcount] array */
		+ sizeof(char*)* maxcount    /* storage for basename[maxcount] array */
		+ sizeof(char*)* maxcount;   /* storage for extension[maxcount] array */

	result = (struct arg_file*)malloc(nbytes);
	if (result)
	{
		int i;

		/* init the arg_hdr struct */
		result->hdr.flag = ARG_HASVALUE;
		result->hdr.shortopts = shortopts;
		result->hdr.longopts = longopts;
		result->hdr.glossary = glossary;
		result->hdr.datatype = datatype ? datatype : "<file>";
		result->hdr.mincount = mincount;
		result->hdr.maxcount = maxcount;
		result->hdr.parent = result;
		result->hdr.resetfn = (arg_resetfn*)resetfn_file;
		result->hdr.scanfn = (arg_scanfn*)scanfn_file;
		result->hdr.checkfn = (arg_checkfn*)checkfn_file;
		result->hdr.errorfn = (arg_errorfn*)errorfn_file;

		/* store the filename,basename,extension arrays immediately after the arg_file struct */
		result->filename = (const char**)(result + 1);
		result->basename = result->filename + maxcount;
		result->extension = result->basename + maxcount;
		result->count = 0;

		/* foolproof the string pointers by initialising them with empty strings */
		for (i = 0; i<maxcount; i++)
		{
			result->filename[i] = "";
			result->basename[i] = "";
			result->extension[i] = "";
		}
	}
	/*printf("arg_filen() returns %p\n",result);*/
	return result;
}




/*********************************************************************
MODULE: arg_int
This file is part of the argtable2 library.
Copyright (C) 1998-2001,2003-2011 Stewart Heitmann
sheitmann@users.sourceforge.net

The argtable2 library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

This software is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
USA.
**********************************************************************/

#include <limits.h>

/* local error codes */
enum { EMINCOUNT_INT = 1, EMAXCOUNT_INT, EBADINT, EOVERFLOW };

static void resetfn_int(struct arg_int *parent)
{
	/*printf("%s:resetfn(%p)\n",__FILE__,parent);*/
	parent->count = 0;
}

/* strtol0x() is like strtol() except that the numeric string is    */
/* expected to be prefixed by "0X" where X is a user supplied char. */
/* The string may optionally be prefixed by white space and + or -  */
/* as in +0X123 or -0X123.                                          */
/* Once the prefix has been scanned, the remainder of the numeric   */
/* string is converted using strtol() with the given base.          */
/* eg: to parse hex str="-0X12324", specify X='X' and base=16.      */
/* eg: to parse oct str="+0o12324", specify X='O' and base=8.       */
/* eg: to parse bin str="-0B01010", specify X='B' and base=2.       */
/* Failure of conversion is indicated by result where *endptr==str. */
static long int strtol0X(const char* str, const char **endptr, char X, int base)
{
	long int val;               /* stores result */
	int s = 1;                    /* sign is +1 or -1 */
	const char *ptr = str;        /* ptr to current position in str */

	/* skip leading whitespace */
	while (isspace(*ptr))
		ptr++;
	/* printf("1) %s\n",ptr); */

	/* scan optional sign character */
	switch (*ptr)
	{
	case '+':
		ptr++;
		s = 1;
		break;
	case '-':
		ptr++;
		s = -1;
		break;
	default:
		s = 1;
		break;
	}
	/* printf("2) %s\n",ptr); */

	/* '0X' prefix */
	if ((*ptr++) != '0')
	{
		/* printf("failed to detect '0'\n"); */
		*endptr = str;
		return 0;
	}
	/* printf("3) %s\n",ptr); */
	if (toupper(*ptr++) != toupper(X))
	{
		/* printf("failed to detect '%c'\n",X); */
		*endptr = str;
		return 0;
	}
	/* printf("4) %s\n",ptr); */

	/* attempt conversion on remainder of string using strtol() */
	val = strtol(ptr, (char**)endptr, base);
	if (*endptr == ptr)
	{
		/* conversion failed */
		*endptr = str;
		return 0;
	}

	/* success */
	return s*val;
}


/* Returns 1 if str matches suffix (case insensitive).    */
/* Str may contain trailing whitespace, but nothing else. */
static int detectsuffix(const char *str, const char *suffix)
{
	/* scan pairwise through strings until mismatch detected */
	while (toupper(*str) == toupper(*suffix))
	{
		/* printf("'%c' '%c'\n", *str, *suffix); */

		/* return 1 (success) if match persists until the string terminator */
		if (*str == '\0')
			return 1;

		/* next chars */
		str++;
		suffix++;
	}
	/* printf("'%c' '%c' mismatch\n", *str, *suffix); */

	/* return 0 (fail) if the matching did not consume the entire suffix */
	if (*suffix != 0)
		return 0;   /* failed to consume entire suffix */

	/* skip any remaining whitespace in str */
	while (isspace(*str))
		str++;

	/* return 1 (success) if we have reached end of str else return 0 (fail) */
	return (*str == '\0') ? 1 : 0;
}


static int scanfn_int(struct arg_int *parent, const char *argval)
{
	int errorcode = 0;

	if (parent->count == parent->hdr.maxcount)
	{
		/* maximum number of arguments exceeded */
		errorcode = EMAXCOUNT_INT;
	}
	else if (!argval)
	{
		/* a valid argument with no argument value was given. */
		/* This happens when an optional argument value was invoked. */
		/* leave parent arguiment value unaltered but still count the argument. */
		parent->count++;
	}
	else
	{
		long int val;
		const char *end;

		/* attempt to extract hex integer (eg: +0x123) from argval into val conversion */
		val = strtol0X(argval, &end, 'X', 16);
		if (end == argval)
		{
			/* hex failed, attempt octal conversion (eg +0o123) */
			val = strtol0X(argval, &end, 'O', 8);
			if (end == argval)
			{
				/* octal failed, attempt binary conversion (eg +0B101) */
				val = strtol0X(argval, &end, 'B', 2);
				if (end == argval)
				{
					/* binary failed, attempt decimal conversion with no prefix (eg 1234) */
					val = strtol(argval, (char**)&end, 10);
					if (end == argval)
					{
						/* all supported number formats failed */
						return EBADINT;
					}
				}
			}
		}

		/* Safety check for integer overflow. WARNING: this check    */
		/* achieves nothing on machines where size(int)==size(long). */
		if (val>INT_MAX || val<INT_MIN)
			errorcode = EOVERFLOW;

		/* Detect any suffixes (KB,MB,GB) and multiply argument value appropriately. */
		/* We need to be mindful of integer overflows when using such big numbers.   */
		if (detectsuffix(end, "KB"))             /* kilobytes */
		{
			if (val>(INT_MAX / 1024) || val<(INT_MIN / 1024))
				errorcode = EOVERFLOW;          /* Overflow would occur if we proceed */
			else
				val *= 1024;                      /* 1KB = 1024 */
		}
		else if (detectsuffix(end, "MB"))        /* megabytes */
		{
			if (val>(INT_MAX / 1048576) || val<(INT_MIN / 1048576))
				errorcode = EOVERFLOW;          /* Overflow would occur if we proceed */
			else
				val *= 1048576;                   /* 1MB = 1024*1024 */
		}
		else if (detectsuffix(end, "GB"))        /* gigabytes */
		{
			if (val>(INT_MAX / 1073741824) || val<(INT_MIN / 1073741824))
				errorcode = EOVERFLOW;          /* Overflow would occur if we proceed */
			else
				val *= 1073741824;                /* 1GB = 1024*1024*1024 */
		}
		else if (!detectsuffix(end, ""))
			errorcode = EBADINT;                /* invalid suffix detected */

		/* if success then store result in parent->ival[] array */
		if (errorcode == 0)
			parent->ival[parent->count++] = val;
	}

	/* printf("%s:scanfn(%p,%p) returns %d\n",__FILE__,parent,argval,errorcode); */
	return errorcode;
}

static int checkfn_int(struct arg_int *parent)
{
	int errorcode = (parent->count < parent->hdr.mincount) ? EMINCOUNT_INT : 0;
	/*printf("%s:checkfn(%p) returns %d\n",__FILE__,parent,errorcode);*/
	return errorcode;
}

static void errorfn_int(struct arg_int *parent, FILE *fp, int errorcode, const char *argval, const char *progname)
{
	const char *shortopts = parent->hdr.shortopts;
	const char *longopts = parent->hdr.longopts;
	const char *datatype = parent->hdr.datatype;

	/* make argval NULL safe */
	argval = argval ? argval : "";

	fprintf(fp, "%s: ", progname);
	switch (errorcode)
	{
	case EMINCOUNT_INT:
		fputs("missing option ", fp);
		arg_print_option(fp, shortopts, longopts, datatype, "\n");
		break;

	case EMAXCOUNT_INT:
		fputs("excess option ", fp);
		arg_print_option(fp, shortopts, longopts, argval, "\n");
		break;

	case EBADINT:
		fprintf(fp, "invalid argument \"%s\" to option ", argval);
		arg_print_option(fp, shortopts, longopts, datatype, "\n");
		break;

	case EOVERFLOW:
		fputs("integer overflow at option ", fp);
		arg_print_option(fp, shortopts, longopts, datatype, " ");
		fprintf(fp, "(%s is too large)\n", argval);
		break;
	}
}


struct arg_int* arg_int0(const char* shortopts,
	const char* longopts,
	const char *datatype,
	const char *glossary)
{
	return arg_intn(shortopts, longopts, datatype, 0, 1, glossary);
}

struct arg_int* arg_int1(const char* shortopts,
	const char* longopts,
	const char *datatype,
	const char *glossary)
{
	return arg_intn(shortopts, longopts, datatype, 1, 1, glossary);
}


struct arg_int* arg_intn(const char* shortopts,
	const char* longopts,
	const char *datatype,
	int mincount,
	int maxcount,
	const char *glossary)
{
	size_t nbytes;
	struct arg_int *result;

	/* foolproof things by ensuring maxcount is not less than mincount */
	maxcount = (maxcount<mincount) ? mincount : maxcount;

	nbytes = sizeof(struct arg_int)     /* storage for struct arg_int */
		+ maxcount * sizeof(int);    /* storage for ival[maxcount] array */

	result = (struct arg_int*)malloc(nbytes);
	if (result)
	{
		/* init the arg_hdr struct */
		result->hdr.flag = ARG_HASVALUE;
		result->hdr.shortopts = shortopts;
		result->hdr.longopts = longopts;
		result->hdr.datatype = datatype ? datatype : "<int>";
		result->hdr.glossary = glossary;
		result->hdr.mincount = mincount;
		result->hdr.maxcount = maxcount;
		result->hdr.parent = result;
		result->hdr.resetfn = (arg_resetfn*)resetfn_int;
		result->hdr.scanfn = (arg_scanfn*)scanfn_int;
		result->hdr.checkfn = (arg_checkfn*)checkfn_int;
		result->hdr.errorfn = (arg_errorfn*)errorfn_int;

		/* store the ival[maxcount] array immediately after the arg_int struct */
		result->ival = (int*)(result + 1);
		result->count = 0;
	}
	/*printf("arg_intn() returns %p\n",result);*/
	return result;
}


/*********************************************************************
MODULE: arg_lit
This file is part of the argtable2 library.
Copyright (C) 1998-2001,2003-2011 Stewart Heitmann
sheitmann@users.sourceforge.net

The argtable2 library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

This software is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
USA.
**********************************************************************/


/* local error codes */
enum { EMINCOUNT_LIT = 1, EMAXCOUNT_LIT };

static void resetfn_lit(struct arg_lit *parent)
{
	/*printf("%s:resetfn(%p)\n",__FILE__,parent);*/
	parent->count = 0;
}

static int scanfn_lit(struct arg_lit *parent, const char *argval)
{
	int errorcode = 0;
	if (parent->count < parent->hdr.maxcount)
		parent->count++;
	else
		errorcode = EMAXCOUNT_LIT;
	/*printf("%s:scanfn(%p,%s) returns %d\n",__FILE__,parent,argval,errorcode);*/
	return errorcode;
}

static int checkfn_lit(struct arg_lit *parent)
{
	int errorcode = (parent->count < parent->hdr.mincount) ? EMINCOUNT_LIT : 0;
	/*printf("%s:checkfn(%p) returns %d\n",__FILE__,parent,errorcode);*/
	return errorcode;
}

static void errorfn_lit(struct arg_lit *parent, FILE *fp, int errorcode, const char *argval, const char *progname)
{
	const char *shortopts = parent->hdr.shortopts;
	const char *longopts = parent->hdr.longopts;
	const char *datatype = parent->hdr.datatype;

	switch (errorcode)
	{
	case EMINCOUNT_LIT:
		fprintf(fp, "%s: missing option ", progname);
		arg_print_option(fp, shortopts, longopts, datatype, "\n");
		fprintf(fp, "\n");
		break;

	case EMAXCOUNT_LIT:
		fprintf(fp, "%s: extraneous option ", progname);
		arg_print_option(fp, shortopts, longopts, datatype, "\n");
		break;
	}
}

struct arg_lit* arg_lit0(const char* shortopts,
	const char* longopts,
	const char* glossary)
{
	return arg_litn(shortopts, longopts, 0, 1, glossary);
}

struct arg_lit* arg_lit1(const char* shortopts,
	const char* longopts,
	const char* glossary)
{
	return arg_litn(shortopts, longopts, 1, 1, glossary);
}


struct arg_lit* arg_litn(const char* shortopts,
	const char* longopts,
	int mincount,
	int maxcount,
	const char *glossary)
{
	struct arg_lit *result;

	/* foolproof things by ensuring maxcount is not less than mincount */
	maxcount = (maxcount<mincount) ? mincount : maxcount;

	result = (struct arg_lit*)malloc(sizeof(struct arg_lit));
	if (result)
	{
		/* init the arg_hdr struct */
		result->hdr.flag = 0;
		result->hdr.shortopts = shortopts;
		result->hdr.longopts = longopts;
		result->hdr.datatype = NULL;
		result->hdr.glossary = glossary;
		result->hdr.mincount = mincount;
		result->hdr.maxcount = maxcount;
		result->hdr.parent = result;
		result->hdr.resetfn = (arg_resetfn*)resetfn_lit;
		result->hdr.scanfn = (arg_scanfn*)scanfn_lit;
		result->hdr.checkfn = (arg_checkfn*)checkfn_lit;
		result->hdr.errorfn = (arg_errorfn*)errorfn_lit;

		/* init local variables */
		result->count = 0;
	}
	/*printf("arg_litn() returns %p\n",result);*/
	return result;
}


/*********************************************************************
MODULE: arg_rem
This file is part of the argtable2 library.
Copyright (C) 1998-2001,2003-2011 Stewart Heitmann
sheitmann@users.sourceforge.net

The argtable2 library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

This software is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
USA.
**********************************************************************/


struct arg_rem* arg_rem(const char *datatype,
	const char *glossary)
{
	struct arg_rem *result = (struct arg_rem*)malloc(sizeof(struct arg_rem));
	if (result)
	{
		/* init the arg_hdr struct */
		result->hdr.flag = 0;
		result->hdr.shortopts = NULL;
		result->hdr.longopts = NULL;
		result->hdr.datatype = datatype;
		result->hdr.glossary = glossary;
		result->hdr.mincount = 1;
		result->hdr.maxcount = 1;
		result->hdr.parent = result;
		result->hdr.resetfn = NULL;
		result->hdr.scanfn = NULL;
		result->hdr.checkfn = NULL;
		result->hdr.errorfn = NULL;
	}
	/*printf("arg_rem() returns %p\n",result);*/
	return result;
}


/*********************************************************************
MODULE: arg_str
This file is part of the argtable2 library.
Copyright (C) 1998-2001,2003-2011 Stewart Heitmann
sheitmann@users.sourceforge.net

The argtable2 library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

This software is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
USA.
**********************************************************************/

/* local error codes */
enum { EMINCOUNT_STR = 1, EMAXCOUNT_STR };

static void resetfn_str(struct arg_str *parent)
{
	/*printf("%s:resetfn(%p)\n",__FILE__,parent);*/
	parent->count = 0;
}

static int scanfn_str(struct arg_str *parent, const char *argval)
{
	int errorcode = 0;

	if (parent->count == parent->hdr.maxcount)
	{
		/* maximum number of arguments exceeded */
		errorcode = EMAXCOUNT_STR;
	}
	else if (!argval)
	{
		/* a valid argument with no argument value was given. */
		/* This happens when an optional argument value was invoked. */
		/* leave parent arguiment value unaltered but still count the argument. */
		parent->count++;
	}
	else
	{
		parent->sval[parent->count++] = argval;
	}

	/*printf("%s:scanfn(%p) returns %d\n",__FILE__,parent,errorcode);*/
	return errorcode;
}

static int checkfn_str(struct arg_str *parent)
{
	int errorcode = (parent->count < parent->hdr.mincount) ? EMINCOUNT_STR : 0;
	/*printf("%s:checkfn(%p) returns %d\n",__FILE__,parent,errorcode);*/
	return errorcode;
}

static void errorfn_str(struct arg_str *parent, FILE *fp, int errorcode, const char *argval, const char *progname)
{
	const char *shortopts = parent->hdr.shortopts;
	const char *longopts = parent->hdr.longopts;
	const char *datatype = parent->hdr.datatype;

	/* make argval NULL safe */
	argval = argval ? argval : "";

	fprintf(fp, "%s: ", progname);
	switch (errorcode)
	{
	case EMINCOUNT_STR:
		fputs("missing option ", fp);
		arg_print_option(fp, shortopts, longopts, datatype, "\n");
		break;

	case EMAXCOUNT_STR:
		fputs("excess option ", fp);
		arg_print_option(fp, shortopts, longopts, argval, "\n");
		break;
	}
}


struct arg_str* arg_str0(const char* shortopts,
	const char* longopts,
	const char *datatype,
	const char *glossary)
{
	return arg_strn(shortopts, longopts, datatype, 0, 1, glossary);
}

struct arg_str* arg_str1(const char* shortopts,
	const char* longopts,
	const char *datatype,
	const char *glossary)
{
	return arg_strn(shortopts, longopts, datatype, 1, 1, glossary);
}


struct arg_str* arg_strn(const char* shortopts,
	const char* longopts,
	const char *datatype,
	int mincount,
	int maxcount,
	const char *glossary)
{
	size_t nbytes;
	struct arg_str *result;

	/* foolproof things by ensuring maxcount is not less than mincount */
	maxcount = (maxcount<mincount) ? mincount : maxcount;

	nbytes = sizeof(struct arg_str)     /* storage for struct arg_str */
		+ maxcount * sizeof(char*);  /* storage for sval[maxcount] array */

	result = (struct arg_str*)malloc(nbytes);
	if (result)
	{
		int i;

		/* init the arg_hdr struct */
		result->hdr.flag = ARG_HASVALUE;
		result->hdr.shortopts = shortopts;
		result->hdr.longopts = longopts;
		result->hdr.datatype = datatype ? datatype : "<string>";
		result->hdr.glossary = glossary;
		result->hdr.mincount = mincount;
		result->hdr.maxcount = maxcount;
		result->hdr.parent = result;
		result->hdr.resetfn = (arg_resetfn*)resetfn_str;
		result->hdr.scanfn = (arg_scanfn*)scanfn_str;
		result->hdr.checkfn = (arg_checkfn*)checkfn_str;
		result->hdr.errorfn = (arg_errorfn*)errorfn_str;

		/* store the sval[maxcount] array immediately after the arg_str struct */
		result->sval = (const char**)(result + 1);
		result->count = 0;

		/* foolproof the string pointers by initialising them to reference empty strings */
		for (i = 0; i<maxcount; i++)
		{
			result->sval[i] = "";
		}
	}
	/*printf("arg_strn() returns %p\n",result);*/
	return result;
}


/* Getopt for GNU.
NOTE: getopt is now part of the C library, so if you don't know what
"Keep this file name-space clean" means, talk to roland@gnu.ai.mit.edu
before changing it!

Copyright (C) 1987, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97
Free Software Foundation, Inc.

This file is part of the GNU C Library.  Its master source is NOT part of
the C library, however.  The master source lives in /gd/gnu/lib.

The GNU C Library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The GNU C Library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with the GNU C Library; see the file COPYING.LIB.  If not,
write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* This tells Alpha OSF/1 not to define a getopt prototype in <stdio.h>.
Ditto for AIX 3.2 and <stdlib.h>.  */
#ifndef _NO_PROTO
#define _NO_PROTO
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#if !defined (__STDC__) || !__STDC__
/* This is a separate conditional since some stdc systems
reject `defined (const)'.  */
#ifndef const
#define const
#endif
#endif

#include <stdio.h>

/* Comment out all this code if we are using the GNU C Library, and are not
actually compiling the library itself.  This code is part of the GNU C
Library, but also included in many other GNU distributions.  Compiling
and linking in this code is a waste when using the GNU C library
(especially if it is a shared library).  Rather than having every GNU
program understand `configure --with-gnu-libc' and omit the object files,
it is simpler to just do this in the source for each such file.  */

#define GETOPT_INTERFACE_VERSION 2
#if !defined (_LIBC) && defined (__GLIBC__) && __GLIBC__ >= 2
#include <gnu-versions.h>
#if _GNU_GETOPT_INTERFACE_VERSION == GETOPT_INTERFACE_VERSION
#define ELIDE_CODE
#endif
#endif

#ifndef ELIDE_CODE

/* This needs to come after some library #include
to get __GNU_LIBRARY__ defined.  */
#ifdef	__GNU_LIBRARY__
/* Don't include stdlib.h for non-GNU C libraries because some of them
contain conflicting prototypes for getopt.  */
#include <stdlib.h>
#include <unistd.h>
#endif /* GNU C library.  */

#ifdef VMS
#include <unixlib.h>
#if HAVE_STRING_H - 0
#include <string.h>
#ifdef STRNCASECMP_IN_STRINGS_H
#   include <strings.h>
#endif
#endif
#endif

#if defined (WIN32) && !defined (__CYGWIN32__)
/* It's not Unix, really.  See?  Capital letters.  */
#include <windows.h>
#define getpid() GetCurrentProcessId()
#endif

#ifndef _
/* This is for other GNU distributions with internationalized messages.
When compiling libc, the _ macro is predefined.  */
#ifdef HAVE_LIBINTL_H
#include <libintl.h>
#define _(msgid)	gettext (msgid)
#else
#define _(msgid)	(msgid)
#endif
#endif

/* This version of `getopt' appears to the caller like standard Unix `getopt'
but it behaves differently for the user, since it allows the user
to intersperse the options with the other arguments.

As `getopt' works, it permutes the elements of ARGV so that,
when it is done, all the options precede everything else.  Thus
all application programs are extended to handle flexible argument order.

Setting the environment variable POSIXLY_CORRECT disables permutation.
Then the behavior is completely standard.

GNU application programs can use a third alternative mode in which
they can distinguish the relative order of options and other arguments.  */



/* For communication from `getopt' to the caller.
When `getopt' finds an option that takes an argument,
the argument value is returned here.
Also, when `ordering' is RETURN_IN_ORDER,
each non-option ARGV-element is returned here.  */

char *optarg = NULL;

/* Index in ARGV of the next element to be scanned.
This is used for communication to and from the caller
and for communication between successive calls to `getopt'.

On entry to `getopt', zero means this is the first call; initialize.

When `getopt' returns -1, this is the index of the first of the
non-option elements that the caller should itself scan.

Otherwise, `optind' communicates from one call to the next
how much of ARGV has been scanned so far.  */

/* 1003.2 says this must be 1 before any call.  */
int optind = 1;

/* Formerly, initialization of getopt depended on optind==0, which
causes problems with re-calling getopt as programs generally don't
know that. */

int __getopt_initialized = 0;

/* The next char to be scanned in the option-element
in which the last option character we returned was found.
This allows us to pick up the scan where we left off.

If this is zero, or a null string, it means resume the scan
by advancing to the next ARGV-element.  */

static char *nextchar;

/* Callers store zero here to inhibit the error message
for unrecognized options.  */

int opterr = 1;

/* Set to an option character which was unrecognized.
This must be initialized on some systems to avoid linking in the
system's own getopt implementation.  */

int optopt = '?';

/* Describe how to deal with options that follow non-option ARGV-elements.

If the caller did not specify anything,
the default is REQUIRE_ORDER if the environment variable
POSIXLY_CORRECT is defined, PERMUTE otherwise.

REQUIRE_ORDER means don't recognize them as options;
stop option processing when the first non-option is seen.
This is what Unix does.
This mode of operation is selected by either setting the environment
variable POSIXLY_CORRECT, or using `+' as the first character
of the list of option characters.

PERMUTE is the default.  We permute the contents of ARGV as we scan,
so that eventually all the non-options are at the end.  This allows options
to be given in any order, even with programs that were not written to
expect this.

RETURN_IN_ORDER is an option available to programs that were written
to expect options and other ARGV-elements in any order and that care about
the ordering of the two.  We describe each non-option ARGV-element
as if it were the argument of an option with character code 1.
Using `-' as the first character of the list of option characters
selects this mode of operation.

The special argument `--' forces an end of option-scanning regardless
of the value of `ordering'.  In the case of RETURN_IN_ORDER, only
`--' can cause `getopt' to return -1 with `optind' != ARGC.  */

static enum
{
	REQUIRE_ORDER, PERMUTE, RETURN_IN_ORDER
}
ordering;

/* Value of POSIXLY_CORRECT environment variable.  */
static char *posixly_correct;

#ifdef	__GNU_LIBRARY__
/* We want to avoid inclusion of string.h with non-GNU libraries
because there are many ways it can cause trouble.
On some systems, it contains special magic macros that don't work
in GCC.  */
#include <string.h>
#define	my_index	strchr
#else

/* Avoid depending on library functions or files
whose names are inconsistent.  */

char *getenv();

static char *
my_index(str, chr)
const char *str;
int chr;
{
	while (*str)
	{
		if (*str == chr)
			return (char *)str;
		str++;
	}
	return 0;
}

/* If using GCC, we can safely declare strlen this way.
If not using GCC, it is ok not to declare it.  */
#ifdef __GNUC__
/* Note that Motorola Delta 68k R3V7 comes with GCC but not stddef.h.
That was relevant to code that was here before.  */
#if !defined (__STDC__) || !__STDC__
/* gcc with -traditional declares the built-in strlen to return int,
and has done so at least since version 2.4.5. -- rms.  */
extern int strlen(const char *);

#endif /* not __STDC__ */
#endif /* __GNUC__ */

#endif /* not __GNU_LIBRARY__ */

/* Handle permutation of arguments.  */

/* Describe the part of ARGV that contains non-options that have
been skipped.  `first_nonopt' is the index in ARGV of the first of them;
`last_nonopt' is the index after the last of them.  */

static int first_nonopt;
static int last_nonopt;

#ifdef _LIBC
/* Bash 2.0 gives us an environment variable containing flags
indicating ARGV elements that should not be considered arguments.  */

static const char *nonoption_flags;
static int nonoption_flags_len;

static int original_argc;
static char *const *original_argv;

/* Make sure the environment variable bash 2.0 puts in the environment
is valid for the getopt call we must make sure that the ARGV passed
to getopt is that one passed to the process.  */
static void store_args(int argc, char *const *argv) __attribute__((unused));
static void
store_args(int argc, char *const *argv)
{
	/* XXX This is no good solution.  We should rather copy the args so
	that we can compare them later.  But we must not use malloc(3).  */
	original_argc = argc;
	original_argv = argv;
}
text_set_element(__libc_subinit, store_args);
#endif

/* Exchange two adjacent subsequences of ARGV.
One subsequence is elements [first_nonopt,last_nonopt)
which contains all the non-options that have been skipped so far.
The other is elements [last_nonopt,optind), which contains all
the options processed since those non-options were skipped.

`first_nonopt' and `last_nonopt' are relocated so that they describe
the new indices of the non-options in ARGV after they are moved.  */

#if defined (__STDC__) && __STDC__
static void exchange(char **);

#endif

static void
exchange(argv)
char **argv;
{
	int bottom = first_nonopt;
	int middle = last_nonopt;
	int top = optind;
	char *tem;

	/* Exchange the shorter segment with the far end of the longer segment.
	That puts the shorter segment into the right place.
	It leaves the longer segment in the right place overall,
	but it consists of two parts that need to be swapped next.  */

	while (top > middle && middle > bottom)
	{
		if (top - middle > middle - bottom)
		{
			/* Bottom segment is the short one.  */
			int len = middle - bottom;
			register int i;

			/* Swap it with the top part of the top segment.  */
			for (i = 0; i < len; i++)
			{
				tem = argv[bottom + i];
				argv[bottom + i] = argv[top - (middle - bottom) + i];
				argv[top - (middle - bottom) + i] = tem;
			}
			/* Exclude the moved bottom segment from further swapping.  */
			top -= len;
		}
		else
		{
			/* Top segment is the short one.  */
			int len = top - middle;
			register int i;

			/* Swap it with the bottom part of the bottom segment.  */
			for (i = 0; i < len; i++)
			{
				tem = argv[bottom + i];
				argv[bottom + i] = argv[middle + i];
				argv[middle + i] = tem;
			}
			/* Exclude the moved top segment from further swapping.  */
			bottom += len;
		}
	}

	/* Update records for the slots the non-options now occupy.  */

	first_nonopt += (optind - last_nonopt);
	last_nonopt = optind;
}

/* Initialize the internal data when the first call is made.  */

#if defined (__STDC__) && __STDC__
static const char *_getopt_initialize(int, char *const *, const char *);

#endif
static const char *
_getopt_initialize(argc, argv, optstring)
int argc;
char *const *argv;
const char *optstring;
{
	/* Start processing options with ARGV-element 1 (since ARGV-element 0
	is the program name); the sequence of previously skipped
	non-option ARGV-elements is empty.  */

	first_nonopt = last_nonopt = optind = 1;

	nextchar = NULL;

	posixly_correct = getenv("POSIXLY_CORRECT");

	/* Determine how to handle the ordering of options and nonoptions.  */

	if (optstring[0] == '-')
	{
		ordering = RETURN_IN_ORDER;
		++optstring;
	}
	else if (optstring[0] == '+')
	{
		ordering = REQUIRE_ORDER;
		++optstring;
	}
	else if (posixly_correct != NULL)
		ordering = REQUIRE_ORDER;
	else
		ordering = PERMUTE;

#ifdef _LIBC
	if (posixly_correct == NULL
		&& argc == original_argc && argv == original_argv)
	{
		/* Bash 2.0 puts a special variable in the environment for each
		command it runs, specifying which ARGV elements are the results of
		file name wildcard expansion and therefore should not be
		considered as options.  */
		char var[100];

		sprintf(var, "_%d_GNU_nonoption_argv_flags_", getpid());
		nonoption_flags = getenv(var);
		if (nonoption_flags == NULL)
			nonoption_flags_len = 0;
		else
			nonoption_flags_len = strlen(nonoption_flags);
	}
	else
		nonoption_flags_len = 0;
#endif

	return optstring;
}

/* Scan elements of ARGV (whose length is ARGC) for option characters
given in OPTSTRING.

If an element of ARGV starts with '-', and is not exactly "-" or "--",
then it is an option element.  The characters of this element
(aside from the initial '-') are option characters.  If `getopt'
is called repeatedly, it returns successively each of the option characters
from each of the option elements.

If `getopt' finds another option character, it returns that character,
updating `optind' and `nextchar' so that the next call to `getopt' can
resume the scan with the following option character or ARGV-element.

If there are no more option characters, `getopt' returns -1.
Then `optind' is the index in ARGV of the first ARGV-element
that is not an option.  (The ARGV-elements have been permuted
so that those that are not options now come last.)

OPTSTRING is a string containing the legitimate option characters.
If an option character is seen that is not listed in OPTSTRING,
return '?' after printing an error message.  If you set `opterr' to
zero, the error message is suppressed but we still return '?'.

If a char in OPTSTRING is followed by a colon, that means it wants an arg,
so the following text in the same ARGV-element, or the text of the following
ARGV-element, is returned in `optarg'.  Two colons mean an option that
wants an optional arg; if there is text in the current ARGV-element,
it is returned in `optarg', otherwise `optarg' is set to zero.

If OPTSTRING starts with `-' or `+', it requests different methods of
handling the non-option ARGV-elements.
See the comments about RETURN_IN_ORDER and REQUIRE_ORDER, above.

Long-named options begin with `--' instead of `-'.
Their names may be abbreviated as long as the abbreviation is unique
or is an exact match for some defined option.  If they have an
argument, it follows the option name in the same ARGV-element, separated
from the option name by a `=', or else the in next ARGV-element.
When `getopt' finds a long-named option, it returns 0 if that option's
`flag' field is nonzero, the value of the option's `val' field
if the `flag' field is zero.

The elements of ARGV aren't really const, because we permute them.
But we pretend they're const in the prototype to be compatible
with other systems.

LONGOPTS is a vector of `struct option' terminated by an
element containing a name which is zero.

LONGIND returns the index in LONGOPT of the long-named option found.
It is only valid when a long-named option has been found by the most
recent call.

If LONG_ONLY is nonzero, '-' as well as '--' can introduce
long-named options.  */

int
_getopt_internal(argc, argv, optstring, longopts, longind, long_only)
int argc;
char *const *argv;
const char *optstring;
const struct option *longopts;
int *longind;
int long_only;
{
	optarg = NULL;

	if (!__getopt_initialized || optind == 0)
	{
		optstring = _getopt_initialize(argc, argv, optstring);
		optind = 1;	/* Don't scan ARGV[0], the program name.  */
		__getopt_initialized = 1;
	}

	/* Test whether ARGV[optind] points to a non-option argument.
	Either it does not have option syntax, or there is an environment flag
	from the shell indicating it is not an option.  The later information
	is only used when the used in the GNU libc.  */
#ifdef _LIBC
#define NONOPTION_P (argv[optind][0] != '-' || argv[optind][1] == '\0'	      \
	|| (optind < nonoption_flags_len			      \
	&& nonoption_flags[optind] == '1'))
#else
#define NONOPTION_P (argv[optind][0] != '-' || argv[optind][1] == '\0')
#endif

	if (nextchar == NULL || *nextchar == '\0')
	{
		/* Advance to the next ARGV-element.  */

		/* Give FIRST_NONOPT & LAST_NONOPT rational values if OPTIND has been
		moved back by the user (who may also have changed the arguments).  */
		if (last_nonopt > optind)
			last_nonopt = optind;
		if (first_nonopt > optind)
			first_nonopt = optind;

		if (ordering == PERMUTE)
		{
			/* If we have just processed some options following some non-options,
			exchange them so that the options come first.  */

			if (first_nonopt != last_nonopt && last_nonopt != optind)
				exchange((char **)argv);
			else if (last_nonopt != optind)
				first_nonopt = optind;

			/* Skip any additional non-options
			and extend the range of non-options previously skipped.  */

			while (optind < argc && NONOPTION_P)
				optind++;
			last_nonopt = optind;
		}

		/* The special ARGV-element `--' means premature end of options.
		Skip it like a null option,
		then exchange with previous non-options as if it were an option,
		then skip everything else like a non-option.  */

		if (optind != argc && !strcmp(argv[optind], "--"))
		{
			optind++;

			if (first_nonopt != last_nonopt && last_nonopt != optind)
				exchange((char **)argv);
			else if (first_nonopt == last_nonopt)
				first_nonopt = optind;
			last_nonopt = argc;

			optind = argc;
		}

		/* If we have done all the ARGV-elements, stop the scan
		and back over any non-options that we skipped and permuted.  */

		if (optind == argc)
		{
			/* Set the next-arg-index to point at the non-options
			that we previously skipped, so the caller will digest them.  */
			if (first_nonopt != last_nonopt)
				optind = first_nonopt;
			return -1;
		}

		/* If we have come to a non-option and did not permute it,
		either stop the scan or describe it to the caller and pass it by.  */

		if (NONOPTION_P)
		{
			if (ordering == REQUIRE_ORDER)
				return -1;
			optarg = argv[optind++];
			return 1;
		}

		/* We have found another option-ARGV-element.
		Skip the initial punctuation.  */

		nextchar = (argv[optind] + 1
			+ (longopts != NULL && argv[optind][1] == '-'));
	}

	/* Decode the current option-ARGV-element.  */

	/* Check whether the ARGV-element is a long option.

	If long_only and the ARGV-element has the form "-f", where f is
	a valid short option, don't consider it an abbreviated form of
	a long option that starts with f.  Otherwise there would be no
	way to give the -f short option.

	On the other hand, if there's a long option "fubar" and
	the ARGV-element is "-fu", do consider that an abbreviation of
	the long option, just like "--fu", and not "-f" with arg "u".

	This distinction seems to be the most useful approach.  */

	if (longopts != NULL
		&& (argv[optind][1] == '-'
		|| (long_only && (argv[optind][2] || !my_index(optstring, argv[optind][1])))))
	{
		char *nameend;
		const struct option *p;
		const struct option *pfound = NULL;
		int exact = 0;
		int ambig = 0;
		int indfound = -1;
		int option_index;

		for (nameend = nextchar; *nameend && *nameend != '='; nameend++)
			/* Do nothing.  */;

		/* Test all long options for either exact match
		or abbreviated matches.  */
		for (p = longopts, option_index = 0; p->name; p++, option_index++)
		if (!strncmp(p->name, nextchar, nameend - nextchar))
		{
			if ((unsigned int)(nameend - nextchar)
				== (unsigned int)strlen(p->name))
			{
				/* Exact match found.  */
				pfound = p;
				indfound = option_index;
				exact = 1;
				break;
			}
			else if (pfound == NULL)
			{
				/* First nonexact match found.  */
				pfound = p;
				indfound = option_index;
			}
			else
				/* Second or later nonexact match found.  */
				ambig = 1;
		}

		if (ambig && !exact)
		{
			if (opterr)
				fprintf(stderr, _("%s: option `%s' is ambiguous\n"),
				argv[0], argv[optind]);
			nextchar += strlen(nextchar);
			optind++;
			optopt = 0;
			return '?';
		}

		if (pfound != NULL)
		{
			option_index = indfound;
			optind++;
			if (*nameend)
			{
				/* Don't test has_arg with >, because some C compilers don't
				allow it to be used on enums.  */
				if (pfound->has_arg)
					optarg = nameend + 1;
				else
				{
					if (opterr)
					{
						if (argv[optind - 1][1] == '-')
							/* --option */
							fprintf(stderr,
							_("%s: option `--%s' doesn't allow an argument\n"),
							argv[0], pfound->name);
						else
							/* +option or -option */
							fprintf(stderr,
							_("%s: option `%c%s' doesn't allow an argument\n"),
							argv[0], argv[optind - 1][0], pfound->name);
					}

					nextchar += strlen(nextchar);

					optopt = pfound->val;
					return '?';
				}
			}
			else if (pfound->has_arg == 1)
			{
				if (optind < argc)
					optarg = argv[optind++];
				else
				{
					if (opterr)
						fprintf(stderr,
						_("%s: option `%s' requires an argument\n"),
						argv[0], argv[optind - 1]);
					nextchar += strlen(nextchar);
					optopt = pfound->val;
					return optstring[0] == ':' ? ':' : '?';
				}
			}
			nextchar += strlen(nextchar);
			if (longind != NULL)
				*longind = option_index;
			if (pfound->flag)
			{
				*(pfound->flag) = pfound->val;
				return 0;
			}
			return pfound->val;
		}

		/* Can't find it as a long option.  If this is not getopt_long_only,
		or the option starts with '--' or is not a valid short
		option, then it's an error.
		Otherwise interpret it as a short option.  */
		if (!long_only || argv[optind][1] == '-'
			|| my_index(optstring, *nextchar) == NULL)
		{
			if (opterr)
			{
				if (argv[optind][1] == '-')
					/* --option */
					fprintf(stderr, _("%s: unrecognized option `--%s'\n"),
					argv[0], nextchar);
				else
					/* +option or -option */
					fprintf(stderr, _("%s: unrecognized option `%c%s'\n"),
					argv[0], argv[optind][0], nextchar);
			}
			nextchar = (char *) "";
			optind++;
			optopt = 0;
			return '?';
		}
	}

	/* Look at and handle the next short option-character.  */

	{
		char c = *nextchar++;
		char *temp = my_index(optstring, c);

		/* Increment `optind' when we start to process its last character.  */
		if (*nextchar == '\0')
			++optind;

		if (temp == NULL || c == ':')
		{
			if (opterr)
			{
				if (posixly_correct)
					/* 1003.2 specifies the format of this message.  */
					fprintf(stderr, _("%s: illegal option -- %c\n"),
					argv[0], c);
				else
					fprintf(stderr, _("%s: invalid option -- %c\n"),
					argv[0], c);
			}
			optopt = c;
			return '?';
		}
		/* Convenience. Treat POSIX -W foo same as long option --foo */
		if (temp[0] == 'W' && temp[1] == ';')
		{
			char *nameend;
			const struct option *p;
			const struct option *pfound = NULL;
			int exact = 0;
			int ambig = 0;
			int indfound = 0;
			int option_index;

			/* This is an option that requires an argument.  */
			if (*nextchar != '\0')
			{
				optarg = nextchar;
				/* If we end this ARGV-element by taking the rest as an arg,
				we must advance to the next element now.  */
				optind++;
			}
			else if (optind == argc)
			{
				if (opterr)
				{
					/* 1003.2 specifies the format of this message.  */
					fprintf(stderr, _("%s: option requires an argument -- %c\n"),
						argv[0], c);
				}
				optopt = c;
				if (optstring[0] == ':')
					c = ':';
				else
					c = '?';
				return c;
			}
			else
				/* We already incremented `optind' once;
				increment it again when taking next ARGV-elt as argument.  */
				optarg = argv[optind++];

			/* optarg is now the argument, see if it's in the
			table of longopts.  */

			for (nextchar = nameend = optarg; *nameend && *nameend != '='; nameend++)
				/* Do nothing.  */;

			/* Test all long options for either exact match
			or abbreviated matches.  */
			for (p = longopts, option_index = 0; p->name; p++, option_index++)
			if (!strncmp(p->name, nextchar, nameend - nextchar))
			{
				if ((unsigned int)(nameend - nextchar) == strlen(p->name))
				{
					/* Exact match found.  */
					pfound = p;
					indfound = option_index;
					exact = 1;
					break;
				}
				else if (pfound == NULL)
				{
					/* First nonexact match found.  */
					pfound = p;
					indfound = option_index;
				}
				else
					/* Second or later nonexact match found.  */
					ambig = 1;
			}
			if (ambig && !exact)
			{
				if (opterr)
					fprintf(stderr, _("%s: option `-W %s' is ambiguous\n"),
					argv[0], argv[optind]);
				nextchar += strlen(nextchar);
				optind++;
				return '?';
			}
			if (pfound != NULL)
			{
				option_index = indfound;
				if (*nameend)
				{
					/* Don't test has_arg with >, because some C compilers don't
					allow it to be used on enums.  */
					if (pfound->has_arg)
						optarg = nameend + 1;
					else
					{
						if (opterr)
							fprintf(stderr, _("\
											  %s: option `-W %s' doesn't allow an argument\n"),
											  argv[0], pfound->name);

						nextchar += strlen(nextchar);
						return '?';
					}
				}
				else if (pfound->has_arg == 1)
				{
					if (optind < argc)
						optarg = argv[optind++];
					else
					{
						if (opterr)
							fprintf(stderr,
							_("%s: option `%s' requires an argument\n"),
							argv[0], argv[optind - 1]);
						nextchar += strlen(nextchar);
						return optstring[0] == ':' ? ':' : '?';
					}
				}
				nextchar += strlen(nextchar);
				if (longind != NULL)
					*longind = option_index;
				if (pfound->flag)
				{
					*(pfound->flag) = pfound->val;
					return 0;
				}
				return pfound->val;
			}
			nextchar = NULL;
			return 'W';	/* Let the application handle it.   */
		}
		if (temp[1] == ':')
		{
			if (temp[2] == ':')
			{
				/* This is an option that accepts an argument optionally.  */
				if (*nextchar != '\0')
				{
					optarg = nextchar;
					optind++;
				}
				else
					optarg = NULL;
				nextchar = NULL;
			}
			else
			{
				/* This is an option that requires an argument.  */
				if (*nextchar != '\0')
				{
					optarg = nextchar;
					/* If we end this ARGV-element by taking the rest as an arg,
					we must advance to the next element now.  */
					optind++;
				}
				else if (optind == argc)
				{
					if (opterr)
					{
						/* 1003.2 specifies the format of this message.  */
						fprintf(stderr,
							_("%s: option requires an argument -- %c\n"),
							argv[0], c);
					}
					optopt = c;
					if (optstring[0] == ':')
						c = ':';
					else
						c = '?';
				}
				else
					/* We already incremented `optind' once;
					increment it again when taking next ARGV-elt as argument.  */
					optarg = argv[optind++];
				nextchar = NULL;
			}
		}
		return c;
	}
}

int
getopt(argc, argv, optstring)
int argc;
char *const *argv;
const char *optstring;
{
	return _getopt_internal(argc, argv, optstring,
		(const struct option *) 0,
		(int *)0,
		0);
}

#endif /* Not ELIDE_CODE.  */

#ifdef TEST

/* Compile with -DTEST to make an executable for use in testing
the above definition of `getopt'.  */

int
main(argc, argv)
int argc;
char **argv;
{
	int c;
	int digit_optind = 0;

	while (1)
	{
		int this_option_optind = optind ? optind : 1;

		c = getopt(argc, argv, "abc:d:0123456789");
		if (c == -1)
			break;

		switch (c)
		{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			if (digit_optind != 0 && digit_optind != this_option_optind)
				printf("digits occur in two different argv-elements.\n");
			digit_optind = this_option_optind;
			printf("option %c\n", c);
			break;

		case 'a':
			printf("option a\n");
			break;

		case 'b':
			printf("option b\n");
			break;

		case 'c':
			printf("option c with value `%s'\n", optarg);
			break;

		case '?':
			break;

		default:
			printf("?? getopt returned character code 0%o ??\n", c);
		}
	}

	if (optind < argc)
	{
		printf("non-option ARGV-elements: ");
		while (optind < argc)
			printf("%s ", argv[optind++]);
		printf("\n");
	}

	exit(0);
}

#endif /* TEST */


/* getopt_long and getopt_long_only entry points for GNU getopt.
Copyright (C) 1987,88,89,90,91,92,93,94,96,97 Free Software Foundation, Inc.

This file is part of the GNU C Library.  Its master source is NOT part of
the C library, however.  The master source lives in /gd/gnu/lib.

The GNU C Library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The GNU C Library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with the GNU C Library; see the file COPYING.LIB.  If not,
write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#if !defined (__STDC__) || !__STDC__
/* This is a separate conditional since some stdc systems
reject `defined (const)'.  */
#ifndef const
#define const
#endif
#endif

#include <stdio.h>

/* Comment out all this code if we are using the GNU C Library, and are not
actually compiling the library itself.  This code is part of the GNU C
Library, but also included in many other GNU distributions.  Compiling
and linking in this code is a waste when using the GNU C library
(especially if it is a shared library).  Rather than having every GNU
program understand `configure --with-gnu-libc' and omit the object files,
it is simpler to just do this in the source for each such file.  */

#define GETOPT_INTERFACE_VERSION 2
#if !defined (_LIBC) && defined (__GLIBC__) && __GLIBC__ >= 2
#include <gnu-versions.h>
#if _GNU_GETOPT_INTERFACE_VERSION == GETOPT_INTERFACE_VERSION
#define ELIDE_CODE
#endif
#endif

#ifndef ELIDE_CODE

/* This needs to come after some library #include
to get __GNU_LIBRARY__ defined.  */
#ifdef __GNU_LIBRARY__
#include <stdlib.h>
#endif

#ifndef	NULL
#define NULL 0
#endif

int
getopt_long(argc, argv, options, long_options, opt_index)
int argc;
char *const *argv;
const char *options;
const struct option *long_options;
int *opt_index;
{
	return _getopt_internal(argc, argv, options, long_options, opt_index, 0);
}

/* Like getopt_long, but '-' as well as '--' can indicate a long option.
If an option that starts with '-' (not '--') doesn't match a long option,
but does match a short option, it is parsed as a short option
instead.  */

int
getopt_long_only(argc, argv, options, long_options, opt_index)
int argc;
char *const *argv;
const char *options;
const struct option *long_options;
int *opt_index;
{
	return _getopt_internal(argc, argv, options, long_options, opt_index, 1);
}

#endif /* Not ELIDE_CODE.  */

