#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

void usage(char *name) {
	printf("%s [RGB|RGBA] <fInile> <fOutile> [REV]\n",name);
}

int main(int argc, char **argv)
{
	FILE *fIn,*fOut;

	char *fInile;
	char *fOutile;
	bool bAlpha=false, bReverse=false;
	int comps=3;

	if (argc<4) {
		usage(argv[0]);
		return -1;
	}
	if (strcmp(argv[1],"RGBA")==0) {
		bAlpha=true;
		comps=4;
		printf("4 componenet RGBA input\n");
	} else {
		printf("3 componenet RGB input\n");
	}
	fInile=argv[2];
	fOutile=argv[3];
	
	fIn=fopen(fInile, "r");
	if (!fIn) { 
		printf("Error opening %s\n",fInile);
		return -1;
	}
	fOut=fopen(fOutile, "w");
	if (!fOut) { 
		printf("Error opening %s\n",fOutile);
		return -1;
	}

	if (argc==5) {
		if (strcmp(argv[4],"REV")==0) {
			bReverse=true;
		}
	}
	if (bReverse)
	{
		printf("Output BGRA\n");
	} else {
		printf("Output RGBA\n");
	}
	
	while (!feof(fIn))
	{
		uint8_t comps[4];
		if (fread(comps, 1, bAlpha?4:3, fIn)>0) {
			uint8_t colindex;
			if (bReverse) {
				colindex = 0xC0 | ((comps[0]/85) << 4) | ((comps[1]/85) << 2) | (comps[2]/85);
			}
			else {
				colindex = 0xC0 | ((comps[2]/85) << 4) | ((comps[1]/85) << 2) | (comps[0]/85);
			}

			//printf("0x%02X 0x%02X 0x%02X  --> 0x%02X\n",comps[0], comps[1],comps[2], colindex);
			fputc(colindex,fOut);
		}
	}
	fclose(fIn);
	fclose(fOut);

	return 0;
}

