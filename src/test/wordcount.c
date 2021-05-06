#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <crater/avl.h>
#include <crater/hash.h>
#include <crater/vec.h>
#include <crater/heap.h>
#include <crater/sla.h>

typedef struct{
	char *word;
	uint64_t count;
} word_count;

int combine_u64_then_free(cr8r_base_ft *ft, void *_e, void *_i){
	word_count *e = _e, *i = _i;
	e->count += i->count;
	free(i->word);
	return 1;
}

int cmp_counts(const cr8r_base_ft *ft, const void *_a, const void *_b){
	return cr8r_default_cmp_u64(ft, _a + offsetof(word_count, count), _b + offsetof(word_count, count));
}

cr8r_hashtbl_ft htft_wc = {
	.base = {
		.data = NULL,
		.size = sizeof(word_count)
	},
	.hash = cr8r_default_hash_cstr,
	.cmp = cr8r_default_cmp_cstr,
	.add = combine_u64_then_free,
	.del = cr8r_default_free,
	.load_factor = .5
};

cr8r_sla avl_wc_sla;
cr8r_avl_ft avlft_wc;

cr8r_vec_ft vecft_wc = {
	.base = {
		.data = NULL,
		.size = sizeof(word_count)
	},
	.new_size = cr8r_default_new_size,
	.resize = cr8r_default_resize,
	.del = NULL,
	.copy = NULL,
	.cmp = cmp_counts,
	.swap = cr8r_default_swap
};

char *mmap_open(const char *fname, uint64_t *text_len){
	int fd = open(fname, O_RDONLY);
	if(fd < 0){
		fprintf(stderr, "\e[1;31mERROR: Could not open file!\e[0m\n");
		return NULL;
	}
	struct stat stat_buf;
	if(fstat(fd, &stat_buf)){
		close(fd);
		fprintf(stderr, "\e[1;31mERROR: Could not stat file!\e[0m\n");
		return NULL;
	}
	*text_len = stat_buf.st_size;
	char *text_buf = mmap(NULL, *text_len, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);
	if(text_buf == MAP_FAILED){
		fprintf(stderr, "\e[1;31mERROR: Could not mmap file!\e[0m\n");
		return NULL;
	}
	return text_buf;
}

void hashtbl_appender(void *self, word_count *wc){
	int status;
	cr8r_hash_append(self, &htft_wc, wc, &status);
	if(!status){
		fprintf(stderr, "\e[1;31mERROR: Could not append word count in hashtbl!\e[0m");
	}
}

void avl_appender(void *self, word_count *wc){
	if(!strcmp(wc->word, "p")){
		self = self;
	}
	fprintf(stderr, "\e[1;33m&self=%p, self=%p\e[0m\n", self, *self);
	if(!cr8r_avl_insert_update(self, wc, &avlft_wc)){
		fprintf(stderr, "\e[1;31mERROR: Could not append word count in avl tree!\e[0m");
	}
}

void collect_words(uint64_t text_len, const char text_buf[static text_len], void (*f)(void*, word_count*), void *data){
	int in_word = 0;
	for(uint64_t b = 0, a; b < text_len; ++b){
		int is_letter = 0;
		if(('A' <= text_buf[b] && text_buf[b] <= 'Z') || ('a' <= text_buf[b] && text_buf[b] <= 'z')){
			is_letter = 1;
		}
		if(in_word){
			if(!is_letter){
				in_word = 0;
				word_count wc;
				wc.word = malloc((b - a + 1)*sizeof(char));
				if(!wc.word){
					fprintf(stderr, "\e[1;31mERROR: Could not allocate word buffer!\e[0m");
					continue;
				}
				for(uint64_t i = 0; i < b - a; ++i){
					if('A' <= text_buf[a + i] && text_buf[a + i] <= 'Z'){
						wc.word[i] = text_buf[a + i] + 'a' - 'A';
					}else{
						wc.word[i] = text_buf[a + i];
					}
				}
				wc.word[b - a] = '\0';
				wc.count = 1;
				f(data, &wc);
			}
		}else if(is_letter){
			in_word = 1;
			a = b;
		}
	}
}

int main(){
	fprintf(stderr, "\e[1;34mCounting all words in Frankenstein:\e[0m\n");
	uint64_t text_len;
	char *text_buf = mmap_open("../../resources/frankenstein.txt", &text_len);
	if(!text_buf){
		exit(1);
	}
	cr8r_hashtbl_t wordcount_ht;
	if(!cr8r_hash_init(&wordcount_ht, &htft_wc, 100)){
		munmap(text_buf, text_len);
		fprintf(stderr, "\e[1;31mERROR: Could not allocate hash table!\e[0m\n");
		exit(1);
	}
	cr8r_avl_node *wordcount_avl;
	if(!cr8r_avl_ft_initsla(&avlft_wc, &avl_wc_sla, sizeof(word_count), 100, cr8r_default_cmp_cstr, combine_u64_then_free)){
		cr8r_hash_destroy(&wordcount_ht, &htft_wc);
		munmap(text_buf, text_len);
		fprintf(stderr, "\e[1;31mERROR: Could not set up slab allocator!\e[0m\n");
		exit(1);
	}
	
	fprintf(stderr, "\e[1;34mUsing a hash table...\e[0m\n");
	collect_words(text_len, text_buf, hashtbl_appender, &wordcount_ht);
	fprintf(stderr, "\e[1;34mUsing an avl tree...\e[0m\n");
	collect_words(text_len, text_buf, avl_appender, &wordcount_avl);
	munmap(text_buf, text_len);
	
	fprintf(stderr, "\e[1;34mHash table found %"PRIu64" unique words\e[0m\n", wordcount_ht.full);
	cr8r_vec wordcount_vec;
	if(!cr8r_vec_init(&wordcount_vec, &vecft_wc, wordcount_ht.full)){
		cr8r_hash_destroy(&wordcount_ht, &htft_wc);
		cr8r_avl_delete(wordcount_avl, &avlft_wc);
		fprintf(stderr, "\e[1;31mERROR: Could not allocate vector!\e[0m\n");
		exit(1);
	}
	for(word_count *wc = cr8r_hash_next(&wordcount_ht, &htft_wc, NULL); wc; wc = cr8r_hash_next(&wordcount_ht, &htft_wc, wc)){
		cr8r_vec_pushr(&wordcount_vec, &vecft_wc, wc);
	}
	cr8r_heap_ify(&wordcount_vec, &vecft_wc, 1);
	fprintf(stderr, "\e[1;34mHash table top 100 words:\n");
	for(uint64_t i = 0; i < 100; ++i){
		word_count wc;
		if(!cr8r_heap_pop(&wordcount_vec, &vecft_wc, &wc, 1)){
			break;
		}
		fprintf(stderr, i ? ",  %s: %"PRIu64 : "{%s: %"PRIu64, wc.word, wc.count);
	}
	cr8r_vec_delete(&wordcount_vec, &vecft_wc);
	cr8r_hash_destroy(&wordcount_ht, &htft_wc);
	fprintf(stderr, "...}\e[0m\n");
	
	avlft_wc.cmp = cmp_counts;
	cr8r_avl_heapify(wordcount_avl, &avlft_wc);
	fprintf(stderr, "\e[1;34mAVL top 100 words:\n");
	for(uint64_t i = 0; i < 100 && wordcount_avl; ++i){
		word_count wc = *(word_count*)wordcount_avl->data;
		//we can leak the node here as long as we free wc.word (which it owns), since it lives on the slab
		cr8r_avl_heappop_node(&wordcount_avl, &avlft_wc);
		fprintf(stderr, i ? ",  %s: %"PRIu64 : "{%s: %"PRIu64, wc.word, wc.count);
		free(wc.word);
	}
	for(cr8r_avl_node *n = cr8r_avl_first(wordcount_avl); n; n = cr8r_avl_next(n)){
		free(*(void**)n->data);
	}
	fprintf(stderr, "...}\e[0m\n");
	cr8r_sla_delete(&avl_wc_sla);
}

