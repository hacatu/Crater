#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>

#include <crater/prand.h>

double cr8r_rndchk_chi2_bytes(cr8r_prng *prng, uint64_t n){
	uint64_t *counters = calloc(256, sizeof(uint64_t));
	if(!counters){
		return -1.;
	}
	for(uint64_t i = 0; i < n; ++i){
		uint32_t r = cr8r_prng_get_u32(prng);
		for(uint64_t i = 0; i < 4; ++i){
			++counters[r&0xFF];
			r >>= 8;
		}
	}
	uint64_t num = 0;
	for(uint64_t i = 0; i < 256; ++i){
		uint64_t t = 256*counters[i];
		t = t >= 4*n ? t - 4*n : 4*n - t;
		num += t*t;
	}
	free(counters);
	return num/(1024.*n);
}

double cr8r_rndchk_corr_bytes(cr8r_prng *prng, uint64_t n){
	if(!n){
		return 0.;
	}
	uint32_t r = cr8r_prng_get_u32(prng);
	uint64_t s = 0, s2 = 0, sl = 0;
	uint64_t x0 = r&0xFF, xn, l = 0x100;
	for(uint64_t i = 0;; r = cr8r_prng_get_u32(prng)){
		for(uint64_t i = 0;; r >>= 8){
			uint64_t x = r&0xFF;
			s += x;
			s2 += x*x;
			if(l < 0x100){
				sl += x*l;
			}
			l = x;
			if(i++ == 3){
				break;
			}
		}
		if(++i == n){
			xn = r;
			break;
		}
	}
	int64_t num_cov = (4*n - 1)*sl - (s - x0)*(s - xn);
	int64_t num_var_0 = (4*n - 1)*(s2 - x0*x0) - (s - x0)*(s - x0);
	int64_t num_var_n = (4*n - 1)*(s2 - xn*xn) - (s - xn)*(s - xn);
	return num_cov/(sqrt(num_var_0)*sqrt(num_var_n));
}

static double chi2_prob_wilson_hilferty(double chi2, uint64_t k){
	double z = (cbrt(chi2/k) - (1. - 2./(9.*k)))/(2./(9.*k));
	return .5 - .5*erf(M_SQRT1_2*z);
}

int main(){
	fprintf(stderr, "\e[1;34mTesting random number generators...\e[0m\n");
	cr8r_prng *prng = cr8r_prng_init_lcg(0x29470e6ed1b94291);
	fprintf(stderr, "\e[1;34mTesting %d %s samples with Chi-squared byte counts test...\e[0m\n", 1000000, "LCG");
	double test_stat = cr8r_rndchk_chi2_bytes(prng, 1000000);
	fprintf(stderr, "\e[1;33m --> %f (p ~= %f)\e[0m\n", test_stat, chi2_prob_wilson_hilferty(test_stat, 255));
	fprintf(stderr, "\e[1;34mTesting %d %s samples with sequential bytes correlation test...\e[0m\n", 1000000, "LCG");
	test_stat = cr8r_rndchk_corr_bytes(prng, 1000000);
	fprintf(stderr, "\e[1;33m --> %f\e[0m\n", test_stat);
	free(prng);
	
	prng = cr8r_prng_init_system();
	fprintf(stderr, "\e[1;34mTesting %d %s samples with Chi-squared byte counts test...\e[0m\n", 1000000, "System");
	test_stat = cr8r_rndchk_chi2_bytes(prng, 1000000);
	fprintf(stderr, "\e[1;33m --> %f (p ~= %f)\e[0m\n", test_stat, chi2_prob_wilson_hilferty(test_stat, 255));
	fprintf(stderr, "\e[1;34mTesting %d %s samples with sequential bytes correlation test...\e[0m\n", 1000000, "System");
	test_stat = cr8r_rndchk_corr_bytes(prng, 1000000);
	fprintf(stderr, "\e[1;33m --> %f\e[0m\n", test_stat);
	free(prng);
	
	prng = cr8r_prng_init_lfg_sc(0x6374e583f47f55cb);
	fprintf(stderr, "\e[1;34mTesting %d %s samples with Chi-squared byte counts test...\e[0m\n", 1000000, "SC LFG");
	test_stat = cr8r_rndchk_chi2_bytes(prng, 1000000);
	fprintf(stderr, "\e[1;33m --> %f (p ~= %f)\e[0m\n", test_stat, chi2_prob_wilson_hilferty(test_stat, 255));
	fprintf(stderr, "\e[1;34mTesting %d %s samples with sequential bytes correlation test...\e[0m\n", 1000000, "SC LFG");
	test_stat = cr8r_rndchk_corr_bytes(prng, 1000000);
	fprintf(stderr, "\e[1;33m --> %f\e[0m\n", test_stat);
	free(prng);
	
	prng = cr8r_prng_init_lfg_m(0x8990c29a1d6c6ded);
	fprintf(stderr, "\e[1;34mTesting %d %s samples with Chi-squared byte counts test...\e[0m\n", 1000000, "M LFG");
	test_stat = cr8r_rndchk_chi2_bytes(prng, 1000000);
	fprintf(stderr, "\e[1;33m --> %f (p ~= %f)\e[0m\n", test_stat, chi2_prob_wilson_hilferty(test_stat, 255));
	fprintf(stderr, "\e[1;34mTesting %d %s samples with sequential bytes correlation test...\e[0m\n", 1000000, "M LFG");
	test_stat = cr8r_rndchk_corr_bytes(prng, 1000000);
	fprintf(stderr, "\e[1;33m --> %f\e[0m\n", test_stat);
	free(prng);
	
	
	prng = cr8r_prng_init_xoro(0x31ebf64ab4a7f90e);
	fprintf(stderr, "\e[1;34mTesting %d %s samples with Chi-squared byte counts test...\e[0m\n", 1000000, "Xoro");
	test_stat = cr8r_rndchk_chi2_bytes(prng, 1000000);
	fprintf(stderr, "\e[1;33m --> %f (p ~= %f)\e[0m\n", test_stat, chi2_prob_wilson_hilferty(test_stat, 255));
	fprintf(stderr, "\e[1;34mTesting %d %s samples with sequential bytes correlation test...\e[0m\n", 1000000, "Xoro");
	test_stat = cr8r_rndchk_corr_bytes(prng, 1000000);
	fprintf(stderr, "\e[1;33m --> %f\e[0m\n", test_stat);
	free(prng);
	
	prng = cr8r_prng_init_mt(0xb95f32c4886c1d36);
	fprintf(stderr, "\e[1;34mTesting %d %s samples with Chi-squared byte counts test...\e[0m\n", 1000000, "MT");
	test_stat = cr8r_rndchk_chi2_bytes(prng, 1000000);
	fprintf(stderr, "\e[1;33m --> %f (p ~= %f)\e[0m\n", test_stat, chi2_prob_wilson_hilferty(test_stat, 255));
	fprintf(stderr, "\e[1;34mTesting %d %s samples with sequential bytes correlation test...\e[0m\n", 1000000, "MT");
	test_stat = cr8r_rndchk_corr_bytes(prng, 1000000);
	fprintf(stderr, "\e[1;33m --> %f\e[0m\n", test_stat);
	free(prng);
}

