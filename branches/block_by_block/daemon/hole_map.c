#include <stdlib.h>
#include <stdio.h>

#include "hole_map.h"
#include "../util/logger.h"

/* FIXME : to be included when compiled with the rest of the project
extern FILE *log_file;
#include <errno.h>
#include "../util/logger.h"
*/

struct hole_map *
hole_map_new (int s, int e){
	struct hole_map *map = (struct hole_map *)malloc(sizeof(struct hole_map));
	
	map->start = s;
	map->end = e;
	map->next = NULL;
	
	return map;
}

/* FIXME : avoid annoying printfs to report internal errors
 * 
 * Use log_failure(log_file, "..."); instead
*/
struct hole_map *
hole_map_fill_gap (struct hole_map *map, int s, int e){
	if (map == NULL) {
		
		printf ("!!! error while inserting from %d to %d. \n", s, e);
		printf ("=> no blocks are needed, file is complete !\n");
		return map;
	}
	if (s < map->start) {
		//FIXME : avoid annoying printfs to report internal errors
		printf ("!!! error while inserting from %d to %d. \n", s, e);
		printf ("=> block collision X : between %d and %d !\n", s, map->start - 1);
		return map;
	}	
	if ((s > map->end) && (map->next != NULL)) {
		hole_map_fill_gap (map->next, s, e);
		return map;
	}	
	if (s == map->start) {
		if (e < map->end) {
			map->start = e + 1;
		}
		else if (e == map->end) {
			//struct hole_map *obsolete_map;
			//obsolete_map = map;
			map = map->next;
			//free (obsolete_map);
		}
		else if (e > map->end) {
			//FIXME : avoid annoying printfs to report internal errors
			printf ("!!! error while inserting from %d to %d. \n", s, e);	
			printf ("=> block collision Y : between %d and %d !\n", map->end + 1, e);
		}
		return map;
	}
	if (s > map->start) {
		if (e < map->end) {		
			struct hole_map *inserted_map = hole_map_new (e + 1, map->end);
			map->end = s - 1;
			inserted_map->next = map->next;
			map->next = inserted_map;
			return map;
		}
		else if (e == map->end) {
			map->end = s - 1;
			return map;
		}
		else {
			//FIXME : avoid annoying printfs to report internal errors
			printf ("!!! error while inserting from %d to %d. \n", s, e);
			printf ("=> block collision Z : between %d and %d !\n", map->end + 1, e);
			return map;
		}
	}
	return map;
}
	
void
hole_map_free (struct hole_map *map) {
	if (map == NULL) return;
	if (map->next != NULL) {
		hole_map_free (map->next);
	}
	free (map);
	map = NULL;
	return;		
}

void
hole_map_print (struct hole_map *map) {
	printf ("**** hole_map ****\n");
	hole_map_print_rec (map);
	printf ("******************\n");
}

void
hole_map_print_rec (struct hole_map *map) {
	if (map == NULL)
		printf ("file is complete !\n");
	else {
		printf ("[%d , %d] missing\n", map->start, map->end);
		if (map->next != NULL)
			hole_map_print_rec (map->next);
	}
	return;
}

