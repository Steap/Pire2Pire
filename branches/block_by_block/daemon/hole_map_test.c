#include <stdlib.h>
#include <stdio.h>

#include "hole_map.h"

int
main (void) {

	struct hole_map *map = hole_map_new (0, 1000);
	hole_map_print (map);
	
	map = hole_map_fill_gap (map, 5, 10);
	hole_map_print (map);
	
	map = hole_map_fill_gap (map, 11, 11);
	hole_map_print (map);
	
	map = hole_map_fill_gap (map, 4, 4);
	hole_map_print (map);
	
	map = hole_map_fill_gap (map, 100, 200);
	hole_map_print (map);
	
	map = hole_map_fill_gap (map, 90, 105);
	hole_map_print (map);
	
	map = hole_map_fill_gap (map, 90, 99);
	hole_map_print (map);
	
	map = hole_map_fill_gap (map, 190, 300);
	hole_map_print (map);
	
	map = hole_map_fill_gap (map, 201, 310);
	hole_map_print (map);
	
	map = hole_map_fill_gap (map, 0, 3);
	hole_map_print (map);
	
		map = hole_map_fill_gap (map, 12, 90);
	hole_map_print (map);
	
	map = hole_map_fill_gap (map, 12, 89);
	hole_map_print (map);
	
	map = hole_map_fill_gap (map, 311, 1000);
	hole_map_print (map);
	
	map = hole_map_fill_gap (map, 7, 8);
	hole_map_print (map);
	
	hole_map_free (map);

	return EXIT_SUCCESS;
}
	
	
