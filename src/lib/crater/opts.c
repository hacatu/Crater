#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>

#include <crater/opts.h>
#include <crater/hash.h>

bool cr8r_opt_missing_optional(cr8r_opt *self){
	return true;
}

bool cr8r_opt_print_help(cr8r_opt *self, char *opt){
	cr8r_opt *opts = self->dest;
	fprintf(stderr, "%s", self->description);
	for(uint64_t i = 0; opts[i].short_name || opts[i].long_name; ++i){
		if(opts + i != self){
			if(opts[i].short_name){
				if(opts[i].long_name){
					fprintf(stderr, "\t-%s/--%s: \t%s\n", opts[i].short_name, opts[i].long_name, opts[i].description ?: "<no description>");
				}else{
					fprintf(stderr, "\t-%s: \t%s\n", opts[i].short_name, opts[i].description ?: "<no description>");
				}
			}else{
				fprintf(stderr, "\t--%s: \t%s\n", opts[i].long_name, opts[i].description ?: "<no description>");
			}
		}else{
			fprintf(stderr, "\t-h/--help: display this help information and exit\n");
		}
	}
	exit(0);
}

bool cr8r_opt_ignore_arg(void *data, int argc, char **argv, int i){
	return true;
}

bool cr8r_opt_parse_ull(cr8r_opt *self, char *opt){
	char *end = opt;
	errno = 0;
	*(unsigned long long*)self->dest = strtoull(opt, &end, 0);
	return errno != ERANGE && end != opt && !*end;
}

bool cr8r_opt_parse_ll(cr8r_opt *self, char *opt){
	char *end = opt;
	errno = 0;
	*(long long*)self->dest = strtoll(opt, &end, 0);
	return errno != ERANGE && end != opt && !*end;
}

bool cr8r_opt_parse_ul(cr8r_opt *self, char *opt){
	char *end = opt;
	errno = 0;
	*(unsigned long*)self->dest = strtoul(opt, &end, 0);
	return errno != ERANGE && end != opt && !*end;
}

bool cr8r_opt_parse_l(cr8r_opt *self, char *opt){
	char *end = opt;
	errno = 0;
	*(unsigned long*)self->dest = strtol(opt, &end, 0);
	return errno != ERANGE && end != opt && !*end;
}

bool cr8r_opt_parse_u(cr8r_opt *self, char *opt){
	char *end = opt;
	errno = 0;
	unsigned long n = strtoul(opt, &end, 0);
	*(unsigned*)self->dest = n;
	return errno != ERANGE && end != opt && !*end &&
		n <= UINT_MAX;
}

bool cr8r_opt_parse_i(cr8r_opt *self, char *opt){
	char *end = opt;
	errno = 0;
	long n = strtol(opt, &end, 0);
	*(int*)self->dest = n;
	return errno != ERANGE && end != opt && !*end &&
		INT_MIN <= n && n <= INT_MAX;
}

bool cr8r_opt_parse_us(cr8r_opt *self, char *opt){
	char *end = opt;
	errno = 0;
	unsigned long n = strtoul(opt, &end, 0);
	*(unsigned short*)self->dest = n;
	return errno != ERANGE && end != opt && !*end &&
		n <= USHRT_MAX;
}

bool cr8r_opt_parse_s(cr8r_opt *self, char *opt){
	char *end = opt;
	errno = 0;
	long n  = strtol(opt, &end, 0);
	*(short*)self->dest = n;
	return errno != ERANGE && end != opt && !*end &&
		SHRT_MIN <= n && n <= SHRT_MAX;
}

bool cr8r_opt_parse_uc(cr8r_opt *self, char *opt){
	char *end = opt;
	if(!*end){
		return false;
	}
	if(!end[1]){
		*(unsigned char*)self->dest = end[0];
		return true;
	}
	errno = 0;
	unsigned long n = strtoul(opt, &end, 0);
	*(unsigned char*)self->dest = n;
	return errno != ERANGE && end != opt && !*end &&
		n <= UCHAR_MAX;
}

bool cr8r_opt_parse_sc(cr8r_opt *self, char *opt){
	char *end = opt;
	if(!*end){
		return false;
	}
	if(!end[1]){
		*(signed char*)self->dest = end[0];
		return true;
	}
	errno = 0;
	long n = strtol(opt, &end, 0);
	*(signed char*)self->dest = n;
	return errno != ERANGE && end != opt && !*end &&
		SCHAR_MIN <= n && n <= SCHAR_MAX;
}

bool cr8r_opt_parse_c(cr8r_opt *self, char *opt){
	char *end = opt;
	if(!*end){
		return false;
	}
	if(!end[1]){
		*(char*)self->dest = end[0];
		return true;
	}
	errno = 0;
#if CHAR_MIN
	long n = strtol(opt, &end, 0);
	*(char*)self->dest = n;
#else
	unsigned long n = strtoul(opt, &end, 0);
	*(char*)self->dest = n;
#endif
	return errno != ERANGE && end != opt && !*end &&
		CHAR_MIN <= n && n <= CHAR_MAX;
}

bool cr8r_opt_parse_f(cr8r_opt *self, char *opt){
	char *end = opt;
	errno = 0;
	*(float*)self->dest = strtof(opt, &end);
	return errno != ERANGE && end != opt && !*end;
}

bool cr8r_opt_parse_d(cr8r_opt *self, char *opt){
	char *end = opt;
	errno = 0;
	*(double*)self->dest = strtod(opt, &end);
	return errno != ERANGE && end != opt && !*end;
}

bool cr8r_opt_parse_ld(cr8r_opt *self, char *opt){
	char *end = opt;
	errno = 0;
	*(long double*)self->dest = strtold(opt, &end);
	return errno != ERANGE && end != opt && !*end;
}

bool cr8r_opt_parse_cstr(cr8r_opt *self, char *opt){
	*(const char**)self->dest = opt;
	return !!*opt;
}

bool cr8r_opt_parse_b(cr8r_opt *self, char *opt){
	char *end = opt;
	errno = 0;
	unsigned long n = strtoul(opt, &end, 0);
	*(bool*)self->dest = !!n;
	if(errno == ERANGE || n > 1){
		return false;
	}
	if(end == opt){
		if(!strcasecmp(opt, "true") || !strcasecmp(opt, "yes") || !strcasecmp(opt, "t") || !strcasecmp(opt, "y")){
			*(bool*)self->dest = true;
		}else if(!strcasecmp(opt, "false") || !strcasecmp(opt, "no") || !strcasecmp(opt, "f") || !strcasecmp(opt, "n")){
			*(bool*)self->dest = false;
		}else{
			return false;
		}
	}
	return true;
}

static inline int get_digit_base(int c, int base){
	if(c < '0'){
		return -1;
	}else if(base <= 10){
		return c < '0' + base ? c - '0' : -1;
	}else if(c <= '9'){
		return c - '0';
	}else if(c > 'Z'){
		if(c < 'a'){
			return -1;
		}
		c -= (int)'a' - 'A';
	}
	return c < 'A' + base - 10 ? c - 'A' + 10 : -1;
}

static const __int128 i128_max = ~(((__int128)1) << 127);
static const __int128 i128_min = ((__int128)1) << 127;
static const unsigned __int128 u128_max = ~(unsigned __int128)0;

// this function tries to emulate strtoul/strtol and friends, including most of the bugs in the api
// in particular, str is a pointer to a string to parse, base is the base to use, and is_signed
// indicates whether or not the string is signed.
// base can be 2-36, or 0 to infer the base.  if any other value is supplied, errno is set to EINVAL,
// *end is set to str if end is not NULL, and 0 is returned.
// the string should have the format `\s*[+-](?:0[xX]?)?\d+`.
// "\s" is a whitespace character according to isspace().
// "\d" is a digit in the given base, which are the first `base` digits starting from 0 if `base <= 10`,
// with letters from a-z, case insensitive, if `base > 10`.
// the allowed prefix `(?:0[xX]?)?` depends on the base: if base is 16, `0[xX]` is allowed, but optional,
// and if base is 0, the base is inferred to be 16 if the prefix is `0[xX]`, 8 if the prefix is `0`, and 10
// otherwise.  notice that `0` is not considered to be part of the prefix if base is explicitly set to 8.
// numbers are always allowed to start with 0's.
// if matching the string to the format fails before any digit in the number is read, *end is set to str if
// applicable (NOT the first position which does not match the format, since having it set to str is
// faster to check and more convenient).  this includes if the string is empty or NULL.
// this is why it is important that leading 0's are considered to be part of the number rather than the prefix
// (besides the first 0 if base is 0).  if a non-digit character is encountered after at least one
// digit in the number has been read, the number read up until that point is converted to a 128-bit integer
// and then *end is set to point to the first non-digit character if applicable and the number read so far is
// returned.
// if the number read so far overflows, *end is set to pointto the first character AFTER the digit that caused
// overflow, errno is set to ERANGE, and a clamped limit is returned (ie, i128_min if is_signed and the number
// has a leading '-', i128_max if is_signed and the number does not have a leading '-', and u128_max otherwise).
// 
// bugs carried over from strtoul and friends:
// if !is_signed, a leading '-' is ignored rather than treated as an error.
// errno is not set to 0 in the absence of any error
// **end is not const
static inline unsigned __int128 strtou128(const char *str, char **end, int base, bool is_signed){
	if(end){
		*end = (char*)str;
	}
	if(base < 0 || base > 36 || base == 1){
		errno = EINVAL;
		return 0;
	}
	if(!str){
		return 0;
	}
	while(isspace(*str)){
		++str;
	}
	bool negative = *str == '-';
	if(negative || *str == '+'){
		++str;
	}
	if(!base){
		if(*str == '0'){
			if(!strncasecmp("x", str + 1, 1)){
				str += 2;
				base = 16;
			}else{
				++str;
				base = 8;
			}
		}else{
			base = 10;
		}
	}else if(base == 16 && !strncasecmp("0x", str + 1, 1)){
		str += 2;
	}
	int cur_digit = get_digit_base(*str, base);
	if(cur_digit == -1){
		return 0;
	}
	unsigned __int128 res = 0;
	// res*base <= -i128_min <-- res <= -i128_min/base
	// res*base > -i128_min <-- res >= (-i128_min + base)/base <-> res > -i128_min/base
	unsigned __int128 MAX = (is_signed ? (unsigned __int128)-i128_min : u128_max)/(unsigned __int128)base;
	unsigned __int128 safe_ceil = MAX/(unsigned __int128)base;
	while(1){
		cur_digit = get_digit_base(*++str, base);
		if(cur_digit == -1){
			break;
		}
		// res*base + cur_digit <= MAX
		// cur_digit <= MAX - res*base
		if(res > safe_ceil || (unsigned __int128)cur_digit > MAX - res*(unsigned __int128)base){
			if(end){
				*end = (char*)str;
			}
			errno = ERANGE;
			return is_signed ? negative ? (unsigned __int128)i128_min : (unsigned __int128)i128_max : u128_max;
		}
		res = res*(unsigned __int128)base + (unsigned __int128)cur_digit;
	}
	if(is_signed && !negative && res == (unsigned __int128)-i128_min){
		if(end){
			*end = (char*)str;
		}
		errno = ERANGE;
		return i128_max;
	}else if(end){
		*end = (char*)str;
	}
	return is_signed && negative ? -res : res;
}

bool cr8r_opt_parse_i128(cr8r_opt *self, char *opt){
	char *end = opt;
	errno = 0;
	*(__int128*)self->dest = (__int128)strtou128(opt, &end, 0, true);
	return errno != ERANGE && end != opt && !*end;
}

bool cr8r_opt_parse_u128(cr8r_opt *self, char *opt){
	char *end = opt;
	errno = 0;
	*(unsigned __int128*)self->dest = strtou128(opt, &end, 0, false);
	return errno != ERANGE && end != opt && !*end;
}

static bool init_opt_tbls(cr8r_opt *opts, cr8r_opt_cfg *cfg, cr8r_hashtbl_t *long_opts, cr8r_hashtbl_t *short_opts){
	uint64_t num_opts = 0, num_long = 0, num_short = 0;
	for(cr8r_opt *opt; opt = opts + num_opts, opt->short_name || opt->long_name; ++num_opts){
		int char_len;
		if(opt->arg_mode && !opt->on_opt){
			fprintf(stderr, "Crater opt error: invalid opt spec (argument to opt allowed but on_opt missing)\n");
			return false;
		}else if(opt->long_name && !*opt->long_name){
			fprintf(stderr, "Crater opt error: invalid opt spec (long_name is empty string (use NULL instead))\n");
			return false;
		}else if(opt->short_name && !*opt->short_name){
			fprintf(stderr, "Crater opt error: invalid opt spec (short_name is empty string (use NULL instead))\n");
			return false;
		}else if(opt->short_name && ((char_len = mblen(opt->short_name, MB_CUR_MAX)) == -1 || opt->short_name[char_len])){
			fprintf(stderr, "Crater opt error: invalid opt spec (short_name contains invalid multibyte character or multiple locale characters)\n");
			return false;
		}else if(opt->short_name && strchr("-= ", *opt->short_name)){
			fprintf(stderr, "Crater opt error: invalid opt spec (short_name is '-', '=', or ' ', which are not permitted)\n");
			return false;
		}else if(opt->long_name){
			for(uint64_t i = 0; (char_len = mblen(opt->long_name + i, MB_CUR_MAX)) > 0; i += char_len){
				if(char_len == 1 && strchr("= ", opt->long_name[i])){
					fprintf(stderr, "Crater opt error: invalid opt spec (long_name contains '=' or ' ', which are not permitted)\n");
					return false;
				}
			}
		}else if(opt->arg_mode < 0 || opt->arg_mode > 2){
			fprintf(stderr, "Crater opt error: unknown value for arg_mode\n");
			return false;
		}
		opt->found = false;
		num_long += !!opt->long_name;
		num_short += !!opt->short_name;
	}
	if(!cr8r_hash_init(long_opts, &cr8r_htft_cstr_u64, num_long*3)){
		fprintf(stderr, "Crater opt error: could not allocate long_opts table!\n");
		return false;
	}
	if(!cr8r_hash_init(short_opts, &cr8r_htft_cstr_u64, num_short*3)){
		cr8r_hash_destroy(long_opts, &cr8r_htft_cstr_u64);
		fprintf(stderr, "Crater opt error: could not allocate short_opts table!\n");
		return false;
	}
	bool success = true;
	for(uint64_t i = 0; i < num_opts; ++i){
		int status;
		if(opts[i].short_name){
			cr8r_hash_insert(short_opts, &cr8r_htft_cstr_u64, &(cr8r_htent_cstr_u64){opts[i].short_name, i}, &status);
			if(status != 1){
				fprintf(stderr, "Crater opt error: %s\n", status ? "duplicate short name" : "hash_insert failed!");
				success = false;
				break;
			}
		}
		if(opts[i].long_name){
			cr8r_hash_insert(long_opts, &cr8r_htft_cstr_u64, &(cr8r_htent_cstr_u64){opts[i].long_name, i}, &status);
			if(status != 1){
				fprintf(stderr, "Crater opt error: %s\n", status ? "duplicate long name" : "hash_insert failed!");
				success = false;
				break;
			}
		}
	}
	if(!success){
		cr8r_hash_destroy(long_opts, &cr8r_htft_cstr_u64);
		cr8r_hash_destroy(short_opts, &cr8r_htft_cstr_u64);
	}
	return success;
}

static const char *strmbchr(const char *str, const char *mbchr){
	for(int i = 0, char_len; (char_len = mblen(str + i, MB_CUR_MAX)) > 0; i += char_len){
		if(!memcmp(str + i, mbchr, char_len)){
			return str + i;
		}
	}
	return NULL;
}

static inline bool parse_long_opt(cr8r_hashtbl_t *long_opts, cr8r_opt *opts, cr8r_opt_cfg *cfg, int argc, char **argv, int *argi){
	char *opt = NULL;
	cr8r_htent_cstr_u64 *ent = NULL;
	char *eq_chr = (char*)strmbchr(argv[*argi] + 2, "=");
	if(eq_chr){
		*eq_chr = '\0';
	}
	ent = cr8r_hash_get(long_opts, &cr8r_htft_cstr_u64, &(cr8r_htent_cstr_u64){.str=argv[*argi] + 2});
	if(!ent){
		fprintf(stderr, "Unrecognized option --%s\n", argv[*argi] + 2);
		if(eq_chr){
			*eq_chr = '=';
		}
		++*argi;
		return false;
	}
	if(eq_chr){
		*eq_chr = '=';
	}
	if(eq_chr){//option argument can be passed after an '=' in the same argv entry
		opt = eq_chr + 1;
		++*argi;
		if(opts[ent->n].arg_mode == 0){
			fprintf(stderr, "Option --%s does not take an argument\n", opts[ent->n].long_name);
			opts[ent->n].found = true;
			return false;
		}
	}else if(opts[ent->n].arg_mode == 0){
		++*argi;
		opts[ent->n].found = true;
		return true;
	}else if(*argi + 1 != argc && *argv[*argi + 1] != '-'){//or in the next argv entry, if it does not start with '-'
		opt = argv[*argi + 1];
		*argi += 2;
	}else if(opts[ent->n].arg_mode == 1){//or there is no option argument
		fprintf(stderr, "Option --%s missing required argument\n", opts[ent->n].long_name);
		++*argi;
		opts[ent->n].found = true;
		return false;
	}//if arg_mode == 2 and on_opt is not NULL, on_opt must be able to tolerate NULL opt
	if(opts[ent->n].on_opt){
		if(!opts[ent->n].on_opt(opts + ent->n, opt)){
			if(opt){
				fprintf(stderr, "Invalid argument to option --%s\n", opts[ent->n].long_name);
			}else{
				fprintf(stderr, "Crater opt error: option --%s has arg_mode 2 but on_opt failed on NULL.  arg_mode or on_opt should be changed\n", opts[ent->n].long_name);
			}
			opts[ent->n].found = true;
			return false;
		}
	}
	opts[ent->n].found = true;
	return true;
}

static inline bool parse_short_optgrp(cr8r_hashtbl_t *short_opts, cr8r_opt *opts, cr8r_opt_cfg *cfg, int argc, char **argv, int *argi){
	char *opt = NULL;
	cr8r_htent_cstr_u64 *ent = NULL;
	if(!argv[*argi][1]){
		fprintf(stderr, "Stray \"-\" in argv\n");
		++*argi;
		return false;
	}
	bool success = true, reading_group = true;
	for(int j = 1, char_len; reading_group && argv[*argi][j]; j += char_len){
		if(!success && cfg->stop_on_first_err){
			return false;
		}
		char_len = mblen(argv[*argi] + j, MB_CUR_MAX);
		if(strchr("-= ", argv[*argi][j]) || char_len <= 0){
			fprintf(stderr, "Invalid short option name ('-', '=', ' ', '\\n', or a bad multibyte character)\n");
			success = false;
			continue;
		}
		char tmp_chr = argv[*argi][j + char_len];
		argv[*argi][j + char_len] = '\0';
		ent = cr8r_hash_get(short_opts, &cr8r_htft_cstr_u64, &(cr8r_htent_cstr_u64){.str=argv[*argi] + j});
		argv[*argi][j + char_len] = tmp_chr;
		if(!ent){
			fprintf(stderr, "Unrecognized option -%.*s\n", char_len, argv[*argi] + j);
			success = false;
			continue;
		}
		if(tmp_chr == '='){
			opt = argv[*argi] + j + char_len + 1;
			++*argi;
			reading_group = false;
			if(opts[ent->n].arg_mode == 0){
				fprintf(stderr, "Option -%s does not take an argument\n", opts[ent->n].short_name);
				success = false;
				break;
			}
		}else if(opts[ent->n].arg_mode == 0){
			opts[ent->n].found = true;
			continue;
		}else if(!tmp_chr && *argi + 1 != argc && *argv[*argi + 1] != '-'){
			opt = argv[*argi + 1];
			*argi += 2;
			reading_group = false;
		}else if(opts[ent->n].arg_mode == 1){
			fprintf(stderr, "Option -%s missing required argument (are you missing a ' '/'='?)\n", opts[ent->n].short_name);
			success = false;
			opts[ent->n].found = true;
			continue;
		}
		if(opts[ent->n].on_opt){
			if(!opts[ent->n].on_opt(opts + ent->n, opt)){
				if(opts[ent->n].arg_mode == 2 && !opt){
					fprintf(stderr, "Crater opt error: option -%s has arg_mode 2 but on_opt failed on NULL.  arg_mode or on_opt should be changed\n", opts[ent->n].short_name);
				}else{
					fprintf(stderr, "Invalid argument to option -%s\n", opts[ent->n].short_name);
				}
				success = false;
				opts[ent->n].found = true;
				continue;
			}
		}
		opts[ent->n].found = true;
	}
	if(reading_group){
		++*argi;
	}
	return success;
}

bool cr8r_opt_parse(cr8r_opt *opts, cr8r_opt_cfg *cfg, int argc, char **argv){
	cr8r_hashtbl_t long_opts, short_opts;
	if(!init_opt_tbls(opts, cfg, &long_opts, &short_opts)){
		return false;
	}
	bool success = true;
	for(int i = 1; i < argc;){
		if(*argv[i] != '-'){
			if(!cfg->on_arg || !cfg->on_arg(cfg->data, argc, argv, i)){
				success = false;
			}
		}else if(argv[i][1] == '-'){
			if(!argv[i][2]){//encountered "--"; done
				break;
			}//otherwise we have a "--long_name" option
			success &= parse_long_opt(&long_opts, opts, cfg, argc, argv, &i);
		}else{
			success &= parse_short_optgrp(&short_opts, opts, cfg, argc, argv, &i);
		}
		if(!success && cfg->stop_on_first_err){
			cr8r_hash_destroy(&long_opts, &cr8r_htft_cstr_u64);
			cr8r_hash_destroy(&short_opts, &cr8r_htft_cstr_u64);
			return false;
		}
	}
	cr8r_hash_destroy(&long_opts, &cr8r_htft_cstr_u64);
	cr8r_hash_destroy(&short_opts, &cr8r_htft_cstr_u64);
	for(cr8r_opt *opt = opts; opt->short_name || opt->long_name; ++opt){
		if(!opt->found && opt->on_missing){
			if(!opt->on_missing(opt)){
				if(opt->short_name){
					if(opt->long_name){
						fprintf(stderr, "Crater opt error: Option -%s/--%s was not provided but its on_missing callback failed\n", opt->short_name, opt->long_name);
					}else{
						fprintf(stderr, "Crater opt error: Option -%s was not provided but its on_missing callback failed\n", opt->short_name);
					}
				}else{
					fprintf(stderr, "Crater opt error: Option --%s was not provided but its on_missing callback failed\n", opt->long_name);
				}
				success = false;
			}
		}
		if(!opt->found && !opt->on_missing){
			if(opt->short_name){
				if(opt->long_name){
					fprintf(stderr, "Missing required option -%s/--%s\n", opt->short_name, opt->long_name);
				}else{
					fprintf(stderr, "Missing required option -%s\n", opt->short_name);
				}
			}else{
				fprintf(stderr, "Missing required option --%s\n", opt->long_name);
			}
			success = false;
		}
		if(!success && cfg->stop_on_first_err){
			return false;
		}
	}
	return success;
}

