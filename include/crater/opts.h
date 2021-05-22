#pragma once

/// @file
/// @author hacatu
/// @version 0.3.0
/// @section LICENSE
/// This Source Code Form is subject to the terms of the Mozilla Public
/// License, v. 2.0. If a copy of the MPL was not distributed with this
/// file, You can obtain one at http://mozilla.org/MPL/2.0/.
/// @section DESCRIPTION
/// Command line options parsing utilities
///

#include <stdbool.h>

/// Description of a command line option
/// A list of these, terminated with a certain sentinel value, must be passed to { @link cr8r_opt_parse }.
/// See { @link CR8R_OPT_GENERIC_OPTIONAL } and { @link CR8R_OPT_GENERIC_REQUIRED } for macros that can
/// automatically generate these descriptions
typedef struct cr8r_opt cr8r_opt;
struct cr8r_opt {
	/// POINTER to variable/memory to store result in.
	/// on_opt and on_missing are responsible for setting this.
	/// To have a default value, the variable pointed by dest may be initialized or on_missing may be used.
	/// May be NULL.
	/// Can also be used to provide CUSTOM callbacks with extra data as well as a pointers to memory to initialize.
	void *dest;
	/// Flag indicating whether or not the option was found.
	/// Should not be set, managed by { @link cr8r_opt_parse }.  Automatically set to false before parsing
	/// starts and true when this argument is found (but after calling on_opt if applicable).
	/// Can be checked after parsing is done to find which arguments were/weren't found, as an alternative
	/// to on_missing.  Can also be checked in on_opt to determine if this option has been encountered
	/// before (ie is a duplicate).
	bool found;
	/// 0 indicates this option does not take an argument, 1 indicates this option has a required argument,
	/// and 2 indicates this option has an optional argument.
	/// For long form options, `--long-name=argument` (argument in same command line argument after
	/// an '=') or `--long-name argument` (argument in next command line argument) are permitted.  If either of
	/// these occurs, the argument will be passed to on_opt.  Otherwise (if there is no '=' and the next
	/// command line argument starts with '-'), the option is considered to not havean argument.
	/// If arg_required is true, this is an error and on_opt is not called.  If arg_required is false,
	/// on_opt is called with NULL.  For short form options, note that `-ab=argument` and `-ab argument` are both
	/// valid ways of providing a short form option `a` with no argument and `b` with argument "argument", but
	/// `-bargument` is not a valid way of doing this.  This would be interpreted as 9 short form options with no
	/// arguments, or possibly an argument to `t` depending on what follows.
	int arg_mode;
	/// The short name for this option.
	/// Must not contain '-', '=', ' ', or more than one character.  However, multibyte characters are allowed.
	/// May be NULL, but if both short_name and long_name are NULL then this indicates the end of the option list.
	/// Any character in a short form option group (command line argument starting with a '-') is checked using this
	/// name.
	const char *short_name;
	/// The long name for this option.
	/// Must not contain '=' or ' ', but may contain any number of characters including multibyte characters and
	/// '-'.
	/// May be NULL, but if both short_name and long_name are NULL then this indicates the end of the option list.
	/// Any long form option (part of a command line argument which starts with "--" and stops if an '=' is encountered)
	/// is checked using this name.
	const char *long_name;
	/// Description of this option, for printing in the command help.
	const char *description;
	/// Function to call when this option is encountered.
	/// If NULL, any argument is ignored and the only effect of encountering this option is setting `found` (and hence not
	/// calling on_missing if applicable).
	/// If this function returns false, the argument to this option is considered invalid.
	/// The argument to the option, opt, can be supplied in the command line either in the same command line argument as the
	/// option but separated by an '=', or in the next command line argument but not preceded by a '-'.
	/// If no such argument is found, opt is NULL, so if arg_required is not set, on_opt must be prepared to deal with that.
	/// found is set after calling this function, so it can be used to check if this option is provided more than once.
	bool (*on_opt)(cr8r_opt *self, char *opt);
	/// Function to call after reading command line arguments if this option was not found.
	/// If the option is not found and this function is NULL or returns false, this option is considered missing.
	/// This function should return true if it is ok for this option to be missing ( { @link cr8r_opt_missing_optional } can
	/// be used to always return true, also see { @link CR8R_OPT_GENERIC_OPTIONAL } ).
	/// This function can also be used to initialize a variable in a default way if it is not provided as an option, eg
	/// by randomizing it, offering more flexibility than simply giving the variable a default value.
	bool (*on_missing)(cr8r_opt *self);
};

/// General configuration settings for parsing options
typedef struct{
	/// Custom data for on_arg if needed
	void *data;
	/// Whether option parsing should fail and return false after the first error or after all errors
	bool stop_on_first_err;
	/// Callback to handle positional arguments.
	/// Command line arguments beginning with "-" are either short option groups or long options.
	/// An argument not beginning with "-" following one that does is considered an argument to the
	/// latter.  Any other argument, or any argument after "--" which terminates option parsing,
	/// is considered a positional argument.
	bool (*on_arg)(void *data, int argc, char **argv, int i);
} cr8r_opt_cfg;

/// "Default" implementation of on_missing that returns true to indicate it is ok for the option to be missing
/// { @link CR8R_OPT_GENERIC_OPTIONAL } is a convenient way to create simple optional arguments.
bool cr8r_opt_missing_optional(cr8r_opt *self);

/// "Default" implementation of on_opt that prints help and exits
/// Should NOT be called or used directly except by { @link CR8R_OPT_HELP }.
/// Requires self->dest to point to the options list.
bool cr8r_opt_print_help(cr8r_opt *self, char *opt);

/// "Default" implementation of on_arg that returns true and ignores a positional argument
/// See { @link cr8r_opt_cfg }.
bool cr8r_opt_ignore_arg(void *data, int argc, char **argv, int i);

/// Parse command line options and arguments.
///
/// `opts` is a list of option descriptions, terminated by one whose `short_name` and `long_name`
/// are both NULL.
/// The command line arguments are then parsed according to these options in the following way:
///
/// Command line arguments are read from increasing indicies starting at 1.
///
/// If an argument is "--", it indicates the end of option parsing and the remaining
/// arguments are all treated as positional arguments.
///
/// Otherwise, if an argument starts with "--", it is considered a long name option.  If the command
/// line argument containing a long name option contains an '=', the portion after the '=' is
/// considered the argument to the option.  Otherwise, if the next command line argument does not
/// start with a '-', it is considered the argument to the option.  Otherwise, the option is
/// considered to not have an argument.
///
/// If an argument starts with "-" followed by one or more valid short option names, it is considered
/// a short option group.  The group ends when '\0', '=', or an invalid short option name
/// is encountered.  All short options until the last one are considered to not have an argument.
/// The rules for determining the argument to the last option are the same as for long name options.
///
/// If an argument is "-", it is an error.
///
/// Any other command line argument, one which is not a long name option, short name option group,
/// "--", "-", or an argument to an option, is a positional argument.
///
/// If a short/long option name is encountered which is not in `opts`, this is an error.
/// If an option that requires an argument but does not have one, this is considered an error.
/// Otherwise, the option's on_opt callback is called if it is not NULL and then it is set to found.
/// If on_opt return false it is an error.
/// After parsing is done, any options which were not found have on_missing called.  If an option is
/// missing and on_missing is NULL or returns false for it, this is an error.  If on_missing
/// returns true, this is ok.
///
/// If a positional argument is encountered, cfg->on_arg is called.  If it is NULL or returns false,
/// this is an error.
///
/// If `opts` is invalid, this function prints an error and returns false.  Otherwise, it parses
/// the arguments as described above.  If an error is encountered, false is returned, immediately
/// if cfg->stop_on_first_err is true and after all arguments have been read otherwise.
/// @param [in,out] opts: pointer to a list of option descriptions, terminated by an option description
/// with both short_name and long_name NULL.  `found` is set on each option according to whether it was seen.
/// @param [in] cfg: general parsing options
/// @param [in] argc, argv: parameters to `main`
bool cr8r_opt_parse(cr8r_opt *opts, cr8r_opt_cfg *cfg, int argc, char **argv);

bool cr8r_opt_parse_ull(cr8r_opt *self, char *opt);
bool cr8r_opt_parse_ll(cr8r_opt *self, char *opt);
bool cr8r_opt_parse_ul(cr8r_opt *self, char *opt);
bool cr8r_opt_parse_l(cr8r_opt *self, char *opt);
bool cr8r_opt_parse_u(cr8r_opt *self, char *opt);
bool cr8r_opt_parse_i(cr8r_opt *self, char *opt);
bool cr8r_opt_parse_us(cr8r_opt *self, char *opt);
bool cr8r_opt_parse_s(cr8r_opt *self, char *opt);
bool cr8r_opt_parse_uc(cr8r_opt *self, char *opt);
bool cr8r_opt_parse_sc(cr8r_opt *self, char *opt);
bool cr8r_opt_parse_c(cr8r_opt *self, char *opt);
bool cr8r_opt_parse_f(cr8r_opt *self, char *opt);
bool cr8r_opt_parse_d(cr8r_opt *self, char *opt);
bool cr8r_opt_parse_ld(cr8r_opt *self, char *opt);
bool cr8r_opt_parse_cstr(cr8r_opt *self, char *opt);
bool cr8r_opt_parse_b(cr8r_opt *self, char *opt);
bool cr8r_opt_parse_i128(cr8r_opt *self, char *opt);
bool cr8r_opt_parse_u128(cr8r_opt *self, char *opt);

#define CR8R_OPT_ARGMODE_NONE 0
#define CR8R_OPT_ARGMODE_REQUIRED 1
#define CR8R_OPT_ARGMODE_OPTIONAL 2

#define _CR8R_OPT_GENERIC_OPTFIELDS(_dest, _short_name, _long_name, _description) 	.dest = (_dest),\
	.arg_mode = CR8R_OPT_ARGMODE_REQUIRED,                                     \
	.short_name = (_short_name),                                               \
	.long_name = (_long_name),                                                 \
	.description = (_description),                                             \
	.on_opt = _Generic(*(_dest),                                               \
	unsigned long long: cr8r_opt_parse_ull,                                    \
	long long: cr8r_opt_parse_ll,                                              \
	unsigned long: cr8r_opt_parse_ul,                                          \
	long: cr8r_opt_parse_l,                                                    \
	unsigned: cr8r_opt_parse_u,                                                \
	int: cr8r_opt_parse_i,                                                     \
	unsigned short: cr8r_opt_parse_us,                                         \
	short: cr8r_opt_parse_s,                                                   \
	unsigned char: cr8r_opt_parse_uc,                                          \
	signed char: cr8r_opt_parse_sc,                                            \
	char: cr8r_opt_parse_c,                                                    \
	float: cr8r_opt_parse_f,                                                   \
	double: cr8r_opt_parse_d,                                                  \
	long double: cr8r_opt_parse_ld,                                            \
	char*: cr8r_opt_parse_cstr,                                                \
	_Bool: cr8r_opt_parse_b,                                                   \
	__int128: cr8r_opt_parse_i128,                                             \
	unsigned __int128: cr8r_opt_parse_u128),

/// Represents a description for an optional scalar option
/// @param [in,out] dest: a pointer to a scalar type (char, _Bool,
/// signed/unsigned char/short/int/long/long long, float/double/long double)
/// where the result of parsing the argument to this option should be stored
/// @param [in] _short_name: short name for the option.  Must be a single (possibly multibyte) character besides ' ', '-', or '='
/// @param [in] _long_name: long name for the option.  May not contain ' ' or '='.  At least one of _long_name and _short_name must be nonnull
/// @param [in] _description: description of the option to print in help text.
#define CR8R_OPT_GENERIC_OPTIONAL(_dest, _short_name, _long_name, _description) ((cr8r_opt){\
	_CR8R_OPT_GENERIC_OPTFIELDS((_dest),( _short_name), (_long_name), (_description))\
	.on_missing = cr8r_opt_missing_optional,})

/// Represents a description for a required scalar option
/// @param [in,out] dest: a pointer to a scalar type (char, _Bool,
/// signed/unsigned char/short/int/long/long long, float/double/long double)
/// where the result of parsing the argument to this option should be stored
/// @param [in] _short_name: short name for the option.  Must be a single (possibly multibyte) character besides ' ', '-', or '='
/// @param [in] _long_name: long name for the option.  May not contain ' ' or '='.  At least one of _long_name and _short_name must be nonnull
/// @param [in] _description: description of the option to print in help text.
#define CR8R_OPT_GENERIC_REQUIRED(_dest, _short_name, _long_name, _description) ((cr8r_opt)){\
	_CR8R_OPT_GENERIC_OPTFIELDS((_dest), (_short_name), (_long_name), (_description))}

#define CR8R_OPT_HELP(_opts, _description) ((cr8r_opt){.dest=(_opts), .short_name="h", .long_name="help", .description=(_description), .on_opt=cr8r_opt_print_help, .on_missing=cr8r_opt_missing_optional})
#define CR8R_OPT_END() ((cr8r_opt){.short_name=NULL, .long_name=NULL})

