#include "globals.h"
#include "colmap.h"

#include "agon/vdp_vdu.h"
#include "agon/vdp_key.h"
#include <mos_api.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

#define CHUNK_SIZE 1024

#define MODE 8
#define SCREENW  320
#define SCREENH  240

static int gMapWidth = 64;
static int gMapHeight = 64;
#define TILESIZE 8

#define SCREENW_TILES SCREENW / TILESIZE
#define SCREENH_TILES SCREENH / TILESIZE

#define RIGHT 0
#define LEFT 1
#define UP 2
#define DOWN 3

#define COL(C) vdp_set_text_colour(C)
#define TAB(X,Y) vdp_cursor_tab(Y,X)

// Position of top-left of screen in world coords (pixel)
int xpos=0, ypos=0;

uint8_t* tilemap;

FILE *open_file( const char *fname, const char *mode);
int close_file( FILE *fp );
int read_str(FILE *fp, char *str, char stop);
int load_bitmap_file( const char *fname, int width, int height, int bmap_id );

void key_event_handler( KEY_EVENT key_event );
void wait_clock( clock_t ticks );
double my_atof(char *str);

void load_images();
void show_map();
void game_loop();

int getWorldCoordX(int sx) { return (xpos + sx); }
int getWorldCoordY(int sy) { return (ypos + sy); }
int getTileX(int sx) { return (sx/TILESIZE); }
int getTileY(int sy) { return (sy/TILESIZE); }
int getTilePosInScreenX(int tx) { return ((tx * TILESIZE) - xpos); }
int getTilePosInScreenY(int ty) { return ((ty * TILESIZE) - ypos); }

void draw_screen();
void scroll_screen(int dir, int step);
void draw_horizontal(int tx, int ty, int len);
void draw_vertical(int tx, int ty, int len);

int load_map(char *mapname);

typedef struct {
	char fname[20];
	uint8_t id;
	int nb[4];
	uint8_t key;
} TileInfoFile;

#define TILE_INFO_FILE "img/tileinfo.txt"
int readTileInfoFile(char *path, TileInfoFile *tif, int items);

void wait()
{
	char k=getchar();
	if (k=='q') exit(0);
}

int main(int argc, char *argv[])
{
	vdp_vdu_init();
	if ( vdp_key_init() == -1 ) return 1;
	vdp_set_key_event_handler( key_event_handler );

	gMapWidth = 256;
	gMapHeight = 256;
	tilemap = (uint8_t *) malloc(sizeof(uint8_t) * gMapWidth * gMapHeight);
	if (tilemap == NULL)
	{
		printf("Out of memory\n");
		return -1;
	}

	//if (load_map("maps/mapmap_dry_256x256.data") != 0)
	if (load_map("maps/myworld_wet_256x256.data") != 0)
	{
		printf("Failed to load map\n");
		goto my_exit;
	}

	vdp_mode(MODE);
	vdp_logical_scr_dims(false);

	load_images();

	game_loop();

my_exit:
	vdp_logical_scr_dims(true);
	free(tilemap);
	return 0;
}

void game_loop()
{
	int exit=0;
	draw_screen();
	do {
		int dir=-1;
		if ( vdp_check_key_press( 0x9a ) ) {dir=0; }	// right
		if ( vdp_check_key_press( 0x9c ) ) {dir=1; }	// left
		if ( vdp_check_key_press( 0x96 ) ) {dir=2; }	// up
		if ( vdp_check_key_press( 0x98 ) ) {dir=3; }	// down
		if (dir>=0) {
			scroll_screen(dir,1);
		}
		if ( vdp_check_key_press( 0x26 ) || vdp_check_key_press( 0x2D ) ) exit=1; // q or x

		//wait_clock(4);
		vdp_update_key_state();
	} while (exit==0);

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
	//printf("%X",key_event.code);
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


int load_bitmap_file( const char *fname, int width, int height, int bmap_id )
{
	FILE *fp;
	char *buffer;
	int bytes_remain = width * height;

	if ( !(buffer = (char *)malloc( CHUNK_SIZE ) ) ) {
		printf( "Failed to allocate %d bytes for buffer.\n",CHUNK_SIZE );
		return -1;
	}
	if ( !(fp = fopen( fname, "rb" ) ) ) {
		printf( "Error opening file \"%s\". Quitting.\n", fname );
		return -1;
	}

	vdp_adv_clear_buffer(0xFA00+bmap_id);

	bytes_remain = width * height;

	while (bytes_remain > 0)
	{
		int size = (bytes_remain>CHUNK_SIZE)?CHUNK_SIZE:bytes_remain;

		vdp_adv_write_block(0xFA00+bmap_id, size);

		if ( fread( buffer, 1, size, fp ) != (size_t)size ) return 0;
		mos_puts( buffer, size, 0 );
		//printf(".");

		bytes_remain -= size;
	}
	vdp_adv_consolidate(0xFA00+bmap_id);

	vdp_adv_select_bitmap(0xFA00+bmap_id);
	vdp_adv_bitmap_from_buffer(width, height, 1); // RGBA2
	
	fclose( fp );
	free( buffer );

	return bmap_id;
}

void load_images() 
{
	char fname[20];
	for (int fn=1; fn<=24; fn++)
	{
		sprintf(fname, "img/t%02d.rgb2",fn);
		//printf("load %s\n",fname);
		load_bitmap_file(fname, TILESIZE,TILESIZE, fn-1);
	}
}

// draw full screen at World position in xpos/ypos
void draw_screen()
{
	int tx=getTileX(xpos);
	int ty=getTileY(ypos);

	for (int i=0; i < (1+SCREENH/TILESIZE); i++) 
	{
		draw_horizontal(tx, ty+i, 1+(SCREENW/TILESIZE));
	}
}

void draw_horizontal(int tx, int ty, int len)
{
	int px=getTilePosInScreenX(tx);
	int py=getTilePosInScreenY(ty);

	for (int i=0; i<len; i++)
	{
		vdp_select_bitmap( tilemap[ty*gMapWidth + tx+i] );
		vdp_draw_bitmap( px + i*TILESIZE, py );
	}
	
}
void draw_vertical(int tx, int ty, int len)
{
	int px = getTilePosInScreenX(tx);
	int py = getTilePosInScreenY(ty);

	for (int i=0; i<len; i++)
	{
		vdp_select_bitmap( tilemap[(ty+i)*gMapWidth + tx] );
		vdp_draw_bitmap( px, py + i*TILESIZE );
	}
}

/* 0=right, 1, left, 2=up, 3=down */
void scroll_screen(int dir, int step)
{
	switch (dir) {
		case RIGHT:
			if (xpos > step)
			{
				xpos -= step;
				vdp_scroll_screen(dir, step);
				// draw tiles (tx,ty) to (tx,ty+len)
				int tx=getTileX(xpos);
				int ty=getTileY(ypos);
				draw_vertical(tx,ty, 1+(SCREENH/TILESIZE));
			}
			break;
		case LEFT:
			if ((xpos + SCREENW + step) < (gMapWidth * TILESIZE))
			{
				xpos += step;
				vdp_scroll_screen(dir, step);
				// draw tiles (tx,ty) to (tx,ty+len)
				int tx=getTileX(xpos + SCREENW -1);
				int ty=getTileY(ypos);
				draw_vertical(tx,ty, 1+(SCREENH/TILESIZE));
			}
			break;
		case UP:
			if (ypos > step)
			{
				ypos -= step;
				vdp_scroll_screen(dir, step);
				// draw tiles (tx,ty) to (tx+len,ty)
				int tx=getTileX(xpos);
				int ty=getTileY(ypos);
				draw_horizontal(tx,ty, 1+(SCREENW/TILESIZE));
			}
			break;
		case DOWN:
			if ((ypos + SCREENH + step) < (gMapHeight * TILESIZE))
			{
				ypos += step;
				vdp_scroll_screen(dir, step);
				// draw tiles (tx,ty) to (tx+len,ty)
				int tx=getTileX(xpos);
				int ty=getTileY(ypos + SCREENH -1);
				draw_horizontal(tx,ty, 1+(SCREENW/TILESIZE));
			}
			break;
		default:
			break;
	}
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
		tif[item].id = (uint8_t) atoi(pch[1]);
		tif[item].nb[0] = atoi(pch[2]);
		tif[item].nb[1] = atoi(pch[3]);
		tif[item].nb[2] = atoi(pch[4]);
		tif[item].nb[3] = atoi(pch[5]);

		item++;
	}

	close_file(fp);
	return item;
}

int load_map(char *mapname)
{
	uint8_t ret = mos_load( mapname, (uint24_t) tilemap,  gMapWidth * gMapHeight );
	return ret;
}
