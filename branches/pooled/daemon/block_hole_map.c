#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "block_hole_map.h"

struct block_hole_map * 
block_hole_map_new (int start, int end) {
	struct block_hole_map* map;
	
	map = (struct block_hole_map *)malloc(sizeof (struct block_hole_map));
	
	map->start_block = start;
	map->end_block = end;
	map->next = NULL;
	map->previous = NULL;
	
	return map;
	
}

void 
block_hole_fill_gap (struct block_hole_map *map, int received_start, 
					int received_end) {
	// FIXME : doesn't take in account errors 
	// like a block received that would override already received blocs.

	if (received_start < map->start_block) {
		printf ("error while filling with [%d,%d]\n", 
				received_start, received_end);
		return;
	}
	if (received_start > map->end_block) {
		if (map->next != NULL) {
			block_hole_fill_gap (map->next, received_start, received_end);
			return;
		}

	} 
	if (received_start == map->start_block) {
		if (received_end < map->end_block) {
			map->start_block = received_end + 1;
		}

		else {
			// FIXME : completed the first gap 
			// => unlink and free the block_hole_map
		}
		return;
	}
	else {
		if (received_end < map->end_block) {
			struct block_hole_map *inserted_map =  
				block_hole_map_new (received_end + 1, map->end_block);
			map->end_block = received_start - 1;
			if (map->next!=NULL)
				map->next->previous = inserted_map;
			inserted_map->next = map->next;
			map->next = inserted_map;
			inserted_map->previous = map;
		}
		else if (received_end > map->end_block) {
			printf ("error while filling with [%d,%d]\n", 
				received_start, received_end);
			return;
		}
		else {
			map->end_block = received_start - 1;
		}
		return;
	}

}
		
void 
block_hole_map_free (struct block_hole_map *map) {
	if (map == NULL) return;
	
	if (map->next == NULL) {}
	else {
		block_hole_map_free (map->next);
	}
	free (map);
	map = NULL;
}

void 
bhm_print (struct block_hole_map *map) {
	printf ("***hole map:***\n");
	if (map != NULL)
		bhm_print_rec (map);
		printf ("***************\n");
}

void 
bhm_print_rec (struct block_hole_map *map) {
	if (map != NULL) {
		printf ("%d <......> %d\n", map->start_block, map->end_block);
		if (map->next!=NULL)
			bhm_print_rec (map->next);
	}
}

