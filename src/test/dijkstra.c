// This test case is based on Project Euler problem 83
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <crater/hash.h>
#include <crater/pheap.h>

typedef struct{
	uint64_t i, j, weight;
} graph_edge;

static uint64_t hash_edge(const cr8r_base_ft *ft, const void *_a){
	const graph_edge *a = _a;
	return a->i^(a->j*83);
}

static int cmp_edge_key(const cr8r_base_ft *ft, const void *_a, const void *_b){
	const graph_edge *a = _a, *b = _b;
	if(a->i < b->i){
		return -1;
	}else if(a->i > b->i){
		return 1;
	}else if(a->j < b->j){
		return -1;
	}else if(a->j > b->j){
		return 1;
	}
	return 0;
}

cr8r_hashtbl_ft graph_edge_ft = {
	.base={
		.size=sizeof(graph_edge)
	},
	.hash=hash_edge,
	.cmp=cmp_edge_key,
	.load_factor=.7
};

typedef struct{
	uint64_t dist;
	bool visited;
	cr8r_pheap_node imeta;
} graph_node;

static int cmp_node_dist(const cr8r_base_ft *ft, const void *_a, const void *_b){
	const graph_node *a = _a, *b = _b;
	if(a->dist < b->dist){
		return -1;
	}else if(a->dist > b->dist){
		return 1;
	}
	return 0;
}

typedef struct{
	cr8r_hashtbl_t edges;
	uint64_t width, height, start_weight;
	graph_node *nodes;
	cr8r_pheap_node *frontier;
} graph;

cr8r_pheap_ft graph_pqft = {
	.base={
		.size=offsetof(graph_node, imeta)
	},
	.cmp=cmp_node_dist
};

static bool read_file(const char *path, graph *out){
	FILE *f = fopen(path, "r");
	out->edges.cap = 0;
	if(!f){
		goto ERROR;
	}
	if(!cr8r_hash_init(&out->edges, &graph_edge_ft, 1000)){
		goto ERROR;
	}
	uint64_t weight, col = 0, row = 0, idx = 0;
	char sep;
	for(int status;; ++idx){
		status = fscanf(f, "%"SCNu64"%c", &weight, &sep);
		if(status == 0 || status == EOF){
			break;
		}else if(status == 1){
			sep = '\n';
		}
		//printf("(%"PRIu64",%"PRIu64"):", col, row);
		if(!idx){
			out->start_weight = weight;
		}
		if(col){// add edge from the left if not in leftmost column
			graph_edge key = {idx-1, idx, weight};
			//printf(">%"PRIu64";%"PRIu64, key.i, key.j);
			if(!cr8r_hash_insert(&out->edges, &graph_edge_ft, &key, NULL)){
				goto ERROR;
			}
		}
		if(sep == ','){// add edge from the right if not in rightmost column
			graph_edge key = {idx+1, idx, weight};
			//printf(">%"PRIu64";%"PRIu64, key.i, key.j);
			if(!cr8r_hash_insert(&out->edges, &graph_edge_ft, &key, NULL)){
				goto ERROR;
			}
		}
		if(row){// add edge from top if not in top row, then add edge out of the top
			graph_edge key = {idx-out->width, idx, weight};
			//printf("v%"PRIu64";%"PRIu64, key.i, key.j);
			if(!cr8r_hash_insert(&out->edges, &graph_edge_ft, &key, NULL)){
				goto ERROR;
			}
			if(row > 1){// copy edges from row j-1 up to row j-2 to create the edges from j-1 to j
				graph_edge key = {idx-2*out->width, idx-out->width, 0};
				key = *(graph_edge*)cr8r_hash_get(&out->edges, &graph_edge_ft, &key);
				key.i = idx;
				//printf("v%"PRIu64";%"PRIu64, key.i, key.j);
				if(!cr8r_hash_insert(&out->edges, &graph_edge_ft, &key, NULL)){
					goto ERROR;
				}
			}else{// for the top row down into the second row, we must copy from horizontal edges
				graph_edge key = {col ? col - 1 : 1, col, 0};
				if(out->width > 1){
					key = *(graph_edge*)cr8r_hash_get(&out->edges, &graph_edge_ft, &key);
				}else{
					key.weight = out->start_weight;
				}
				key.i = idx;
				//printf("v%"PRIu64";%"PRIu64, key.i, key.j);
				if(!cr8r_hash_insert(&out->edges, &graph_edge_ft, &key, NULL)){
					goto ERROR;
				}
			}
		}
		if(sep == ','){
			++col;
		}else if(sep == '\n'){
			if(!row){
				out->width = col + 1;
			}else if(out->width != col + 1){
				fprintf(stderr, "\e[1;31mERROR: Line length mismatch!\e[0m\n");
				goto ERROR;
			}
			col = 0;
			++row;
		}else{
			fprintf(stderr, "\e[1;31mERROR: Invalid character in file '%c'\e[0m\n", sep);
			goto ERROR;
		}
		//printf("\n");
	}
	fclose(f);
	f = NULL;
	out->height = row;
	out->nodes = calloc(out->width*out->height, sizeof(*out->nodes));
	if(!out->nodes){
		goto ERROR;
	}
	for(uint64_t i = 0; i < out->width*out->height; ++i){
		out->nodes[i].dist = UINT64_MAX;
	}
	out->frontier = NULL;
	return 1;
	ERROR:;
	if(f){
		fclose(f);
	}
	if(out->edges.cap){
		cr8r_hash_destroy(&out->edges, &graph_edge_ft);
	}
	return 0;
}

static void visit_edge(graph *self, uint64_t i, uint64_t j){
	graph_node *source = self->nodes + i;
	graph_node *dest = self->nodes + j;
	graph_edge *edge = cr8r_hash_get(&self->edges, &graph_edge_ft, &(graph_edge){i, j, 0});
	uint64_t dist_here = source->dist + edge->weight;
	if(!dest->visited){
		dest->visited = true;
		dest->dist = dist_here;
		self->frontier = cr8r_pheap_meld(self->frontier, &dest->imeta, &graph_pqft);
	}else if(dist_here < dest->dist){// TODO: this is redundant for completed nodes
		dest->dist = dist_here;
		self->frontier = cr8r_pheap_decreased_key(&dest->imeta, &graph_pqft);
	}
}

int main(int argc, char **argv){
	if(argc != 2){
		fprintf(stderr, "\e[1;31mERROR: Invalid arguments.  Use like:\n%s <input file>\e[0m\n", argv[0]);
		return EXIT_FAILURE;
	}
	graph graph;
	if(!read_file(argv[1], &graph)){
		fprintf(stderr, "\e[1;31mERROR: Could not read file!\e[0m\n");
		return EXIT_FAILURE;
	}
	graph.nodes[0].dist = graph.start_weight;
	graph.nodes[0].visited = true;
	graph.frontier = CR8R_INNER_S(graph.nodes, &graph_pqft);
	for(graph_node *curr; (curr = CR8R_OUTER_S(cr8r_pheap_pop(&graph.frontier, &graph_pqft), &graph_pqft));){
		uint64_t idx = curr - graph.nodes;
		//printf("[%"PRIu64"]=%"PRIu64"\n", idx, graph.nodes[idx].dist);
		if(idx ==  graph.width*graph.height - 1){
			break;
		}
		if(idx >= graph.width){
			visit_edge(&graph, idx, idx - graph.width);
		}
		if(idx < graph.width*(graph.height - 1)){
			visit_edge(&graph, idx, idx + graph.width);
		}
		if(idx%graph.width){
			visit_edge(&graph, idx, idx - 1);
		}
		if(idx%graph.width != graph.width - 1){
			visit_edge(&graph, idx, idx + 1);
		}
	};
	uint64_t dist = graph.nodes[graph.width*graph.height - 1].dist;
	fprintf(stderr, dist == 425185 ? 
		"\e[1;32mSUCCESS: distance from top left to bottom right is %"PRIu64".\e[0m\n" :
		"\e[1;31mFAILED: got incorrect distance from top left to bottom right (%"PRIu64").\e[0m",
		dist);
	
	free(graph.nodes);
	cr8r_hash_destroy(&graph.edges, &graph_edge_ft);
}

