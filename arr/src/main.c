/*
 * Title:			Hello World - C example
 * Author:			Dean Belfield
 * Created:			22/06/2022
 * Last Updated:	22/11/2022
 *
 * Modinfo:
 */
 
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// Parameters:
// - argc: Argument count
// - argv: Pointer to the argument string - zero terminated, parameters separated by spaces
//

int main(int argc, char * argv[])
{
	if (argc < 2) {
		printf("run %s . <size in Kb> [repeat]\n",argv[0]);
		printf("Pass size of array to malloc in Kbytes\n");
		return -1;
	}
	int size = atoi(argv[1]);
	int num_arr = 1;
	if (argc > 2) {
		num_arr = atoi(argv[2]);
	}

	uint8_t** arrs = (uint8_t **) malloc(sizeof(uint8_t*)*num_arr);

	// Alloc
	int count = 0;	
	while (count< num_arr)
	{
		printf("Alloc array size %dkb (%d bytes)\n",size, size*1024);
		arrs[count] = (uint8_t*)malloc(size*1024*sizeof(uint8_t));
		printf("Populate with rand values\n");
		for (int i=0;i<size*1024;i++)
		{
			arrs[count][i] = rand()%256;
			if (i%1024==0) {printf(".");}
		}
		printf("Done.\n");
		count++;
	}

	printf("Free arrays\n");
	count = 0;	
	while (count< num_arr)
	{
		free(arrs[count]);
		count++;
	}

	free(arrs);
	
	return 0;
}
