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
#ifndef ARGTABLE2
#define ARGTABLE2

#if defined(_MSC_VER) && _MSC_VER >= 1400
#define _CRT_SECURE_NO_WARNINGS // suppress warnings about fopen() and similar "unsafe" functions defined by MS
#endif

#include <stdio.h>      /* FILE */
#include <time.h>       /* struct tm */
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif


	/* bit masks for arg_hdr.flag */
	enum
	{
		ARG_TERMINATOR = 0x1,
		ARG_HASVALUE = 0x2,
		ARG_HASOPTVALUE = 0x4
	};

	typedef void (arg_resetfn)(void *parent);
	typedef int  (arg_scanfn)(void *parent, const char *argval);
	typedef int  (arg_checkfn)(void *parent);
	typedef void (arg_errorfn)(void *parent, FILE *fp, int error, const char *argval, const char *progname);


	/*
	* The arg_hdr struct defines properties that are common to all arg_xxx structs.
	* The argtable library requires each arg_xxx struct to have an arg_hdr
	* struct as its first data member.
	* The argtable library functions then use this data to identify the
	* properties of the command line option, such as its option tags,
	* datatype string, and glossary strings, and so on.
	* Moreover, the arg_hdr struct contains pointers to custom functions that
	* are provided by each arg_xxx struct which perform the tasks of parsing
	* that particular arg_xxx arguments, performing post-parse checks, and
	* reporting errors.
	* These functions are private to the individual arg_xxx source code
	* and are the pointer to them are initiliased by that arg_xxx struct's
	* constructor function. The user could alter them after construction
	* if desired, but the original intention is for them to be set by the
	* constructor and left unaltered.
	*/
	struct arg_hdr
	{
		char         flag;        /* Modifier flags: ARG_TERMINATOR, ARG_HASVALUE. */
		const char  *shortopts;   /* String defining the short options */
		const char  *longopts;    /* String defiing the long options */
		const char  *datatype;    /* Description of the argument data type */
		const char  *glossary;    /* Description of the option as shown by arg_print_glossary function */
		int          mincount;    /* Minimum number of occurences of this option accepted */
		int          maxcount;    /* Maximum number of occurences if this option accepted */
		void        *parent;      /* Pointer to parent arg_xxx struct */
		arg_resetfn *resetfn;     /* Pointer to parent arg_xxx reset function */
		arg_scanfn  *scanfn;      /* Pointer to parent arg_xxx scan function */
		arg_checkfn *checkfn;     /* Pointer to parent arg_xxx check function */
		arg_errorfn *errorfn;     /* Pointer to parent arg_xxx error function */
		void        *priv;        /* Pointer to private header data for use by arg_xxx functions */
	};

	struct arg_rem
	{
		struct arg_hdr hdr;      /* The mandatory argtable header struct */
	};

	struct arg_lit
	{
		struct arg_hdr hdr;      /* The mandatory argtable header struct */
		int count;               /* Number of matching command line args */
	};

	struct arg_int
	{
		struct arg_hdr hdr;      /* The mandatory argtable header struct */
		int count;               /* Number of matching command line args */
		int *ival;               /* Array of parsed argument values */
	};

	struct arg_dbl
	{
		struct arg_hdr hdr;      /* The mandatory argtable header struct */
		int count;               /* Number of matching command line args */
		double *dval;            /* Array of parsed argument values */
	};

	struct arg_str
	{
		struct arg_hdr hdr;      /* The mandatory argtable header struct */
		int count;               /* Number of matching command line args */
		const char **sval;       /* Array of parsed argument values */
	};

	struct arg_rex
	{
		struct arg_hdr hdr;      /* The mandatory argtable header struct */
		int count;               /* Number of matching command line args */
		const char **sval;       /* Array of parsed argument values */
	};

	struct arg_file
	{
		struct arg_hdr hdr;      /* The mandatory argtable header struct */
		int count;               /* Number of matching command line args*/
		const char **filename;   /* Array of parsed filenames  (eg: /home/foo.bar) */
		const char **basename;   /* Array of parsed basenames  (eg: foo.bar) */
		const char **extension;  /* Array of parsed extensions (eg: .bar) */
	};

	struct arg_date
	{
		struct arg_hdr hdr;      /* The mandatory argtable header struct */
		const char *format;      /* strptime format string used to parse the date */
		int count;               /* Number of matching command line args */
		struct tm *tmval;        /* Array of parsed time values */
	};

	enum { ARG_ELIMIT = 1, ARG_EMALLOC, ARG_ENOMATCH, ARG_ELONGOPT, ARG_EMISSARG };
	struct arg_end
	{
		struct arg_hdr hdr;      /* The mandatory argtable header struct */
		int count;               /* Number of errors encountered */
		int *error;              /* Array of error codes */
		void **parent;           /* Array of pointers to offending arg_xxx struct */
		const char **argval;     /* Array of pointers to offending argv[] string */
	};


	/**** arg_xxx constructor functions *********************************/

	struct arg_rem* arg_rem(const char* datatype, const char* glossary);

	struct arg_lit* arg_lit0(const char* shortopts,
		const char* longopts,
		const char* glossary);
	struct arg_lit* arg_lit1(const char* shortopts,
		const char* longopts,
		const char *glossary);
	struct arg_lit* arg_litn(const char* shortopts,
		const char* longopts,
		int mincount,
		int maxcount,
		const char *glossary);

	struct arg_key* arg_key0(const char* keyword,
		int flags,
		const char* glossary);
	struct arg_key* arg_key1(const char* keyword,
		int flags,
		const char* glossary);
	struct arg_key* arg_keyn(const char* keyword,
		int flags,
		int mincount,
		int maxcount,
		const char* glossary);

	struct arg_int* arg_int0(const char* shortopts,
		const char* longopts,
		const char* datatype,
		const char* glossary);
	struct arg_int* arg_int1(const char* shortopts,
		const char* longopts,
		const char* datatype,
		const char *glossary);
	struct arg_int* arg_intn(const char* shortopts,
		const char* longopts,
		const char *datatype,
		int mincount,
		int maxcount,
		const char *glossary);

	struct arg_dbl* arg_dbl0(const char* shortopts,
		const char* longopts,
		const char* datatype,
		const char* glossary);
	struct arg_dbl* arg_dbl1(const char* shortopts,
		const char* longopts,
		const char* datatype,
		const char *glossary);
	struct arg_dbl* arg_dbln(const char* shortopts,
		const char* longopts,
		const char *datatype,
		int mincount,
		int maxcount,
		const char *glossary);

	struct arg_str* arg_str0(const char* shortopts,
		const char* longopts,
		const char* datatype,
		const char* glossary);
	struct arg_str* arg_str1(const char* shortopts,
		const char* longopts,
		const char* datatype,
		const char *glossary);
	struct arg_str* arg_strn(const char* shortopts,
		const char* longopts,
		const char* datatype,
		int mincount,
		int maxcount,
		const char *glossary);

	struct arg_rex* arg_rex0(const char* shortopts,
		const char* longopts,
		const char* pattern,
		const char* datatype,
		int flags,
		const char* glossary);
	struct arg_rex* arg_rex1(const char* shortopts,
		const char* longopts,
		const char* pattern,
		const char* datatype,
		int flags,
		const char *glossary);
	struct arg_rex* arg_rexn(const char* shortopts,
		const char* longopts,
		const char* pattern,
		const char* datatype,
		int mincount,
		int maxcount,
		int flags,
		const char *glossary);

	struct arg_file* arg_file0(const char* shortopts,
		const char* longopts,
		const char* datatype,
		const char* glossary);
	struct arg_file* arg_file1(const char* shortopts,
		const char* longopts,
		const char* datatype,
		const char *glossary);
	struct arg_file* arg_filen(const char* shortopts,
		const char* longopts,
		const char* datatype,
		int mincount,
		int maxcount,
		const char *glossary);

	struct arg_date* arg_date0(const char* shortopts,
		const char* longopts,
		const char* format,
		const char* datatype,
		const char* glossary);
	struct arg_date* arg_date1(const char* shortopts,
		const char* longopts,
		const char* format,
		const char* datatype,
		const char *glossary);
	struct arg_date* arg_daten(const char* shortopts,
		const char* longopts,
		const char* format,
		const char* datatype,
		int mincount,
		int maxcount,
		const char *glossary);

	struct arg_end* arg_end(int maxerrors);


	/**** other functions *******************************************/
	int arg_nullcheck(void **argtable);
	int arg_parse(int argc, char **argv, void **argtable);
	void arg_print_option(FILE *fp, const char *shortopts, const char *longopts, const char *datatype, const char *suffix);
	void arg_print_syntax(FILE *fp, void **argtable, const char *suffix);
	void arg_print_syntaxv(FILE *fp, void **argtable, const char *suffix);
	void arg_print_glossary(FILE *fp, void **argtable, const char *format);
	void arg_print_glossary_gnu(FILE *fp, void **argtable);
	void arg_print_errors(FILE* fp, struct arg_end* end, const char* progname);
	void arg_freetable(void **argtable, size_t n);

	/**** deprecated functions, for back-compatibility only ********/
	void arg_free(void **argtable);

#ifdef __cplusplus
}
#endif
#endif


/* Declarations for getopt.
Copyright (C) 1989,90,91,92,93,94,96,97 Free Software Foundation, Inc.

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

#ifndef _GETOPT_H
#define _GETOPT_H 1

#ifdef	__cplusplus
extern "C"
{
#endif

	/* For communication from `getopt' to the caller.
	When `getopt' finds an option that takes an argument,
	the argument value is returned here.
	Also, when `ordering' is RETURN_IN_ORDER,
	each non-option ARGV-element is returned here.  */

	extern char *optarg;

	/* Index in ARGV of the next element to be scanned.
	This is used for communication to and from the caller
	and for communication between successive calls to `getopt'.

	On entry to `getopt', zero means this is the first call; initialize.

	When `getopt' returns -1, this is the index of the first of the
	non-option elements that the caller should itself scan.

	Otherwise, `optind' communicates from one call to the next
	how much of ARGV has been scanned so far.  */

	extern int optind;

	/* Callers store zero here to inhibit the error message `getopt' prints
	for unrecognized options.  */

	extern int opterr;

	/* Set to an option character which was unrecognized.  */

	extern int optopt;

	/* Describe the long-named options requested by the application.
	The LONG_OPTIONS argument to getopt_long or getopt_long_only is a vector
	of `struct option' terminated by an element containing a name which is
	zero.

	The field `has_arg' is:
	no_argument          (or 0) if the option does not take an argument,
	required_argument    (or 1) if the option requires an argument,
	optional_argument    (or 2) if the option takes an optional argument.

	If the field `flag' is not NULL, it points to a variable that is set
	to the value given in the field `val' when the option is found, but
	left unchanged if the option is not found.

	To have a long-named option do something other than set an `int' to
	a compiled-in constant, such as set a value from `optarg', set the
	option's `flag' field to zero and its `val' field to a nonzero
	value (the equivalent single-letter option character, if there is
	one).  For long options that have a zero `flag' field, `getopt'
	returns the contents of the `val' field.  */

	struct option
	{
#if defined (__STDC__) && __STDC__
		const char *name;
#else
		char *name;
#endif
		/* has_arg can't be an enum because some compilers complain about
		type mismatches in all the code that assumes it is an int.  */
		int has_arg;
		int *flag;
		int val;
	};

	/* Names for the values of the `has_arg' field of `struct option'.  */

#define	no_argument		0
#define required_argument	1
#define optional_argument	2

#if defined (__STDC__) && __STDC__
#ifdef __GNU_LIBRARY__
	/* Many other libraries have conflicting prototypes for getopt, with
	differences in the consts, in stdlib.h.  To avoid compilation
	errors, only prototype getopt for the GNU C library.  */
	extern int getopt(int argc, char *const *argv, const char *shortopts);
#else				/* not __GNU_LIBRARY__ */
	extern int getopt();
#endif				/* __GNU_LIBRARY__ */
	extern int getopt_long(int argc, char *const *argv, const char *shortopts,
		const struct option *longopts, int *longind);
	extern int getopt_long_only(int argc, char *const *argv,
		const char *shortopts,
		const struct option *longopts, int *longind);

	/* Internal only.  Users should not call this directly.  */
	extern int _getopt_internal(int argc, char *const *argv,
		const char *shortopts,
		const struct option *longopts, int *longind,
		int long_only);
#else				/* not __STDC__ */
	extern int getopt();
	extern int getopt_long();
	extern int getopt_long_only();

	extern int _getopt_internal();
#endif				/* __STDC__ */

#ifdef	__cplusplus
}
#endif

#endif				/* _GETOPT_H */







