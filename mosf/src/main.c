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
#include <mos_api.h>
#include <string.h>

void print_sysinfo();

int main(int argc, char * argv[])
{
	char filename[40];

	if (argc < 2) {
		printf("run %s . filename\n",argv[0]);
		return -1;
	}
	strcpy(filename,argv[1]);
	
	print_sysinfo();
	printf("\n");

	printf("DIR\n");

	printf("\n---------------------------------\n");
	mos_dir(".");
	printf("\n---------------------------------\n");
	printf("\n");

	/* This doesn't work 
	printf("Now redirect to a file\n");

	if (freopen("dir.txt","w",stdout) == NULL)
	{
		printf("freopen() failed\n");
		return -1;
	}

	mos_dir(".");

	fclose(stdout);
	*/

	printf("Check for existance of a file\n");

	uint8_t fh = mos_fopen(filename, FA_READ);
	if (fh == 0)
	{
		printf("Could not open file %s\n",filename);
	} else {
		printf("Found file handle %d\n",fh);

		FIL* fil = mos_getfil(fh);
		printf("Size : %lu\n",fil->obj.objsize);
		mos_fclose(fh);
	}


	printf("Press a key \n");
	getchar();
	
	return 0;
}

void print_sysinfo()
{
	printf("Screen size %dx%d\n",getsysvar_scrwidth(),getsysvar_scrheight());
	printf("Colours %d\n",getsysvar_scrColours());
	printf("Time: %lu\n",getsysvar_time());
}
