#ifndef BLOCK_HOLE_MAP_H
#define BLOCK_HOLE_MAP_H



struct block_hole_map {
	int 					start_block;
	int 					end_block;
	struct block_hole_map	*next;
	struct block_hole_map 	*previous; 

};

struct block_hole_map * 
block_hole_map_new (int start, int end);

void block_hole_fill_gap 
(struct block_hole_map *map, int received_start, int received_end);

		
void block_hole_map_free (struct block_hole_map *map);

void bhm_print (struct block_hole_map *map);
void bhm_print_rec (struct block_hole_map *map);
#endif//BLOCK_HOLE_MAP_H
