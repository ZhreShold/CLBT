#ifndef _GET_OPT_WRAPPER_H_
#define _GET_OPT_WRAPPER_H_

#if defined(WIN32) || defined(_WIN32)

/* Getopt for Microsoft C
This code is a modification of the Free Software Foundation, Inc.
Getopt library for parsing command line argument the purpose was
to provide a Microsoft Visual C friendly derivative. This code
provides functionality for both Unicode and Multibyte builds.

Date: 02/03/2011 - Ludvik Jerabek - Initial Release
Version: 1.0
Comment: Supports getopt, getopt_long, and getopt_long_only
and POSIXLY_CORRECT environment flag
License: LGPL

Revisions:

02/03/2011 - Ludvik Jerabek - Initial Release
02/20/2011 - Ludvik Jerabek - Fixed compiler warnings at Level 4
07/05/2011 - Ludvik Jerabek - Added no_argument, required_argument, optional_argument defs
08/03/2011 - Ludvik Jerabek - Fixed non-argument runtime bug which caused runtime exception
08/09/2011 - Ludvik Jerabek - Added code to export functions for DLL and LIB
02/15/2012 - Ludvik Jerabek - Fixed _GETOPT_THROW definition missing in implementation file
08/01/2012 - Ludvik Jerabek - Created separate functions for char and wchar_t characters so single dll can do both unicode and ansi
10/15/2012 - Ludvik Jerabek - Modified to match latest GNU features

**DISCLAIMER**
THIS MATERIAL IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING, BUT Not LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE, OR NON-INFRINGEMENT. SOME JURISDICTIONS DO NOT ALLOW THE
EXCLUSION OF IMPLIED WARRANTIES, SO THE ABOVE EXCLUSION MAY NOT
APPLY TO YOU. IN NO EVENT WILL I BE LIABLE TO ANY PARTY FOR ANY
DIRECT, INDIRECT, SPECIAL OR OTHER CONSEQUENTIAL DAMAGES FOR ANY
USE OF THIS MATERIAL INCLUDING, WITHOUT LIMITATION, ANY LOST
PROFITS, BUSINESS INTERRUPTION, LOSS OF PROGRAMS OR OTHER DATA ON
YOUR INFORMATION HANDLING SYSTEM OR OTHERWISE, EVEN If WE ARE
EXPRESSLY ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
*/
#ifndef __GETOPT_H_
	#define __GETOPT_H_

	#define STATIC_GETOPT

	#ifdef _GETOPT_API
		#undef _GETOPT_API
	#endif

	#if defined(EXPORTS_GETOPT) && defined(STATIC_GETOPT)
		#error "The preprocessor definitions of EXPORTS_GETOPT and STATIC_GETOPT can only be used individually"
	#elif defined(STATIC_GETOPT)
		/* #pragma message("Warning static builds of getopt violate the Lesser GNU Public License") */
		#define _GETOPT_API
	#elif defined(EXPORTS_GETOPT)
		#pragma message("Exporting getopt library")
		#define _GETOPT_API __declspec(dllexport)
	#else
		#pragma message("Importing getopt library")
		#define _GETOPT_API __declspec(dllimport)
	#endif

	/* Change behavior for C\C++ */
	#ifdef __cplusplus
		#define _BEGIN_EXTERN_C extern "C" {
		#define _END_EXTERN_C }
		#define _GETOPT_THROW throw()
	#else
		#define _BEGIN_EXTERN_C
		#define _END_EXTERN_C
		#define _GETOPT_THROW
	#endif

	/* Standard GNU options */
	#define	null_argument		0	/*Argument Null*/
	#define	no_argument			0	/*Argument Switch Only*/
	#define required_argument	1	/*Argument Required*/
	#define optional_argument	2	/*Argument Optional*/	

	/* Shorter Options */
	#define ARG_NULL	0	/*Argument Null*/
	#define ARG_NONE	0	/*Argument Switch Only*/
	#define ARG_REQ		1	/*Argument Required*/
	#define ARG_OPT		2	/*Argument Optional*/

	#include <string.h>
	#include <wchar.h>

_BEGIN_EXTERN_C

	extern _GETOPT_API int optind;
	extern _GETOPT_API int opterr;
	extern _GETOPT_API int optopt;

	/* Ansi */
	struct option_a
	{
		const char* name;
		int has_arg;
		int *flag;
		char val;
	};
	extern _GETOPT_API char *optarg_a;
	extern _GETOPT_API int getopt_a(int argc, char *const *argv, const char *optstring) _GETOPT_THROW;
	extern _GETOPT_API int getopt_long_a(int argc, char *const *argv, const char *options, const struct option_a *long_options, int *opt_index) _GETOPT_THROW;
	extern _GETOPT_API int getopt_long_only_a(int argc, char *const *argv, const char *options, const struct option_a *long_options, int *opt_index) _GETOPT_THROW;

	/* Unicode */
	struct option_w
	{
		const wchar_t* name;
		int has_arg;
		int *flag;
		wchar_t val;
	};
	extern _GETOPT_API wchar_t *optarg_w;
	extern _GETOPT_API int getopt_w(int argc, wchar_t *const *argv, const wchar_t *optstring) _GETOPT_THROW;
	extern _GETOPT_API int getopt_long_w(int argc, wchar_t *const *argv, const wchar_t *options, const struct option_w *long_options, int *opt_index) _GETOPT_THROW;
	extern _GETOPT_API int getopt_long_only_w(int argc, wchar_t *const *argv, const wchar_t *options, const struct option_w *long_options, int *opt_index) _GETOPT_THROW;	
	
_END_EXTERN_C

	#undef _BEGIN_EXTERN_C
	#undef _END_EXTERN_C
	#undef _GETOPT_THROW
	#undef _GETOPT_API

	#ifdef _UNICODE
		#define getopt getopt_w
		#define getopt_long getopt_long_w
		#define getopt_long_only getopt_long_only_w
		#define option option_w
		#define optarg optarg_w
	#else
		#define getopt getopt_a
		#define getopt_long getopt_long_a
		#define getopt_long_only getopt_long_only_a
		#define option option_a
		#define optarg optarg_a
	#endif
#endif  /* __GETOPT_H_ */

#else
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
#endif


#endif /* END _GET_OPT_WRAPPER_H_*/