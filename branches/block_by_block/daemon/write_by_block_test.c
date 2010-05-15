#include <stdlib.h>
#include <stdio.h>

#include "hole_map.h"

void
copy_file_block (char *buffer, FILE* f_out, int start, int end);

int
main (void) { 
	FILE *f_out;
	
	char *buffer = "alex\nrepain\n21\nyeah...";
	
	struct hole_map *map = hole_map_new (0, 22);
	hole_map_print (map);

	if ((f_out = fopen("testFileOut.txt", "w+")) == NULL) {
		printf("error while loading output file.\n");
		return EXIT_FAILURE;
	}
	
	
	copy_file_block (buffer, f_out, 20, 22);
	map = hole_map_fill_gap (map, 20, 22);
	hole_map_print (map);
	
	copy_file_block (buffer, f_out, 0, 8);
	map = hole_map_fill_gap (map, 0, 8);
	hole_map_print (map);
	
	copy_file_block (buffer, f_out, 10, 11);
	map = hole_map_fill_gap (map, 10, 11);
	hole_map_print (map);
	
	copy_file_block (buffer, f_out, 12, 17);
	map = hole_map_fill_gap (map, 12, 17);
	hole_map_print (map);
	
	copy_file_block (buffer, f_out, 9, 9);
	map = hole_map_fill_gap (map, 9, 9);
	hole_map_print (map);
    
    
    copy_file_block (buffer, f_out, 18, 19);
	map = hole_map_fill_gap (map, 18, 19);
	hole_map_print (map);
    
    
	fclose(f_out);
	hole_map_free (map);
	
	return EXIT_SUCCESS;
}
	
void
copy_file_block (char *buffer, FILE* f_out, int start, int end) {
	fseek (f_out, start, SEEK_SET);
	if (fwrite (&(buffer[start]), (end-(start-1)) * sizeof(char), 1, f_out) < 1) {
				printf("error while copying block.\n");
				return;
	}   
		
	return;
}
	
