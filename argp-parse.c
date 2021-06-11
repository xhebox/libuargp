/* Hierarchial argument parsing, layered over getopt
   Copyright (C) 1995-2000, 2002, 2003, 2004 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Written by Miles Bader <miles at gnu.ai.mit.edu>.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If
   not, see <http://www.gnu.org/licenses/>.

   Modified for uClibc by: Salvatore Cro <salvatore.cro at st.com>
*/

#ifndef _WIN32
#include <alloca.h>
#include <features.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <getopt.h>
#include <libintl.h>
#include <locale.h>
#include <argp.h>
#include "internal.h"

#define _GETOPT_DATA_INITIALIZER { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 }
#define NONOPTION_P (argv[d->optind][0] != '-' || argv[d->optind][1] == '\0')

/* For communication from `getopt' to the caller.
	When `getopt' finds an option that takes an argument,
	the argument value is returned here.
	Also, when `ordering' is RETURN_IN_ORDER,
	each non-option ARGV-element is returned here.  */

char *optarg;

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

/* Callers store zero here to inhibit the error message
	for unrecognized options.  */

int opterr = 1;

/* Set to an option character which was unrecognized.
	This must be initialized on some systems to avoid linking in the
	system's own getopt implementation.  */

int optopt = '?';

/* Exchange two adjacent subsequences of ARGV.
	One subsequence is elements [first_nonopt,last_nonopt)
	which contains all the non-options that have been skipped so far.
	The other is elements [last_nonopt,optind), which contains all
	the options processed since those non-options were skipped.
	`first_nonopt' and `last_nonopt' are relocated so that they describe
	the new indices of the non-options in ARGV after they are moved.  */
static void exchange (char **argv, struct getopt_data *d)
{
	int bottom = d->__first_nonopt;
	int middle = d->__last_nonopt;
	int top = d->optind;
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

	d->__first_nonopt += (d->optind - d->__last_nonopt);
	d->__last_nonopt = d->optind;
}

/* Initialize the internal data when the first call is made.  */

	static const char *
_getopt_initialize (const char *optstring, struct getopt_data *d)
{
	/* Start processing options with ARGV-element 1 (since ARGV-element 0
		is the program name); the sequence of previously skipped
		non-option ARGV-elements is empty.  */

	d->__first_nonopt = d->__last_nonopt = d->optind;

	d->__nextchar = NULL;

	d->__posixly_correct = !!getenv ("POSIXLY_CORRECT");

	/* Determine how to handle the ordering of options and nonoptions.  */

	if (optstring[0] == '-')
	{
		d->__ordering = RETURN_IN_ORDER;
		++optstring;
	}
	else if (optstring[0] == '+')
	{
		d->__ordering = REQUIRE_ORDER;
		++optstring;
	}
	else if (d->__posixly_correct)
		d->__ordering = REQUIRE_ORDER;
	else
		d->__ordering = PERMUTE;

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

static int _getopt_internal_r (int argc, char *const *argv,
	const char *optstring, const struct option *longopts,
	int *longind, int long_only, struct getopt_data *d)
{
	int print_errors = d->opterr;
	if (optstring[0] == ':')
		print_errors = 0;

	if (argc < 1)
		return -1;

	d->optarg = NULL;

	if (d->optind == 0 || !d->__initialized)
	{
		if (d->optind == 0)
			d->optind = 1;	/* Don't scan ARGV[0], the program name.  */
		optstring = _getopt_initialize (optstring, d);
		d->__initialized = 1;
	}

	/* Test whether ARGV[optind] points to a non-option argument.
		Either it does not have option syntax, or there is an environment flag
		from the shell indicating it is not an option.  The later information
		is only used when the used in the GNU libc.  */

	if (d->__nextchar == NULL || *d->__nextchar == '\0')
	{
		/* Advance to the next ARGV-element.  */

		/* Give FIRST_NONOPT & LAST_NONOPT rational values if OPTIND has been
			moved back by the user (who may also have changed the arguments).  */
		if (d->__last_nonopt > d->optind)
			d->__last_nonopt = d->optind;
		if (d->__first_nonopt > d->optind)
			d->__first_nonopt = d->optind;

		if (d->__ordering == PERMUTE)
		{
			/* If we have just processed some options following some non-options,
				exchange them so that the options come first.  */

			if (d->__first_nonopt != d->__last_nonopt
				&& d->__last_nonopt != d->optind)
				exchange ((char **) argv, d);
			else if (d->__last_nonopt != d->optind)
				d->__first_nonopt = d->optind;

			/* Skip any additional non-options
				and extend the range of non-options previously skipped.  */

			while (d->optind < argc && NONOPTION_P)
				d->optind++;
			d->__last_nonopt = d->optind;
		}

		/* The special ARGV-element `--' means premature end of options.
			Skip it like a null option,
			then exchange with previous non-options as if it were an option,
			then skip everything else like a non-option.  */

		if (d->optind != argc && !strcmp (argv[d->optind], "--"))
		{
			d->optind++;

			if (d->__first_nonopt != d->__last_nonopt
				&& d->__last_nonopt != d->optind)
				exchange ((char **) argv, d);
			else if (d->__first_nonopt == d->__last_nonopt)
				d->__first_nonopt = d->optind;
			d->__last_nonopt = argc;

			d->optind = argc;
		}

		/* If we have done all the ARGV-elements, stop the scan
			and back over any non-options that we skipped and permuted.  */

		if (d->optind == argc)
		{
			/* Set the next-arg-index to point at the non-options
				that we previously skipped, so the caller will digest them.  */
			if (d->__first_nonopt != d->__last_nonopt)
				d->optind = d->__first_nonopt;
			return -1;
		}

		/* If we have come to a non-option and did not permute it,
			either stop the scan or describe it to the caller and pass it by.  */

		if (NONOPTION_P)
		{
			if (d->__ordering == REQUIRE_ORDER)
				return -1;
			d->optarg = argv[d->optind++];
			return 1;
		}

		/* We have found another option-ARGV-element.
			Skip the initial punctuation.  */

		d->__nextchar = (argv[d->optind] + 1
			+ (longopts != NULL && argv[d->optind][1] == '-'));
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
		&& (argv[d->optind][1] == '-'
			|| (long_only && (argv[d->optind][2]
					|| !strchr (optstring, argv[d->optind][1])))))
	{
		char *nameend;
		const struct option *p;
		const struct option *pfound = NULL;
		int exact = 0;
		int ambig = 0;
		int indfound = -1;
		int option_index;

		for (nameend = d->__nextchar; *nameend && *nameend != '='; nameend++)
			/* Do nothing.  */ ;

		/* Test all long options for either exact match
			or abbreviated matches.  */
		for (p = longopts, option_index = 0; p->name; p++, option_index++)
			if (!strncmp (p->name, d->__nextchar, nameend - d->__nextchar))
			{
				if ((unsigned int) (nameend - d->__nextchar)
					== (unsigned int) strlen (p->name))
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
				else if (long_only
					|| pfound->has_arg != p->has_arg
					|| pfound->flag != p->flag
					|| pfound->val != p->val)
					/* Second or later nonexact match found.  */
					ambig = 1;
			}

		if (ambig && !exact)
		{
			if (print_errors)
			{
				fprintf (stderr, "%s: option `%s' is ambiguous\n",
					argv[0], argv[d->optind]);
			}
			d->__nextchar += strlen (d->__nextchar);
			d->optind++;
			d->optopt = 0;
			return '?';
		}

		if (pfound != NULL)
		{
			option_index = indfound;
			d->optind++;
			if (*nameend)
			{
				/* Don't test has_arg with >, because some C compilers don't
					allow it to be used on enums.  */
				if (pfound->has_arg)
					d->optarg = nameend + 1;
				else
				{
					if (print_errors)
					{
						if (argv[d->optind - 1][1] == '-')
						{
							/* --option */
							fprintf (stderr, "\
								%s: option `--%s' doesn't allow an argument\n",
								argv[0], pfound->name);
						}
						else
						{
							/* +option or -option */
							fprintf (stderr, "\
								%s: option `%c%s' doesn't allow an argument\n",
								argv[0], argv[d->optind - 1][0],
								pfound->name);
						}

					}

					d->__nextchar += strlen (d->__nextchar);

					d->optopt = pfound->val;
					return '?';
				}
			}
			else if (pfound->has_arg == 1)
			{
				if (d->optind < argc)
					d->optarg = argv[d->optind++];
				else
				{
					if (print_errors)
					{
						fprintf (stderr,
							"%s: option `%s' requires an argument\n",
							argv[0], argv[d->optind - 1]);
					}
					d->__nextchar += strlen (d->__nextchar);
					d->optopt = pfound->val;
					return optstring[0] == ':' ? ':' : '?';
				}
			}
			d->__nextchar += strlen (d->__nextchar);
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
		if (!long_only || argv[d->optind][1] == '-'
			|| strchr (optstring, *d->__nextchar) == NULL)
		{
			if (print_errors)
			{

				if (argv[d->optind][1] == '-')
				{
					/* --option */
					fprintf (stderr, "%s: unrecognized option `--%s'\n",
						argv[0], d->__nextchar);
				}
				else
				{
					/* +option or -option */
					fprintf (stderr, "%s: unrecognized option `%c%s'\n",
						argv[0], argv[d->optind][0], d->__nextchar);
				}

			}
			d->__nextchar = (char *) "";
			d->optind++;
			d->optopt = 0;
			return '?';
		}
	}

	/* Look at and handle the next short option-character.  */

	{
		char c = *d->__nextchar++;
		char *temp = strchr (optstring, c);

		/* Increment `optind' when we start to process its last character.  */
		if (*d->__nextchar == '\0')
			++d->optind;

		if (temp == NULL || c == ':')
		{
			if (print_errors)
			{
				if (d->__posixly_correct)
				{
					/* 1003.2 specifies the format of this message.  */
					fprintf (stderr, "%s: illegal option -- %c\n", argv[0], c);
				}
				else
				{
					fprintf (stderr, "%s: invalid option -- %c\n", argv[0], c);
				}

			}
			d->optopt = c;
			return '?';
		}
		if (temp[1] == ':')
		{
			if (temp[2] == ':')
			{
				/* This is an option that accepts an argument optionally.  */
				if (*d->__nextchar != '\0')
				{
					d->optarg = d->__nextchar;
					d->optind++;
				}
				else
					d->optarg = NULL;
				d->__nextchar = NULL;
			}
			else
			{
				/* This is an option that requires an argument.  */
				if (*d->__nextchar != '\0')
				{
					d->optarg = d->__nextchar;
					/* If we end this ARGV-element by taking the rest as an arg,
						we must advance to the next element now.  */
					d->optind++;
				}
				else if (d->optind == argc)
				{
					if (print_errors)
					{
						/* 1003.2 specifies the format of this message.  */
						fprintf (stderr,
							"%s: option requires an argument -- %c\n",
							argv[0], c);
					}
					d->optopt = c;
					if (optstring[0] == ':')
						c = ':';
					else
						c = '?';
				}
				else
					/* We already incremented `optind' once;
						increment it again when taking next ARGV-elt as argument.  */
					d->optarg = argv[d->optind++];
				d->__nextchar = NULL;
			}
		}
		return c;
	}
}

static int _getopt_long_r (int argc, char *const *argv, const char *options,
	const struct option *long_options, int *opt_index,
	struct getopt_data *d)
{
	return _getopt_internal_r (argc, argv, options, long_options, opt_index,
		0, d);
}

static int _getopt_long_only_r (int argc, char *const *argv, const char *options,
	const struct option *long_options, int *opt_index,
	struct getopt_data *d)
{
	return _getopt_internal_r (argc, argv, options, long_options, opt_index, 1, d);
}

#undef dgettext
#define dgettext(domain, msgid) (msgid ? dcgettext(domain, msgid, LC_MESSAGES) : NULL)

/* Getopt return values.  */
#define KEY_END (-1)           /* The end of the options.  */
#define KEY_ARG 1              /* A non-option argument.  */
#define KEY_ERR '?'            /* An error parsing the options.  */

/* The meta-argument used to prevent any further arguments being interpreted
   as options.  */
#define QUOTE "--"

/* The number of bits we steal in a long-option value for our own use.  */
#define GROUP_BITS CHAR_BIT

/* The number of bits available for the user value.  */
#define USER_BITS ((sizeof ((struct option *)0)->val * CHAR_BIT) - GROUP_BITS)
#define USER_MASK ((1 << USER_BITS) - 1)

/* EZ alias for ARGP_ERR_UNKNOWN.  */
#define EBADKEY ARGP_ERR_UNKNOWN

/* Default options.  */

/* When argp is given the --HANG switch, _ARGP_HANG is set and argp will sleep
   for one second intervals, decrementing _ARGP_HANG until it's zero.  Thus
   you can force the program to continue by attaching a debugger and setting
   it to 0 yourself.  */
static volatile int _argp_hang;

#define OPT_PROGNAME   -2
#define OPT_USAGE      -3
#define OPT_HANG       -4

static const struct argp_option argp_default_options[] =
{
  {"help",       '?',          0, 0,  "Give this help list", -1},
  {"usage",      OPT_USAGE,    0, 0,  "Give a short usage message"},
  {"program-name",OPT_PROGNAME,"NAME", OPTION_HIDDEN, "Set the program name"},
  {"HANG",       OPT_HANG,    "SECS", OPTION_ARG_OPTIONAL | OPTION_HIDDEN,
     "Hang for SECS seconds (default 3600)"},
  {0, 0}
};

static error_t
argp_default_parser (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case '?':
      argp_state_help (state, state->out_stream, ARGP_HELP_STD_HELP);
      break;
    case OPT_USAGE:
      argp_state_help (state, state->out_stream,
                      ARGP_HELP_USAGE | ARGP_HELP_EXIT_OK);
      break;

    case OPT_PROGNAME:         /* Set the program name.  */
#if defined _LIBC && defined(__UCLIBC_HAS_PROGRAM_INVOCATION_NAME__)
      program_invocation_name = arg;
#endif
      /* [Note that some systems only have PROGRAM_INVOCATION_SHORT_NAME (aka
        __PROGNAME), in which case, PROGRAM_INVOCATION_NAME is just defined
        to be that, so we have to be a bit careful here.]  */

      /* Update what we use for messages.  */
      state->name = strrchr (arg, '/');
      if (state->name)
       state->name++;
      else
       state->name = arg;

#if defined _LIBC && defined(__UCLIBC_HAS_PROGRAM_INVOCATION_NAME__)
      program_invocation_short_name = state->name;
#endif

      if ((state->flags & (ARGP_PARSE_ARGV0 | ARGP_NO_ERRS))
         == ARGP_PARSE_ARGV0)
       /* Update what getopt uses too.  */
       state->argv[0] = arg;

      break;

    case OPT_HANG:
      _argp_hang = atoi (arg ? arg : "3600");
      while (_argp_hang-- > 0)
       sleep (1);
      break;

    default:
      return EBADKEY;
    }
  return 0;
}

static const struct argp argp_default_argp =
  {argp_default_options, &argp_default_parser, NULL, NULL, NULL, NULL, "libc"};


static const struct argp_option argp_version_options[] =
{
  {"version",    'V',          0, 0,  "Print program version", -1},
  {0, 0}
};

static error_t
argp_version_parser (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case 'V':
      if (argp_program_version_hook)
       (*argp_program_version_hook) (state->out_stream, state);
      else if (argp_program_version)
       fprintf (state->out_stream, "%s\n", argp_program_version);
      else
       argp_error (state, dgettext (state->root_argp->argp_domain,
                                      "(PROGRAM ERROR) No version known!?"));
      if (! (state->flags & ARGP_NO_EXIT))
       exit (0);
      break;
    default:
      return EBADKEY;
    }
  return 0;
}

static const struct argp argp_version_argp =
  {argp_version_options, &argp_version_parser, NULL, NULL, NULL, NULL, "libc"};

/* Returns the offset into the getopt long options array LONG_OPTIONS of a
   long option with called NAME, or -1 if none is found.  Passing NULL as
   NAME will return the number of options.  */
static int
find_long_option (struct option *long_options, const char *name)
{
  struct option *l = long_options;
  while (l->name != NULL)
    if (name != NULL && strcmp (l->name, name) == 0)
      return l - long_options;
    else
      l++;
  if (name == NULL)
    return l - long_options;
  else
    return -1;
}


/* Call GROUP's parser with KEY and ARG, swapping any group-specific info
   from STATE before calling, and back into state afterwards.  If GROUP has
   no parser, EBADKEY is returned.  */
static error_t
group_parse (struct group *group, struct argp_state *state, int key, char *arg)
{
  if (group->parser)
    {
      error_t err;
      state->hook = group->hook;
      state->input = group->input;
      state->child_inputs = group->child_inputs;
      state->arg_num = group->args_processed;
      err = (*group->parser)(key, arg, state);
      group->hook = state->hook;
      return err;
    }
  else
    return EBADKEY;
}

/* Converts all options in ARGP (which is put in GROUP) and ancestors
   into getopt options stored in SHORT_OPTS and LONG_OPTS; SHORT_END and
   CVT->LONG_END are the points at which new options are added.  Returns the
   next unused group entry.  CVT holds state used during the conversion.  */
static struct group *
convert_options (const struct argp *argp,
                struct group *parent, unsigned parent_index,
                struct group *group, struct parser_convert_state *cvt)
{
  /* REAL is the most recent non-alias value of OPT.  */
  const struct argp_option *real = argp->options;
  const struct argp_child *children = argp->children;

  if (real || argp->parser)
    {
      const struct argp_option *opt;

      if (real)
       for (opt = real; !_option_is_end (opt); opt++)
         {
           if (! (opt->flags & OPTION_ALIAS))
             /* OPT isn't an alias, so we can use values from it.  */
             real = opt;

           if (! (real->flags & OPTION_DOC))
             /* A real option (not just documentation).  */
             {
               if (_option_is_short (opt))
                 /* OPT can be used as a short option.  */
                 {
                   *cvt->short_end++ = opt->key;
                   if (real->arg)
                     {
                       *cvt->short_end++ = ':';
                       if (real->flags & OPTION_ARG_OPTIONAL)
                         *cvt->short_end++ = ':';
                     }
                   *cvt->short_end = '\0'; /* keep 0 terminated */
                 }

               if (opt->name
                   && find_long_option (cvt->parser->long_opts, opt->name) < 0)
                 /* OPT can be used as a long option.  */
                 {
                   cvt->long_end->name = opt->name;
                   cvt->long_end->has_arg =
                     (real->arg
                      ? (real->flags & OPTION_ARG_OPTIONAL
                         ? optional_argument
                         : required_argument)
                      : no_argument);
                   cvt->long_end->flag = 0;
                   /* we add a disambiguating code to all the user's
                      values (which is removed before we actually call
                      the function to parse the value); this means that
                      the user loses use of the high 8 bits in all his
                      values (the sign of the lower bits is preserved
                      however)...  */
                   cvt->long_end->val =
                     ((opt->key | real->key) & USER_MASK)
                     + (((group - cvt->parser->groups) + 1) << USER_BITS);

                   /* Keep the LONG_OPTS list terminated.  */
                   (++cvt->long_end)->name = NULL;
                 }
             }
           }

      group->parser = argp->parser;
      group->argp = argp;
      group->short_end = cvt->short_end;
      group->args_processed = 0;
      group->parent = parent;
      group->parent_index = parent_index;
      group->input = 0;
      group->hook = 0;
      group->child_inputs = 0;

      if (children)
       /* Assign GROUP's CHILD_INPUTS field some space from
           CVT->child_inputs_end.*/
       {
         unsigned num_children = 0;
         while (children[num_children].argp)
           num_children++;
         group->child_inputs = cvt->child_inputs_end;
         cvt->child_inputs_end += num_children;
       }

      parent = group++;
    }
  else
    parent = 0;

  if (children)
    {
      unsigned index = 0;
      while (children->argp)
       group =
         convert_options (children++->argp, parent, index++, group, cvt);
    }

  return group;
}

/* Find the merged set of getopt options, with keys appropiately prefixed. */
static void
parser_convert (struct parser *parser, const struct argp *argp, int flags)
{
  struct parser_convert_state cvt;

  cvt.parser = parser;
  cvt.short_end = parser->short_opts;
  cvt.long_end = parser->long_opts;
  cvt.child_inputs_end = parser->child_inputs;

  if (flags & ARGP_IN_ORDER)
    *cvt.short_end++ = '-';
  else if (flags & ARGP_NO_ARGS)
    *cvt.short_end++ = '+';
  *cvt.short_end = '\0';

  cvt.long_end->name = NULL;

  parser->argp = argp;

  if (argp)
    parser->egroup = convert_options (argp, 0, 0, parser->groups, &cvt);
  else
    parser->egroup = parser->groups; /* No parsers at all! */
}

/* For ARGP, increments the NUM_GROUPS field in SZS by the total number of
 argp structures descended from it, and the SHORT_LEN & LONG_LEN fields by
 the maximum lengths of the resulting merged getopt short options string and
 long-options array, respectively.  */
static void
calc_sizes (const struct argp *argp,  struct parser_sizes *szs)
{
  const struct argp_child *child = argp->children;
  const struct argp_option *opt = argp->options;

  if (opt || argp->parser)
    {
      szs->num_groups++;
      if (opt)
       {
         int num_opts = 0;
         while (!_option_is_end (opt++))
           num_opts++;
         szs->short_len += num_opts * 3; /* opt + up to 2 `:'s */
         szs->long_len += num_opts;
       }
    }

  if (child)
    while (child->argp)
      {
       calc_sizes ((child++)->argp, szs);
       szs->num_child_inputs++;
      }
}


extern char * __argp_short_program_name (void);
/* Initializes PARSER to parse ARGP in a manner described by FLAGS.  */
static error_t
parser_init (struct parser *parser, const struct argp *argp,
            int argc, char **argv, int flags, void *input)
{
  error_t err = 0;
  struct group *group;
  struct parser_sizes szs;
  struct getopt_data opt_data = _GETOPT_DATA_INITIALIZER;

  szs.short_len = (flags & ARGP_NO_ARGS) ? 0 : 1;
  szs.long_len = 0;
  szs.num_groups = 0;
  szs.num_child_inputs = 0;

  if (argp)
    calc_sizes (argp, &szs);

  /* Lengths of the various bits of storage used by PARSER.  */
#define GLEN (szs.num_groups + 1) * sizeof (struct group)
#define CLEN (szs.num_child_inputs * sizeof (void *))
#define LLEN ((szs.long_len + 1) * sizeof (struct option))
#define SLEN (szs.short_len + 1)

  parser->storage = malloc (GLEN + CLEN + LLEN + SLEN);
  if (! parser->storage)
    return ENOMEM;

  parser->groups = parser->storage;
  parser->child_inputs = parser->storage + GLEN;
  parser->long_opts = parser->storage + GLEN + CLEN;
  parser->short_opts = parser->storage + GLEN + CLEN + LLEN;
  parser->opt_data = opt_data;

  memset (parser->child_inputs, 0, szs.num_child_inputs * sizeof (void *));
  parser_convert (parser, argp, flags);

  memset (&parser->state, 0, sizeof (struct argp_state));
  parser->state.root_argp = parser->argp;
  parser->state.argc = argc;
  parser->state.argv = argv;
  parser->state.flags = flags;
  parser->state.err_stream = stderr;
  parser->state.out_stream = stdout;
  parser->state.next = 0;      /* Tell getopt to initialize.  */
  parser->state.pstate = parser;

  parser->try_getopt = 1;

  /* Call each parser for the first time, giving it a chance to propagate
     values to child parsers.  */
  if (parser->groups < parser->egroup)
    parser->groups->input = input;
  for (group = parser->groups;
       group < parser->egroup && (!err || err == EBADKEY);
       group++)
    {
      if (group->parent)
       /* If a child parser, get the initial input value from the parent. */
       group->input = group->parent->child_inputs[group->parent_index];

      if (!group->parser
         && group->argp->children && group->argp->children->argp)
       /* For the special case where no parsing function is supplied for an
          argp, propagate its input to its first child, if any (this just
          makes very simple wrapper argps more convenient).  */
       group->child_inputs[0] = group->input;

      err = group_parse (group, &parser->state, ARGP_KEY_INIT, 0);
    }
  if (err == EBADKEY)
    err = 0;                   /* Some parser didn't understand.  */

  if (err)
    return err;

  if (parser->state.flags & ARGP_NO_ERRS)
    {
      parser->opt_data.opterr = 0;
      if (parser->state.flags & ARGP_PARSE_ARGV0)
       /* getopt always skips ARGV[0], so we have to fake it out.  As long
          as OPTERR is 0, then it shouldn't actually try to access it.  */
       parser->state.argv--, parser->state.argc++;
    }
  else
    parser->opt_data.opterr = 1;       /* Print error messages.  */

  if (parser->state.argv == argv && argv[0])
    /* There's an argv[0]; use it for messages.  */
    {
      char *short_name = strrchr (argv[0], '/');
      parser->state.name = short_name ? short_name + 1 : argv[0];
    }
  else
    parser->state.name = __argp_short_program_name ();

  return 0;
}

/* Free any storage consumed by PARSER (but not PARSER itself).  */
static error_t
parser_finalize (struct parser *parser,
                error_t err, int arg_ebadkey, int *end_index)
{
  struct group *group;

  if (err == EBADKEY && arg_ebadkey)
    /* Suppress errors generated by unparsed arguments.  */
    err = 0;

  if (! err)
    {
      if (parser->state.next == parser->state.argc)
       /* We successfully parsed all arguments!  Call all the parsers again,
          just a few more times... */
       {
         for (group = parser->groups;
              group < parser->egroup && (!err || err==EBADKEY);
              group++)
           if (group->args_processed == 0)
             err = group_parse (group, &parser->state, ARGP_KEY_NO_ARGS, 0);
         for (group = parser->egroup - 1;
              group >= parser->groups && (!err || err==EBADKEY);
              group--)
           err = group_parse (group, &parser->state, ARGP_KEY_END, 0);

         if (err == EBADKEY)
           err = 0;            /* Some parser didn't understand.  */

         /* Tell the user that all arguments are parsed.  */
         if (end_index)
           *end_index = parser->state.next;
       }
      else if (end_index)
       /* Return any remaining arguments to the user.  */
       *end_index = parser->state.next;
      else
       /* No way to return the remaining arguments, they must be bogus. */
       {
         if (!(parser->state.flags & ARGP_NO_ERRS)
             && parser->state.err_stream)
           fprintf (parser->state.err_stream,
                    dgettext (parser->argp->argp_domain,
                              "%s: Too many arguments\n"),
                    parser->state.name);
         err = EBADKEY;
       }
    }

  /* Okay, we're all done, with either an error or success; call the parsers
     to indicate which one.  */

  if (err)
    {
      /* Maybe print an error message.  */
      if (err == EBADKEY)
       /* An appropriate message describing what the error was should have
          been printed earlier.  */
       argp_state_help (&parser->state, parser->state.err_stream,
                          ARGP_HELP_STD_ERR);

      /* Since we didn't exit, give each parser an error indication.  */
      for (group = parser->groups; group < parser->egroup; group++)
       group_parse (group, &parser->state, ARGP_KEY_ERROR, 0);
    }
  else
    /* Notify parsers of success, and propagate back values from parsers.  */
    {
      /* We pass over the groups in reverse order so that child groups are
        given a chance to do there processing before passing back a value to
        the parent.  */
      for (group = parser->egroup - 1
          ; group >= parser->groups && (!err || err == EBADKEY)
          ; group--)
       err = group_parse (group, &parser->state, ARGP_KEY_SUCCESS, 0);
      if (err == EBADKEY)
       err = 0;                /* Some parser didn't understand.  */
    }

  /* Call parsers once more, to do any final cleanup.  Errors are ignored.  */
  for (group = parser->egroup - 1; group >= parser->groups; group--)
    group_parse (group, &parser->state, ARGP_KEY_FINI, 0);

  if (err == EBADKEY)
    err = EINVAL;

  free (parser->storage);

  return err;
}

/* Call the user parsers to parse the non-option argument VAL, at the current
   position, returning any error.  The state NEXT pointer is assumed to have
   been adjusted (by getopt) to point after this argument; this function will
   adjust it correctly to reflect however many args actually end up being
   consumed.  */
static error_t
parser_parse_arg (struct parser *parser, char *val)
{
  /* Save the starting value of NEXT, first adjusting it so that the arg
     we're parsing is again the front of the arg vector.  */
  int index = --parser->state.next;
  error_t err = EBADKEY;
  struct group *group;
  int key = 0;                 /* Which of ARGP_KEY_ARG[S] we used.  */

  /* Try to parse the argument in each parser.  */
  for (group = parser->groups
       ; group < parser->egroup && err == EBADKEY
       ; group++)
    {
      parser->state.next++;    /* For ARGP_KEY_ARG, consume the arg.  */
      key = ARGP_KEY_ARG;
      err = group_parse (group, &parser->state, key, val);

      if (err == EBADKEY)
       /* This parser doesn't like ARGP_KEY_ARG; try ARGP_KEY_ARGS instead. */
       {
         parser->state.next--; /* For ARGP_KEY_ARGS, put back the arg.  */
         key = ARGP_KEY_ARGS;
         err = group_parse (group, &parser->state, key, 0);
       }
    }

  if (! err)
    {
      if (key == ARGP_KEY_ARGS)
       /* The default for ARGP_KEY_ARGS is to assume that if NEXT isn't
          changed by the user, *all* arguments should be considered
          consumed.  */
       parser->state.next = parser->state.argc;

      if (parser->state.next > index)
       /* Remember that we successfully processed a non-option
          argument -- but only if the user hasn't gotten tricky and set
          the clock back.  */
       (--group)->args_processed += (parser->state.next - index);
      else
       /* The user wants to reparse some args, give getopt another try.  */
       parser->try_getopt = 1;
    }

  return err;
}

/* Call the user parsers to parse the option OPT, with argument VAL, at the
   current position, returning any error.  */
static error_t
parser_parse_opt (struct parser *parser, int opt, char *val)
{
  /* The group key encoded in the high bits; 0 for short opts or
     group_number + 1 for long opts.  */
  int group_key = opt >> USER_BITS;
  error_t err = EBADKEY;

  if (group_key == 0)
    /* A short option.  By comparing OPT's position in SHORT_OPTS to the
       various starting positions in each group's SHORT_END field, we can
       determine which group OPT came from.  */
    {
      struct group *group;
      char *short_index = strchr (parser->short_opts, opt);

      if (short_index)
       for (group = parser->groups; group < parser->egroup; group++)
         if (group->short_end > short_index)
           {
             err = group_parse (group, &parser->state, opt,
                                parser->opt_data.optarg);
             break;
           }
    }
  else
    /* A long option.  We use shifts instead of masking for extracting
       the user value in order to preserve the sign.  */
    err =
      group_parse (&parser->groups[group_key - 1], &parser->state,
                  (opt << GROUP_BITS) >> GROUP_BITS,
                  parser->opt_data.optarg);

  if (err == EBADKEY)
    /* At least currently, an option not recognized is an error in the
       parser, because we pre-compute which parser is supposed to deal
       with each option.  */
    {
      static const char bad_key_err[] =
       "(PROGRAM ERROR) Option should have been recognized!?";
      if (group_key == 0)
       argp_error (&parser->state, "-%c: %s", opt,
                     dgettext (parser->argp->argp_domain, bad_key_err));
      else
       {
         struct option *long_opt = parser->long_opts;
         while (long_opt->val != opt && long_opt->name)
           long_opt++;
         argp_error (&parser->state, "--%s: %s",
                       long_opt->name ? long_opt->name : "???",
                       dgettext (parser->argp->argp_domain, bad_key_err));
       }
    }

  return err;
}

/* Parse the next argument in PARSER (as indicated by PARSER->state.next).
   Any error from the parsers is returned, and *ARGP_EBADKEY indicates
   whether a value of EBADKEY is due to an unrecognized argument (which is
   generally not fatal).  */
static error_t
parser_parse_next (struct parser *parser, int *arg_ebadkey)
{
  int opt;
  error_t err = 0;

  if (parser->state.quoted && parser->state.next < parser->state.quoted)
    /* The next argument pointer has been moved to before the quoted
       region, so pretend we never saw the quoting `--', and give getopt
       another chance.  If the user hasn't removed it, getopt will just
       process it again.  */
    parser->state.quoted = 0;

  if (parser->try_getopt && !parser->state.quoted)
    /* Give getopt a chance to parse this.  */
    {
      /* Put it back in OPTIND for getopt.  */
      parser->opt_data.optind = parser->state.next;
      /* Distinguish KEY_ERR from a real option.  */
      parser->opt_data.optopt = KEY_END;
      if (parser->state.flags & ARGP_LONG_ONLY)
       opt = _getopt_long_only_r (parser->state.argc, parser->state.argv,
                                  parser->short_opts, parser->long_opts, 0,
                                  &parser->opt_data);
      else
       opt = _getopt_long_r (parser->state.argc, parser->state.argv,
                             parser->short_opts, parser->long_opts, 0,
                             &parser->opt_data);
      /* And see what getopt did.  */
      parser->state.next = parser->opt_data.optind;

      if (opt == KEY_END)
       /* Getopt says there are no more options, so stop using
          getopt; we'll continue if necessary on our own.  */
       {
         parser->try_getopt = 0;
         if (parser->state.next > 1
             && strcmp (parser->state.argv[parser->state.next - 1], QUOTE)
                  == 0)
           /* Not only is this the end of the options, but it's a
              `quoted' region, which may have args that *look* like
              options, so we definitely shouldn't try to use getopt past
              here, whatever happens.  */
           parser->state.quoted = parser->state.next;
       }
      else if (opt == KEY_ERR && parser->opt_data.optopt != KEY_END)
       /* KEY_ERR can have the same value as a valid user short
          option, but in the case of a real error, getopt sets OPTOPT
          to the offending character, which can never be KEY_END.  */
       {
         *arg_ebadkey = 0;
         return EBADKEY;
       }
    }
  else
    opt = KEY_END;

  if (opt == KEY_END)
    {
      /* We're past what getopt considers the options.  */
      if (parser->state.next >= parser->state.argc
         || (parser->state.flags & ARGP_NO_ARGS))
       /* Indicate that we're done.  */
       {
         *arg_ebadkey = 1;
         return EBADKEY;
       }
      else
       /* A non-option arg; simulate what getopt might have done.  */
       {
         opt = KEY_ARG;
         parser->opt_data.optarg = parser->state.argv[parser->state.next++];
       }
    }

  if (opt == KEY_ARG)
    /* A non-option argument; try each parser in turn.  */
    err = parser_parse_arg (parser, parser->opt_data.optarg);
  else
    err = parser_parse_opt (parser, opt, parser->opt_data.optarg);

  if (err == EBADKEY)
    *arg_ebadkey = (opt == KEY_END || opt == KEY_ARG);

  return err;
}

/* Parse the options strings in ARGC & ARGV according to the argp in ARGP.
   FLAGS is one of the ARGP_ flags above.  If END_INDEX is non-NULL, the
   index in ARGV of the first unparsed option is returned in it.  If an
   unknown option is present, EINVAL is returned; if some parser routine
   returned a non-zero value, it is returned; otherwise 0 is returned.  */
error_t
argp_parse (const struct argp *argp, int argc, char **argv, unsigned flags,
             int *end_index, void *input)
{
  error_t err;
  struct parser parser;

  /* If true, then err == EBADKEY is a result of a non-option argument failing
     to be parsed (which in some cases isn't actually an error).  */
  int arg_ebadkey = 0;

  if (! (flags & ARGP_NO_HELP))
    /* Add our own options.  */
    {
      struct argp_child *child = alloca (4 * sizeof (struct argp_child));
      struct argp *top_argp = alloca (sizeof (struct argp));

      /* TOP_ARGP has no options, it just serves to group the user & default
        argps.  */
      memset (top_argp, 0, sizeof (*top_argp));
      top_argp->children = child;

      memset (child, 0, 4 * sizeof (struct argp_child));

      if (argp)
       (child++)->argp = argp;
      (child++)->argp = &argp_default_argp;
      if (argp_program_version || argp_program_version_hook)
       (child++)->argp = &argp_version_argp;
      child->argp = 0;

      argp = top_argp;
    }

  /* Construct a parser for these arguments.  */
  err = parser_init (&parser, argp, argc, argv, flags, input);

  if (! err)
    /* Parse! */
    {
      while (! err)
       err = parser_parse_next (&parser, &arg_ebadkey);
      err = parser_finalize (&parser, err, arg_ebadkey, end_index);
    }

  return err;
}

int _option_is_short (const struct argp_option *__opt)
{
  if (__opt->flags & OPTION_DOC)
    return 0;
  else
    {
      int __key = __opt->key;
      return __key > 0 && __key <= UCHAR_MAX && isprint (__key);
    }
}

int _option_is_end (const struct argp_option *__opt)
{
  return !__opt->key && !__opt->name && !__opt->doc && !__opt->group;
}
