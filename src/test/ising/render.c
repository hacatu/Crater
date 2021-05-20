#include <SDL2/SDL.h>
#include <errno.h>
#include <stdio.h>

#include "ising.h"

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 800;

SDL_Window *window;
SDL_Renderer *renderer;

void quit(void){
	if(renderer){
		SDL_DestroyRenderer(renderer);
	}
	if(window){
		SDL_DestroyWindow(window);
	}
	SDL_Quit();
}

void init(void){
	if(SDL_Init(SDL_INIT_VIDEO)){
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}
	atexit(quit);
	if(!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")){
		printf("Warning: Linear texture filtering not enabled!");
	}
	window = SDL_CreateWindow("Construction Visualization", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	if(!window){
		printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if(renderer == NULL){
		printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}
	SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
}

typedef struct{
	uint64_t size;
	double B;
	uint64_t spin_masks[];
} ising2D_lattice;

static void draw_lattice(const ising2D_lattice *self){
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	for(uint64_t i = 0; i < self->size*self->size; ++i){
		if(self->spin_masks[i >> 6] & (1ull << (i&0x3F))){
			SDL_Rect site = {
				.x = (i%self->size)*SCREEN_WIDTH/self->size,
				.y = (i/self->size)*SCREEN_HEIGHT/self->size,
				.w = SCREEN_WIDTH/self->size,
				.h = SCREEN_HEIGHT/self->size
			};
			SDL_RenderFillRect(renderer, &site);
		}
	}
}

int main(void){
	fprintf(stderr, "\e[1;34mRunning Ising Model tests on prngs...\e[0m\n");
	double energy_probs[50];
	for(uint64_t n = 2; n < 6; ++n){
		fprintf(stderr, "\e[1;33mn=%"PRIu64"\e[0m\n", n);
		if(!ising2D_compute_energy_probs(n, 1, energy_probs)){
			fprintf(stderr, "\e[1;31mERROR: Could not compute pdf for energy\e[0m\n");
			exit(1);
		}
		double mean_energy = 0;
		uint64_t N = n*n;
		for(uint64_t i = 0; i < N + 1; ++i){
			int64_t energy = (int64_t)(4*i) - (int64_t)(2*N);
			mean_energy += energy*energy_probs[i];
			fprintf(stderr, "\e[1;32mP(H=%"PRIi64")=%f\e[0m\n", energy, energy_probs[i]);
		}
		fprintf(stderr, "\e[1;32mE(H)=%f\e[0m\n", mean_energy);
	}
	init();
	uint64_t n = 400, N = n*n, spin_masks_nmemb = N/64 + 1;
	ising2D_lattice *latt = calloc(offsetof(ising2D_lattice, spin_masks) + spin_masks_nmemb*sizeof(uint64_t), 1);
	if(!latt){
		exit(1);
	}
	cr8r_prng *prng = NULL;
	{
		cr8r_prng *sys_rng = cr8r_prng_init_system();
		if(!sys_rng){
			fprintf(stderr, "\e[1;31mERROR: Could not initialize sys rng!\e[0m\n");
			exit(1);
		}
		uint64_t seed = cr8r_prng_get_u64(sys_rng);
		free(sys_rng);
		prng = cr8r_prng_init_lfg_m(seed);
		if(!prng){
			fprintf(stderr, "\e[1;31mERROR: Could not initialize prng!\e[0m\n");
			exit(1);
		}
		fprintf(stderr, "\e[1;33mseed=%"PRIx64"\e[0m\n", seed);
	}
	latt->size = n;
	latt->B = 10;
	cr8r_prng_get_bytes(prng, spin_masks_nmemb*sizeof(uint64_t), latt->spin_masks);
	draw_lattice(latt);
	SDL_RenderPresent(renderer);
	SDL_Event e;
	while(1){
		int start_time = SDL_GetTicks();
		while(SDL_PollEvent(&e)){
			if(e.type == SDL_QUIT){
				free(prng);
				free(latt);
				exit(0);
			}else if(e.type == SDL_KEYDOWN){
				switch(e.key.keysym.sym){
				case SDLK_LEFT:
					break;
				case SDLK_RIGHT:
					break;
				case SDLK_DOWN:
					break;
				case SDLK_UP:
					break;
				}
			}
		}
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderFillRect(renderer, &(SDL_Rect){0, 0, SCREEN_WIDTH, SCREEN_HEIGHT});
		for(uint64_t i = 0; i < 1000; ++i){
			ising2D_step(latt, prng);
		}
		draw_lattice(latt);
		SDL_RenderPresent(renderer);
		
		int elapsed_time = SDL_GetTicks() - start;
		if(elapsed_time < 0){
			continue;//time overflowed
		}
		int sleep_time = 16 - elapsed_time;
		if(sleep_time > 0){
			SDL_Delay(sleep_time);
		}
	}
}

