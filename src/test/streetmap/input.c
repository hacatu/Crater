#define _POSIX_C_SOURCE 200809L
#include "crater/container.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <crater/hash.h>
#include <crater/pheap.h>
#include <crater/sla.h>

typedef struct{
	uint64_t u, v, way;
	double length;
} street_edge;

typedef struct street_neighbor street_neighbor;
struct street_neighbor{
	uint64_t id;
	street_neighbor *next;
};

typedef struct{
	uint64_t id;
	double lat, lon, dist;
	street_neighbor *out;
	bool visited;
	cr8r_pheap_node imeta;
} street_node;

typedef struct{
	uint64_t id;
	const char *name;
} way_data;

typedef struct{
	cr8r_hashtbl_t nodes, edges, ways;
	cr8r_pheap_node *frontier;
	uint64_t start_id, target_id;
} graph;

static uint64_t hash_edge(const cr8r_base_ft *ft, const void *_a){
	const street_edge *a = _a;
	return a->u^(a->v*131);
}

static int cmp_edges(const cr8r_base_ft *ft, const void *_a, const void *_b){
	const street_edge *a = _a, *b = _b;
	if(a->u < b->u){
		return -1;
	}else if(a->u > b->u){
		return 1;
	}else if(a->v < b->v){
		return -1;
	}else if(a->v > b->v){
		return 1;
	}
	return 0;
}

static void del_way(cr8r_base_ft *ft, void *_a){
	way_data *a = _a;
	free((void*)a->name);
}

static cr8r_hashtbl_ft edge_ft = {
	.base.size=sizeof(street_edge),
	.hash=hash_edge,
	.cmp=cmp_edges,
	.load_factor=.7
};

static cr8r_hashtbl_ft node_ft = {
	.base.size=sizeof(street_node),
	.hash=cr8r_default_hash_u64,
	.cmp=cr8r_default_cmp_u64,
	.load_factor=.7
};

static cr8r_hashtbl_ft way_ft = {
	.base.size=sizeof(way_data),
	.hash=cr8r_default_hash_u64,
	.cmp=cr8r_default_cmp_u64,
	.del=del_way,
	.load_factor=.7
};

static int cmp_lookup_node_dists(const cr8r_base_ft *ft, const void *_a, const void *_b){
	street_node key = {.id=*(const uint64_t*)_a};
	const street_node *a = cr8r_hash_get(ft->data, &node_ft, &key);
	key.id = *(const uint64_t*)_b;
	const street_node *b = cr8r_hash_get(ft->data, &node_ft, &key);
	if(!a){
		return !!b;
	}else if(!b || a->dist < b->dist){
		return -1;
	}else if(a->dist > b->dist){
		return 1;
	}
	return 0;
}

static cr8r_pheap_ft node_pqft = {
	.base={
		.size=offsetof(street_node, imeta)
	},
	.cmp=cmp_lookup_node_dists
};

static cr8r_sla neighbor_sla;

static bool read_file(const char *path, graph *out){
	FILE *f = fopen(path, "r");
	if(!f){
		return 0;
	}
	cr8r_hash_init(&out->nodes, &node_ft, 50000);
	cr8r_hash_init(&out->edges, &edge_ft, 70000);
	cr8r_hash_init(&out->ways, &way_ft, 20000);
	char *line = NULL;
	size_t line_cap = 0;
	ssize_t line_len;
	uint64_t line_num = 0;
	while((line_len = getline(&line, &line_cap, f)) != -1){
		++line_num;
		if(line[line_len - 1] == '\n'){
			line[--line_len] = ' ';
		}
		uint64_t col = 0;
		const char *cols[6] = {};
		for(char *pos, *tok = strtok_r(line, ",", &pos); tok; tok = strtok_r(NULL, ",", &pos)){
			cols[col++] = tok;
		}
		if(!strcmp(cols[0], "edge")){
			street_edge edge = {
				.u=strtoull(cols[2], NULL, 10),
				.v=strtoull(cols[3], NULL, 10),
				.way=strtoull(cols[1], NULL, 10),
				.length=strtod(cols[4], NULL)
			};
			cr8r_hash_insert(&out->edges, &edge_ft, &edge, NULL);
			if(cols[5][0] == '\0'){
				continue;
			}
			way_data data = {.id=edge.way, .name=strdup(cols[5])};
			int status;
			cr8r_hash_insert(&out->ways, &way_ft, &data, &status);
			if(status != 1){
				free((void*)data.name);
			}
		}else if(!strcmp(cols[0], "node")){
			street_node node = {
				.id=strtoull(cols[1], NULL, 10),
				.lat=strtod(cols[2], NULL),
				.lon=strtod(cols[3], NULL),
				.dist=INFINITY
			};
			cr8r_hash_insert(&out->nodes, &node_ft, &node, NULL);
		}
	}
	printf("Read %"PRIu64" lines; found %"PRIu64" nodes and %"PRIu64" edges\n", line_num, out->nodes.full, out->edges.full);
	cr8r_sla_init(&neighbor_sla, sizeof(street_neighbor), 2*out->edges.full);
	for(street_edge *it = cr8r_hash_next(&out->edges, &edge_ft, NULL); it; it = cr8r_hash_next(&out->edges, &edge_ft, it)){
		street_node *u = cr8r_hash_get(&out->nodes, &node_ft, &(street_node){.id=it->u});
		street_neighbor *neighbor = u->out;
		u->out = cr8r_sla_alloc(&neighbor_sla);
		*u->out = (street_neighbor){.id=it->v, neighbor};
		street_node *v = cr8r_hash_get(&out->nodes, &node_ft, &(street_node){.id=it->v});
		neighbor = v->out;
		v->out = cr8r_sla_alloc(&neighbor_sla);
		*v->out = (street_neighbor){.id=it->u, neighbor};
	}
	fclose(f);
	free(line);
	return 1;
}

int main(int argc, char **argv){
	if(argc != 2){
		fprintf(stderr, "\e[1;31mPlease specify the file to read!\e[0m\n");
		exit(EXIT_FAILURE);
	}
	graph graph = {.start_id=104314819, .target_id=104304705};
	node_pqft.base.data = &graph.nodes;
	read_file(argv[1], &graph);
	street_node *start = cr8r_hash_get(&graph.nodes, &node_ft, &(street_node){.id=graph.start_id});
	graph.frontier = CR8R_INNER_S(start, &node_pqft);
	start->dist = 0;
	start->visited = true;
	street_node *curr;
	void *tmp;
	while((tmp = cr8r_pheap_pop(&graph.frontier, &node_pqft))){
		curr = CR8R_OUTER_S(tmp, &node_pqft);
		if(curr->id == graph.target_id){
			break;
		}
		for(street_neighbor *out = curr->out; out; out = out->next){
			street_edge key = {.u=curr->id, .v=out->id};
			if(key.v < key.u){
				key.u = out->id;
				key.v = curr->id;
			}
			street_edge *edge = cr8r_hash_get(&graph.edges, &edge_ft, &key);
			street_node *node = cr8r_hash_get(&graph.nodes, &node_ft, &(street_node){.id = out->id});
			double dist_here = curr->dist + edge->length;
			if(!node->visited){
				node->visited = true;
				node->dist = dist_here;
				graph.frontier = cr8r_pheap_meld(graph.frontier, &node->imeta, &node_pqft);
			}else if(dist_here < node->dist){// TODO: this is redundant for completed nodes
				node->dist = dist_here;
				graph.frontier = cr8r_pheap_decreased_key(&node->imeta, &node_pqft);
			}
		}
	}
	printf("Computed distance from College Farm Road to Allison Road: %f m\n", curr->dist);
	cr8r_sla_delete(&neighbor_sla);
	cr8r_hash_destroy(&graph.edges, &edge_ft);
	cr8r_hash_destroy(&graph.nodes, &node_ft);
	cr8r_hash_destroy(&graph.ways, &way_ft);
}

