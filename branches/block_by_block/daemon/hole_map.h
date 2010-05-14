#ifndef HOLE_MAP
#define HOLE_MAP

#include <stdlib.h>
#include <stdio.h>

struct hole_map {
	int 			start;
	int 			end;
	struct hole_map *next;
};

struct hole_map * 
hole_map_new (int s, int e);

struct hole_map *
hole_map_fill_gap (struct hole_map *map, int given_start, int given_end);

void
hole_map_free (struct hole_map *map);

void
hole_map_print (struct hole_map *map);

void
hole_map_print_rec (struct hole_map *map);

#endif
