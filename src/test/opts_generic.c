#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include <crater/opts.h>

int main(int argc, char **argv){
	unsigned long long v_ull;
	long long v_ll;
	unsigned long v_ul;
	long v_l;
	unsigned v_u;
	int v_i;
	unsigned short v_us;
	short v_s;
	unsigned char v_uc;
	signed char v_sc;
	char v_c;
	float v_f;
	double v_d;
	long double v_ld;
	char *v_cstr;
	_Bool v_b;
	__int128 v_i128;
	unsigned __int128 v_u128;
	cr8r_opt options[] = {
		CR8R_OPT_HELP(options, "Crater generic option macro test\n"
			"Written by hacatu\n\n"),
		CR8R_OPT_GENERIC_OPTIONAL(&v_ull, NULL, "ull", "unsigned long long"),
		CR8R_OPT_GENERIC_OPTIONAL(&v_ll, NULL, "ll", "long long"),
		CR8R_OPT_GENERIC_OPTIONAL(&v_ul, NULL, "ul", "unsigned long"),
		CR8R_OPT_GENERIC_OPTIONAL(&v_l, "l", "l", "long"),
		CR8R_OPT_GENERIC_OPTIONAL(&v_u, "u", "u", "unsigned"),
		CR8R_OPT_GENERIC_OPTIONAL(&v_i, "i", "i", "int"),
		CR8R_OPT_GENERIC_OPTIONAL(&v_us, NULL, "us", "unsigned short"),
		CR8R_OPT_GENERIC_OPTIONAL(&v_s, "s", "s", "short"),
		CR8R_OPT_GENERIC_OPTIONAL(&v_uc, NULL, "uc", "unsigned char"),
		CR8R_OPT_GENERIC_OPTIONAL(&v_sc, NULL, "sc", "signed char"),
		CR8R_OPT_GENERIC_OPTIONAL(&v_c, "c", NULL, "char"),
		CR8R_OPT_GENERIC_OPTIONAL(&v_f, "f", "f", "float"),
		CR8R_OPT_GENERIC_OPTIONAL(&v_d, "d", "d", "double"),
		CR8R_OPT_GENERIC_OPTIONAL(&v_ld, NULL, "ld", "long double"),
		CR8R_OPT_GENERIC_OPTIONAL(&v_cstr, NULL, "cstr", "null terminated string"),
		CR8R_OPT_GENERIC_OPTIONAL(&v_b, "b", "b", "_Bool"),
		CR8R_OPT_GENERIC_OPTIONAL(&v_i128, NULL, "i128", "__int128"),
		CR8R_OPT_GENERIC_OPTIONAL(&v_u128, NULL, "u128", "unsigned __int128"),
		CR8R_OPT_END()
	};
	if(!cr8r_opt_parse(options, &((cr8r_opt_cfg){.on_arg=cr8r_opt_ignore_arg}), argc, argv)){
		exit(1);
	}
	char pbuf[41];
	if(options[1].found){fprintf(stderr, "v_ull=%llu\n", v_ull);}
	if(options[2].found){fprintf(stderr, "v_ll=%lli\n", v_ll);}
	if(options[3].found){fprintf(stderr, "v_ul=%lu\n", v_ul);}
	if(options[4].found){fprintf(stderr, "v_l=%li\n", v_l);}
	if(options[5].found){fprintf(stderr, "v_u=%u\n", v_u);}
	if(options[6].found){fprintf(stderr, "v_i=%i\n", v_i);}
	if(options[7].found){fprintf(stderr, "v_us=%hu\n", v_us);}
	if(options[8].found){fprintf(stderr, "v_s=%hi\n", v_s);}
	if(options[9].found){fprintf(stderr, "v_uc=%hhu\n", v_uc);}
	if(options[10].found){fprintf(stderr, "v_sc=%hhi\n", v_sc);}
	if(options[11].found){fprintf(stderr, CHAR_MIN ? "v_c=%hhi\n" : "v_c=%hhu\n", v_c);}
	if(options[12].found){fprintf(stderr, "v_f=%f\n", v_f);}
	if(options[13].found){fprintf(stderr, "v_d=%f\n", v_d);}
	if(options[14].found){fprintf(stderr, "v_ld=%Lf\n", v_ld);}
	if(options[15].found){fprintf(stderr, "v_cstr=%s\n", v_cstr);}
	if(options[16].found){fprintf(stderr, "v_b=%hhu\n", (unsigned char)v_b);}
	if(options[17].found){fprintf(stderr, "v_i128=%s\n", cr8r_sprint_i128(pbuf, v_i128));}
	if(options[18].found){fprintf(stderr, "v_u128=%s\n", cr8r_sprint_u128(pbuf, v_u128));}
}

