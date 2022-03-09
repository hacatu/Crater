#define _POSIX_C_SOURCE 200809L
#include "crater/container.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <wctype.h>
#include <errno.h>
#include <locale.h>

#include <crater/opts.h>
#include <crater/vec.h>

typedef struct{
	const char *filename;
	int modes;
	uint64_t lines, words, chars, bytes, colns;
} text_counter;

cr8r_vec_ft vecft_cstr_txtcnt = {
	.base.data = NULL,
	.base.size = sizeof(text_counter),
	.new_size = cr8r_default_new_size,
	.resize = cr8r_default_resize,
	.del = cr8r_default_free,
	.copy = NULL,
	.cmp = cr8r_default_cmp_cstr,
	.swap = cr8r_default_swap
};

bool print_version(cr8r_opt *self, char *opt){
	fprintf(stderr, "This is the word count test from Crater container library 0.3.0\n"
		"Copyright 2021 Gabriel Eiseman (hacatu)\n"
		"Distributed under MPL 2.0\n");
	exit(0);
}

bool combine_mode(cr8r_opt *self, char *opt){
	switch(*self->short_name){
	case 'l': *(uint64_t*)self->dest |= 1ull; break;
	case 'w': *(uint64_t*)self->dest |= 2ull; break;
	case 'm': *(uint64_t*)self->dest |= 4ull; break;
	case 'c': *(uint64_t*)self->dest |= 8ull; break;
	case 'L': *(uint64_t*)self->dest |= 16ull; break;
	default: return 0;
	}
	return 1;
}

bool parse_filename(void *data, int argc, char **argv, int i){
	const char *filename1 = NULL;
	if(strcmp(argv[i], "-") && !(filename1 = strdup(argv[i]))){
		fprintf(stderr, "Could not allocate space for filenames!\n");
		return 0;
	}
	return cr8r_vec_pushr(data, &vecft_cstr_txtcnt, &(text_counter){.filename = filename1});
}

bool parse_listfilename(cr8r_opt *self, char *opt){
	char *filename = NULL;
	size_t filename_cap = 0;
	FILE *listfile = strcmp(opt, "-") ? fopen(opt, "rb") : stdin;
	if(!listfile){
		fprintf(stderr, "Could not open list file \"%s\"!\n", opt);
		return 0;
	}
	bool success = true;
	while(1){
		errno = 0;
		int64_t filename_len = getdelim(&filename, &filename_cap, '\0', listfile);
		if(filename_len < 0){
			free(filename);
			if(errno){
				fprintf(stderr, "Could not allocate space for filenames!\n");
				success = false;
			}
			break;
		}else if(!filename_len || !*filename){
			continue;
		}
		char *filename1 = strndup(filename, filename_len);
		if(!filename1 || !cr8r_vec_pushr(self->dest, &vecft_cstr_txtcnt, &(text_counter){.filename = filename1})){
			free(filename);
			free(filename1);
			fprintf(stderr, "Could not allocate space for filenames!\n");
			success = false;
			break;
		}
	}
	if(listfile != stdin){
		fclose(listfile);
	}
	return success;
}

int iswnewline(wint_t wc){
	switch(wc){
	case L'\x0D'://carriage return
		return 2;//indicates the caller should check for a following line feed
	case L'\x0A'://line feed
	case L'\x0B'://vertical tab
	case L'\x0C'://form feed
	case L'\x85'://next line
	case L'\u2028'://line separator
	case L'\u2029'://paragraph separator
		return 1;
	}
	return 0;
}

void process_text(text_counter *counter, const char *text, uint64_t len){
	bool is_prev_space = true;
	int char_len;
	uint64_t line_len = 0;
	for(uint64_t i = 0; i < len; i += char_len){
		wchar_t wc;
		char_len = mbtowc(&wc, text + i, MB_CUR_MAX);
		char_len = char_len > 0 ? char_len : 1;
		counter->bytes += char_len;
		++counter->chars;
		bool is_cur_space = iswspace(wc);
		counter->words += !is_cur_space && is_prev_space;
		is_prev_space = is_cur_space;
		switch(iswnewline(wc)){
		case 2:
			if(text[i + 1] == '\x0A'){
				++counter->bytes;
				++counter->chars;
				++i;
			}
			//fallthrough
		case 1:
			++counter->lines;
			if(line_len > counter->colns){
				counter->colns = line_len;
			}
			line_len = 0;
			break;
		case 0:
			line_len += char_len;
		}
	}
}

bool process_file(text_counter *counter){
	char *line = NULL;
	size_t line_cap = 0;
	counter->bytes = counter->chars = counter->words = counter->lines = counter->colns = 0;
	FILE *file = counter->filename ? fopen(counter->filename, "rb") : stdin;
	if(!file){
		fprintf(stderr, "Could not open file \"%s\"!\n", counter->filename);
		return 1;
	}
	bool success = true;
	while(1){
		errno = 0;
		int64_t line_len = getline(&line, &line_cap, file);
		if(line_len < 0){
			free(line);
			if(errno){
				fprintf(stderr, "Could not process file \"%s\": %s!\n", counter->filename, strerror(errno));
				success = false;
			}
			break;
		}
		process_text(counter, line, line_len);
	}
	return success;
}

// 1 - 20
uint64_t num_digits10(uint64_t n){
	if(!n){
		return 1;
	}
	uint64_t res = 0;
	while(n){
		n /= 10;
		++res;
	}
	return res;
}

int main(int argc, char **argv){
	if(!setlocale(LC_ALL, "en_US.UTF-8")){
		fprintf(stderr, "\e[1;31mERROR: Could not set locale to en_US.UTF-8");
		exit(1);
	}
	text_counter counter = {};
	cr8r_vec filenames;
	if(!cr8r_vec_init(&filenames, &vecft_cstr_txtcnt, 8)){
		fprintf(stderr, "\e[1;31mERRROR: Could not allocate filenames vector!\e[0m\n");
		exit(1);
	}
	cr8r_opt options[] = {
		CR8R_OPT_HELP(options, "Basic line, word, and byte counting utility\n"
			"Written by hacatu\n\n"
			"Usage:\n"
			"\t`wc [<options>] <files>`: print counts for each file and total count.\n"
			"\t`wc [<options>] --files0-from=<file> [<files>]`: read list of null terminated filenames from <file>, and optionally additional files.\n"
			"Counts are printed in the order: lines, words, characters, bytes, maximum line length.\n"
			"If no count options are given, line, word, and byte counts are printed.\n"
			"If no files are given, standard input is read instead.  To read standard input as well as other files, \"-\" can be given as a filename.\n\n"),
		{.dest = &counter.modes, .arg_mode = CR8R_OPT_ARGMODE_NONE,
			.short_name = "l", .long_name = "lines",
			.description = "print line counts, where lines are separated by utf8 line separators, namely u000A, u000B, u000C, u000D, u000D followed by u000A, u0085, u2028, or u2029",
			.on_opt = combine_mode, .on_missing = cr8r_opt_missing_optional},
		{.dest = &counter.modes, .arg_mode = CR8R_OPT_ARGMODE_NONE,
			.short_name = "w", .long_name = "words",
			.description = "print word counts, where a word is defined as a maximal run of non-whitespace characters, as determined by iswspace",
			.on_opt = combine_mode, .on_missing = cr8r_opt_missing_optional},
		{.dest = &counter.modes, .arg_mode = CR8R_OPT_ARGMODE_NONE,
			.short_name = "m", .long_name = "chars",
			.description = "print character counts, counting multibyte utf8 characters, invalid utf8 bytes, and embedded null bytes as one character each",
			.on_opt = combine_mode, .on_missing = cr8r_opt_missing_optional},
		{.dest = &counter.modes, .arg_mode = CR8R_OPT_ARGMODE_NONE,
			.short_name = "c", .long_name = "bytes",
			.description = "print byte counts",
			.on_opt = combine_mode, .on_missing = cr8r_opt_missing_optional},
		{.dest = &counter.modes, .arg_mode = CR8R_OPT_ARGMODE_NONE,
			.short_name = "L", .long_name = "max-line-length",
			.description = "print length of longest line in bytes",
			.on_opt = combine_mode, .on_missing = cr8r_opt_missing_optional},
		{.dest = &filenames, .arg_mode = CR8R_OPT_ARGMODE_REQUIRED,
			.short_name = "f", .long_name = "files0-from",
			.description = "read list of null terminated filenames from the given argument, or stdin if \"-\" is given",
			.on_opt = parse_listfilename, .on_missing = cr8r_opt_missing_optional},
		{.dest = NULL, .arg_mode = CR8R_OPT_ARGMODE_NONE,
			.short_name = "v", .long_name = "version",
			.description = "print version and exit",
			.on_opt = print_version, .on_missing = cr8r_opt_missing_optional},
		CR8R_OPT_END()
	};
	if(!cr8r_opt_parse(options, &((cr8r_opt_cfg){.data = &filenames, .on_arg = parse_filename, .flags = CR8R_OPTS_ALLOW_STRAY_DASH}), argc, argv)){
		exit(1);
	}
	if(!filenames.len && !options[6].found){
		cr8r_vec_pushr(&filenames, &vecft_cstr_txtcnt, &(text_counter){});
	}
	counter.modes = counter.modes ?: 11;//count lines, words, and bytes by default
	for(uint64_t i = 0; i < filenames.len; ++i){
		text_counter *file_counter = cr8r_vec_get(&filenames, &vecft_cstr_txtcnt, i);
		process_file(file_counter);
		counter.bytes += file_counter->bytes;
		counter.chars += file_counter->chars;
		counter.words += file_counter->words;
		counter.lines += file_counter->lines;
		if(file_counter->colns > counter.colns){
			counter.colns = file_counter->colns;
		}
	}
	text_counter coln_widths = {
		.bytes = num_digits10(counter.bytes),
		.chars = num_digits10(counter.chars),
		.words = num_digits10(counter.words),
		.lines = num_digits10(counter.lines),
		.colns = num_digits10(counter.colns),
	};
	for(uint64_t i = 0; i < filenames.len; ++i){
		const text_counter *file_counter = cr8r_vec_get(&filenames, &vecft_cstr_txtcnt, i);
		if(counter.modes & 1){
			printf("%*"PRIu64" ", (int)coln_widths.lines, file_counter->lines);
		}
		if(counter.modes & 2){
			printf("%*"PRIu64" ", (int)coln_widths.words, file_counter->words);
		}
		if(counter.modes & 4){
			printf("%*"PRIu64" ", (int)coln_widths.chars, file_counter->chars);
		}
		if(counter.modes & 8){
			printf("%*"PRIu64" ", (int)coln_widths.bytes, file_counter->bytes);
		}
		if(counter.modes & 16){
			printf("%*"PRIu64" ", (int)coln_widths.colns, file_counter->colns);
		}
		if(filenames.len != 1){
			printf("%s\n", file_counter->filename ?: "");
		}else{
			printf("\n");
		}
	}
	if(filenames.len != 1){
		if(counter.modes & 1){
			printf("%*"PRIu64" ", (int)coln_widths.lines, counter.lines);
		}
		if(counter.modes & 2){
			printf("%*"PRIu64" ", (int)coln_widths.words, counter.words);
		}
		if(counter.modes & 4){
			printf("%*"PRIu64" ", (int)coln_widths.chars, counter.chars);
		}
		if(counter.modes & 8){
			printf("%*"PRIu64" ", (int)coln_widths.bytes, counter.bytes);
		}
		if(counter.modes & 16){
			printf("%*"PRIu64" ", (int)coln_widths.colns, counter.colns);
		}
		printf("total\n");
	}
	cr8r_vec_delete(&filenames, &vecft_cstr_txtcnt);
}

