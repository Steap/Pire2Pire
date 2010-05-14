#include <stdio.h>              // FILE
#include <string.h>             // strcpy ()
#include <unistd.h>  
#include <stdlib.h>

#include "block_hole_map.h"

int
main (void) {
	struct block_hole_map *map = block_hole_map_new (0, 999);
	bhm_print(map);
	block_hole_fill_gap (map, 5, 554);
	bhm_print(map);
	block_hole_fill_gap (map, 745, 990);
	bhm_print(map);
	block_hole_fill_gap (map, 991, 998);
	bhm_print(map);
	block_hole_fill_gap (map, 747, 991);
	bhm_print(map);
	block_hole_fill_gap (map, 3, 7);
	bhm_print(map);
	block_hole_fill_gap (map, 3, 4);
	bhm_print(map);
	
	block_hole_map_free (map);

	return EXIT_SUCCESS;
}
					
