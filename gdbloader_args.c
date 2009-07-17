#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <argp.h>
#include <stdint.h>
#include "currf2_args.h"


const char *argp_program_version = "gdbloader-0.0";
const char *argp_program_bug_address = "<pi3orama@gmail.com>";
static char doc[] =
	"gdbloader: load checkpoint made by currf2, then spin, make gdb can attach to it";
static char args_doc[] =
	"CKPT-FILE LOG-FILE TARGET-EXECUTABLE [TARGET OPTIONS]";

static struct argp_option options[] = {
	{"injectso",	'j', "filename",	0, "Set the system wrapper so file name, "
											"'./libwrapper.so' by default"},
	{"bias",		'b', "bias",		0, "Set the inject so load bias, 0x3000 by default"},
	{"syswrapper",	'w', "wrapsym",		0, "System call wrapper symbol, syscall_wrapper_entry by default"},
	{"debugentry",	'E', "entry",		0, "debug_entry, \'__debug_entry\' by default"},
	{"statevect",	'v', "statevect",	0, "state_vect's symbol, \'state_vector\' by default"},
	{NULL},
};

static struct opts opts = {
	.inj_so		= "./libwrapper.so",
	.wrap_sym 	= "syscall_wrapper_entry",
	.inj_bias	= 0x3000,
	.entry		= "__debug_entry",
	.state_vect		= "state_vector",
};

static error_t
parse_opt(int key, char *arg, struct argp_state *state)
{
	static int found_target = 0;
	switch (key) {
		case 'j':
			opts.inj_so = arg;
			return 0;
		case 'b':
			opts.inj_bias = strtol(arg, NULL, 0);
			return 0;
		case 'w':
			opts.wrap_sym = arg;
			return 0;
		case 'E':
			opts.entry = arg;
			return 0;
		case ARGP_KEY_ARG:
			/* target args */
			found_target = 1;
			return ARGP_ERR_UNKNOWN;
		case ARGP_KEY_ARGS:
			/* consume all args */
			return ARGP_ERR_UNKNOWN;
		case ARGP_KEY_END:
			if (!found_target) {
				argp_usage(state);
				return EINVAL;
			}
			return 0;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static struct argp argp = {options, parse_opt, args_doc, doc};

struct opts *
parse_args(int argc, char * argv[])
{
	int idx;
	int err;
	err = argp_parse(&argp, argc, argv, ARGP_IN_ORDER, &idx, NULL);
	opts.cmd_idx = idx;
	return &opts;
}

// vim:ts=4:sw=4

