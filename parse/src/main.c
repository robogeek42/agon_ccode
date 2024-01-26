#include "agon/vdp_vdu.h"
#include "agon/vdp_key.h"
#include <mos_api.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

FILE *open_file( const char *fname, const char *mode);
int close_file( FILE *fp );
int read_str(FILE *fp, char *str, char stop);

void key_event_handler( KEY_EVENT key_event );
void wait_clock( clock_t ticks );
double my_atof(char *str);

typedef struct {
	char fname[20];
	int id;
	int nb[4];
	uint8_t key;
} TileInfoFile;

int readTileInfoFile(char *path, TileInfoFile *tif, int items);

void wait()
{
	char k=getchar();
	if (k=='q') exit(0);
}

int main(/*int argc, char *argv[]*/)
{
	int items = readTileInfoFile("tileinfo.txt", NULL, 0);

	TileInfoFile *tif = malloc(sizeof(TileInfoFile) * items);
	items = readTileInfoFile("tileinfo.txt", tif, items);
	printf("Read %d tile info items\n",items);
	
	for (int i=0; i<items; i++)
	{
		tif[i].key = 0;
		for (int n=0; n<4; n++) {
			tif[i].key |= (tif[i].nb[n] & 0x3) << 2*n;
		}
		printf("tile %d (%01x %01x %01x %01x) key 0x%02X\n",i,tif[i].nb[0],tif[i].nb[1],tif[i].nb[2],tif[i].nb[3], tif[i].key);
	}

	uint8_t tileLU[256] = {0};
	for (int i=0; i<items; i++)
	{
		tileLU[ tif[i].key ] = tif[i].id;
	}
	for (uint8_t k=0; k<255; k++)
	{
		if (tileLU[k]>0) {
			printf("key 0x%02X tile %d\n", k, tileLU[k]);
		}
	}
	
	wait(); 

	return 0;
}

// Open file
FILE *open_file( const char *fname, const char *mode )
{

	FILE *fp ;

	if ( !(fp = fopen( fname, mode )) ) {
		printf( "Error opening file %s\r\n", fname);
		return NULL;
	}
	return fp;
}

// Close file
int close_file( FILE *fp )
{
	if ( fclose( fp ) ){
		puts( "Error closing file.\r\n" );
		return EOF;
	}
	return 0;
}

int read_str(FILE *fp, char *str, char stop) {
	char c;
	int cnt=0, bytes=0;
	do {
		bytes = fread(&c, 1, 1, fp);
		if (bytes != 0 && c != stop) {
			str[cnt++]=c;
		}
	} while (c != stop && bytes != 0);
	str[cnt++]=0;
	return bytes;
}
	
static KEY_EVENT prev_key_event = { 0 };
void key_event_handler( KEY_EVENT key_event )
{
	if ( key_event.code == 0x7d ) {
		vdp_cursor_enable( true );
		exit( 1 );						// Exit program if esc pressed
	}

	if ( key_event.key_data == prev_key_event.key_data ) return;
	prev_key_event = key_event;
}

void wait_clock( clock_t ticks )
{
	clock_t ticks_now = clock();

	do {
		vdp_update_key_state();
	} while ( clock() - ticks_now < ticks );
}

double my_atof(char *str)
{
	double f = 0.0;
	sscanf(str, "%lf", &f);
	return f;
}


int readTileInfoFile(char *path, TileInfoFile *tif, int items)
{
	char line[40];
	int tif_lines = items;
	FILE *fp = open_file(path, "r");
	if (fp == NULL) { return false; }

	// mode where we tell caller how many lines are in the file so it can malloc
	if (tif == NULL)
	{
		tif_lines = 0;
		while (fgets(line, 40, fp))
		{
			if (line[0] == '#') continue;
			tif_lines++;
		}
		close_file(fp);
		return tif_lines;
	}
	
	// write lines to tif
	fp = open_file(path, "r");
	if (fp == NULL) { return false; }

	int item=0;
	while (fgets(line, 40, fp))
	{
		if (line[0] == '#') continue;

		char *pch[8]; int i=0;

		// get first token
		pch[i] = strtok(line, ",");
		while (pch[i] != NULL)
		{
			// get next token
			i++;
			pch[i] = strtok(NULL,",");
		}
		strncpy(tif[item].fname,pch[0],20);
		tif[item].id = atoi(pch[1]);
		tif[item].nb[0] = atoi(pch[2]);
		tif[item].nb[1] = atoi(pch[3]);
		tif[item].nb[2] = atoi(pch[4]);
		tif[item].nb[3] = atoi(pch[5]);

		item++;
	}

	close_file(fp);
	return item;
}
