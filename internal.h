#ifndef ARGP_INTERNAL
#define ARGP_INTERNAL

#include <argp.h>

/* For __ordering member */
enum {
	REQUIRE_ORDER, PERMUTE, RETURN_IN_ORDER
};

/* Data type for reentrant functions.  */

struct getopt_data
{
	/* These have exactly the same meaning as the corresponding global
		variables, except that they are used for the reentrant
		versions of getopt.  */
	int optind;
	int opterr;
	char *optarg;
	char optopt; /* we store characters here, a byte is enough */

	/* Internal members.  */

	/* True if the internal members have been initialized.  */
	char __initialized;

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

		PERMUTE is the default.  We permute the contents of ARGV as we
		scan, so that eventually all the non-options are at the end.
		This allows options to be given in any order, even with programs
		that were not written to expect this.

		RETURN_IN_ORDER is an option available to programs that were
		written to expect options and other ARGV-elements in any order
		and that care about the ordering of the two.  We describe each
		non-option ARGV-element as if it were the argument of an option
		with character code 1.  Using `-' as the first character of the
		list of option characters selects this mode of operation.

		The special argument `--' forces an end of option-scanning regardless
		of the value of `ordering'.  In the case of RETURN_IN_ORDER, only
		`--' can cause `getopt' to return -1 with `optind' != ARGC.  */
	char __ordering;

	/* If the POSIXLY_CORRECT environment variable is set.  */
	char __posixly_correct;

	/* The next char to be scanned in the option-element
		in which the last option character we returned was found.
		This allows us to pick up the scan where we left off.

		If this is zero, or a null string, it means resume the scan
		by advancing to the next ARGV-element.  */
	char *__nextchar;


	/* Handle permutation of arguments.  */

	/* Describe the part of ARGV that contains non-options that have
		been skipped.  `first_nonopt' is the index in ARGV of the first
		of them; `last_nonopt' is the index after the last of them.  */

	int __first_nonopt;
	int __last_nonopt;
};


struct parser
{
 const struct argp *argp;

 /* SHORT_OPTS is the getopt short options string for the union of all the
  groups of options.  */
 char *short_opts;
 /* LONG_OPTS is the array of getop long option structures for the union of
  all the groups of options.  */
 struct option *long_opts;
 /* OPT_DATA is the getopt data used for the re-entrant getopt.  */
 struct getopt_data opt_data;

 /* States of the various parsing groups.  */
 struct group *groups;
 /* The end of the GROUPS array.  */
 struct group *egroup;
 /* An vector containing storage for the CHILD_INPUTS field in all groups.  */
 void **child_inputs;

 /* True if we think using getopt is still useful; if false, then
  remaining arguments are just passed verbatim with ARGP_KEY_ARG.  This is
  cleared whenever getopt returns KEY_END, but may be set again if the user
  moves the next argument pointer backwards.  */
 int try_getopt;

 /* State block supplied to parsing routines.  */
 struct argp_state state;

 /* Memory used by this parser.  */
 void *storage;
};

/* The state of a `group' during parsing.  Each group corresponds to a
	particular argp structure from the tree of such descending from the top
	level argp passed to argp_parse.  */
struct group
{
	/* This group's parsing function.  */
	argp_parser_t parser;

	/* Which argp this group is from.  */
	const struct argp *argp;

	/* Points to the point in SHORT_OPTS corresponding to the end of the short
		options for this group.  We use it to determine from which group a
		particular short options is from.  */
	char *short_end;

	/* The number of non-option args sucessfully handled by this parser.  */
	unsigned args_processed;

	/* This group's parser's parent's group.  */
	struct group *parent;
	unsigned parent_index;       /* And the our position in the parent.   */

	/* These fields are swapped into and out of the state structure when
		calling this group's parser.  */
	void *input, **child_inputs;
	void *hook;
};

/* The next usable entries in the various parser tables being filled in by
	convert_options.  */
struct parser_convert_state
{
	struct parser *parser;
	char *short_end;
	struct option *long_end;
	void **child_inputs_end;
};


/* Lengths of various parser fields which we will allocated.  */
struct parser_sizes
{
	size_t short_len;            /* Getopt short options string.  */
	size_t long_len;             /* Getopt long options vector.  */
	size_t num_groups;           /* Group structures we allocate.  */
	size_t num_child_inputs;     /* Child input slots.  */
};

#endif
