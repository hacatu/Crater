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

double cr8r_rndchk_cov_bytes(cr8r_prng *prng, uint64_t n){
	if(!n){
		return 0.;
	}
	uint32_t r = cr8r_prng_get_u32(prng);
	uint64_t s = 0, s2 = 0;
	uint64_t x0 = r&0xFF, xn, l = 0x100;
	for(uint64_t i = 0;; r = cr8r_prng_get_u32(prng)){
		for(uint64_t i = 0;; r >>= 8){
			uint64_t x = r&0xFF;
			s += x;
			if(l < 0x100){
				s2 += x*l;
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
	int64_t num1 = (4*n - 1)*s2;
	int64_t num2 = (s - x0)*(s - xn);
	double denom = (4*n - 1)*(4*n - 1);
	return (num1 - num2)/denom;
}

int main(){
	fprintf(stderr, "\e[1;34mTesting random number generators...\e[0m\n");
	cr8r_prng *prng = cr8r_prng_init_lcg(0x29470e6ed1b94291);
	fprintf(stderr, "\e[1;34mTesting %d %s samples with Chi-squared byte counts test...\e[0m\n", 1000000, "LCG");
	double test_stat = cr8r_rndchk_chi2_bytes(prng, 1000000);
	fprintf(stderr, "\e[1;33m --> %f\e[0m\n", test_stat);
	fprintf(stderr, "\e[1;34mTesting %d %s samples with sequential bytes covariance test...\e[0m\n", 1000000, "LCG");
	test_stat = cr8r_rndchk_cov_bytes(prng, 1000000);
	fprintf(stderr, "\e[1;33m --> %f\e[0m\n", test_stat);
	free(prng);
	
	prng = cr8r_prng_init_system();
	fprintf(stderr, "\e[1;34mTesting %d %s samples with Chi-squared byte counts test...\e[0m\n", 1000000, "System");
	test_stat = cr8r_rndchk_chi2_bytes(prng, 1000000);
	fprintf(stderr, "\e[1;33m --> %f\e[0m\n", test_stat);
	fprintf(stderr, "\e[1;34mTesting %d %s samples with sequential bytes covariance test...\e[0m\n", 1000000, "System");
	test_stat = cr8r_rndchk_cov_bytes(prng, 1000000);
	fprintf(stderr, "\e[1;33m --> %f\e[0m\n", test_stat);
	free(prng);
	
	prng = cr8r_prng_init_lfg("\x52\x7c\xb4\x61\xad\x1f\x11\x13\xe3\xfa\x06\x4f\xe7\xa1\x2c\x47\xe0\x16\xa3\x68\x65\x1b\x77\x01\x77\x02\x9e\x6f\xbc\x2a\x64\x4e\x53\x09\xd3\x6c\x93\xe4\x88\xec\x3d\xc1\xb1\xbb\x2a\x84\xe7\x75\x2e\x16\xd6\x58\x76\xc0\xcf\xd5\x41\xd7\xfe\xe7\xdc\xb4\x10\x7b\x3c\xa2\x4d\x7e\x0f\x4b\xde\x4b\x75");
	fprintf(stderr, "\e[1;34mTesting %d %s samples with Chi-squared byte counts test...\e[0m\n", 1000000, "LFG");
	test_stat = cr8r_rndchk_chi2_bytes(prng, 1000000);
	fprintf(stderr, "\e[1;33m --> %f\e[0m\n", test_stat);
	fprintf(stderr, "\e[1;34mTesting %d %s samples with sequential bytes covariance test...\e[0m\n", 1000000, "LFG");
	test_stat = cr8r_rndchk_cov_bytes(prng, 1000000);
	fprintf(stderr, "\e[1;33m --> %f\e[0m\n", test_stat);
	free(prng);
	
	prng = cr8r_prng_init_xoro("\x7c\x93\xf5\x0d\x2f\x24\xd4\x67\x44\x38\x1b\x89\xa3\x13\xdc\xd5\x3b\x3e\x4a\xf5\x33\x49\xbe\x9e\x72\x8f\x3c\x78\xc1\xfe\xc9\xad");
	fprintf(stderr, "\e[1;34mTesting %d %s samples with Chi-squared byte counts test...\e[0m\n", 1000000, "Xoro");
	test_stat = cr8r_rndchk_chi2_bytes(prng, 1000000);
	fprintf(stderr, "\e[1;33m --> %f\e[0m\n", test_stat);
	fprintf(stderr, "\e[1;34mTesting %d %s samples with sequential bytes covariance test...\e[0m\n", 1000000, "Xoro");
	test_stat = cr8r_rndchk_cov_bytes(prng, 1000000);
	fprintf(stderr, "\e[1;33m --> %f\e[0m\n", test_stat);
	free(prng);
	
	prng = cr8r_prng_init_mt(0xb95f32c4886c1d36);
	fprintf(stderr, "\e[1;34mTesting %d %s samples with Chi-squared byte counts test...\e[0m\n", 1000000, "MT");
	test_stat = cr8r_rndchk_chi2_bytes(prng, 1000000);
	fprintf(stderr, "\e[1;33m --> %f\e[0m\n", test_stat);
	fprintf(stderr, "\e[1;34mTesting %d %s samples with sequential bytes covariance test...\e[0m\n", 1000000, "MT");
	test_stat = cr8r_rndchk_cov_bytes(prng, 1000000);
	fprintf(stderr, "\e[1;33m --> %f\e[0m\n", test_stat);
	free(prng);
}

