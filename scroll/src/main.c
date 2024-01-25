#include "agon/vdp_vdu.h"
#include "agon/vdp_key.h"
#include <mos_api.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define CHUNK_SIZE 1024

#define MODE 8
#define SCREENW  320
#define SCREENH  240

#define MAPW 40
#define MAPH 40
#define TILESIZE 16

#define SCREENW_TILES SCREENW / TILESIZE
#define SCREENH_TILES SCREENH / TILESIZE

#define RIGHT 0
#define LEFT 1
#define UP 2
#define DOWN 3

static int bExit=0;

// Position of top-left of screen in world coords (pixel)
static int xpos=0, ypos=0;

uint8_t map[MAPW][MAPH];

void wait() { char k=getchar(); if (k=='q') exit(0); }
void key_event_handler( KEY_EVENT key_event );
void wait_clock( clock_t ticks );

void game_loop();
void load_images();
void create_map(int w, int h);
void draw_screen();

int getWorldCoordX(int sx) { return (xpos + sx); }
int getWorldCoordY(int sy) { return (ypos + sy); }
int getTileX(int sx) { return (sx/TILESIZE); }
int getTileY(int sy) { return (sy/TILESIZE); }
int getTilePosInScreenX(int tx) { return ((tx * TILESIZE) - xpos); }
int getTilePosInScreenY(int ty) { return ((ty * TILESIZE) - ypos); }

void scroll_screen(int dir, int step);
void draw_horizontal(int tx, int ty, int len);
void draw_vertical(int tx, int ty, int len);

int main(/* int argc, char *argv[] */)
{
	vdp_vdu_init();
	if ( vdp_key_init() == -1 ) return 1;
	vdp_set_key_event_handler( key_event_handler );

	vdp_mode(MODE);
	vdp_logical_scr_dims(false);

	load_images();
	//getchar();
	create_map(MAPW, MAPH);
	
	/* start screen centred */
	xpos = TILESIZE*(MAPW - SCREENW/TILESIZE)/2; 
	ypos = TILESIZE*(MAPH - SCREENH/TILESIZE)/2; 

	draw_screen();

	game_loop();

	return 0;
}

void game_loop()
{
	do {
		int dir=-1;
		if ( vdp_check_key_press( 0x9a ) ) {dir=RIGHT; }
		if ( vdp_check_key_press( 0x9c ) ) {dir=LEFT; }
		if ( vdp_check_key_press( 0x96 ) ) {dir=UP; }
		if ( vdp_check_key_press( 0x98 ) ) {dir=DOWN; }
		if (dir>=0) {
			scroll_screen(dir, 2);
		}
		//wait_clock( 1 );
		vdp_update_key_state();
	} while (bExit==0);

}

static KEY_EVENT prev_key_event = { 0 };
void key_event_handler( KEY_EVENT key_event )
{
	if ( key_event.code == 0x7d ) {
		vdp_cursor_enable( true );
		exit( 1 ); // Exit immediately program if esc pressed
	}

	if ( key_event.key_data == prev_key_event.key_data ) return;
	prev_key_event = key_event;

	if ( key_event.ascii == 0x58 || key_event.ascii == 0x78 ) bExit=1; // x X
	if ( key_event.ascii == 0x51 || key_event.ascii == 0x71 ) bExit=1; // q Q
}

void wait_clock( clock_t ticks )
{
	clock_t ticks_now = clock();

	do {
		vdp_update_key_state();
	} while ( clock() - ticks_now < ticks );
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
		//ticksleep(100); // add a delay to try and fix issue on real Agon

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
	for (int fn=1; fn<=18; fn++)
	{
		sprintf(fname, "img/ts%02d.rgb2",fn);
		//printf("load %s\n",fname);
		load_bitmap_file(fname, TILESIZE,TILESIZE, fn);
	}
}

uint8_t lake[8][8] = {
	{ 1, 1, 1, 1, 1, 1, 1, 1 },
	{ 1,12, 5, 5,13, 1, 1, 1 },
	{ 1, 2,10,10, 3, 1, 1, 1 },
	{ 1, 2,10,10, 3,12, 5,13 },
	{ 1, 2,10,10, 3, 2,10, 3 },
	{ 1, 2,10,10, 3,11, 4,14 },
	{ 1,11, 4, 4,14, 1, 1, 1 },
	{ 1, 1, 1, 1, 1, 1, 1, 1 }
};
//data for a tile map 30x30 16x16 tiles
uint8_t terr_lakes[30][30] = {
	{ 1,1,1,1,1,18,1,1,1,1,1,1,1,1,1,1,1,1,1,1,18,1,1,1,1,1,1,1,1,1 },
	{ 1,1,1,1,18,1,18,18,1,1,17,1,18,1,1,1,1,1,1,18,1,18,18,1,1,17,1,18,1,1 },
	{ 1,1,17,18,18,1,1,1,18,1,1,1,17,18,1,1,1,17,18,18,1,1,1,18,1,1,1,17,18,1 },
	{ 1,1,12,5,5,5,5,5,5,13,1,18,18,1,1,1,1,12,5,5,5,5,5,5,13,1,18,18,1,1 },
	{ 1,1,2,10,10,10,10,10,10,3,1,1,1,1,1,1,1,2,10,10,10,10,10,10,3,1,1,1,1,1 },
	{ 1,1,11,6,10,10,10,10,10,3,1,1,1,1,1,1,1,11,6,10,10,10,10,10,3,1,1,1,1,1 },
	{ 1,1,1,2,10,10,10,10,9,14,1,17,1,1,1,1,1,1,2,10,10,10,10,9,14,1,17,1,1,1 },
	{ 1,1,1,2,10,10,10,10,3,1,1,1,1,18,1,1,1,1,2,10,10,10,10,3,1,1,1,1,18,1 },
	{ 1,1,1,11,6,10,10,9,14,1,1,1,1,17,18,1,1,1,11,6,10,10,9,14,1,1,1,1,17,18 },
	{ 1,1,1,1,2,10,10,3,1,12,13,1,1,1,1,1,1,1,1,2,10,10,3,1,12,13,1,1,1,1 },
	{ 1,18,1,1,11,4,4,14,1,11,16,5,5,5,13,1,18,1,1,11,4,4,14,1,11,16,5,5,5,13 },
	{ 1,18,18,1,1,1,1,1,1,1,2,10,10,10,3,1,18,18,1,1,1,1,1,1,1,2,10,10,10,3 },
	{ 1,17,1,18,18,1,1,1,1,18,11,4,6,10,3,1,17,1,18,18,1,1,1,1,18,11,4,6,10,3 },
	{ 1,1,1,17,1,1,1,17,1,1,18,18,11,4,14,1,1,1,17,1,1,1,17,1,1,18,18,11,4,14 },
	{ 1,1,1,1,17,1,1,1,1,1,1,1,1,1,1,1,1,1,1,17,1,1,1,1,1,1,1,1,1,1 },
	{ 1,1,1,1,1,18,1,1,1,1,1,1,1,1,1,1,1,1,1,1,18,1,1,1,1,1,1,1,1,1 },
	{ 1,1,1,1,18,1,18,18,1,1,17,1,18,1,1,1,1,1,1,18,1,18,18,1,1,17,1,18,1,1 },
	{ 1,1,17,18,18,1,1,1,18,1,1,1,17,18,1,1,1,17,18,18,1,1,1,18,1,1,1,17,18,1 },
	{ 1,1,12,5,5,5,5,5,5,13,1,18,18,1,1,1,1,12,5,5,5,5,5,5,13,1,18,18,1,1 },
	{ 1,1,2,10,10,10,10,10,10,3,1,1,1,1,1,1,1,2,10,10,10,10,10,10,3,1,1,1,1,1 },
	{ 1,1,11,6,10,10,10,10,10,3,1,1,1,1,1,1,1,11,6,10,10,10,10,10,3,1,1,1,1,1 },
	{ 1,1,1,2,10,10,10,10,9,14,1,17,1,1,1,1,1,1,2,10,10,10,10,9,14,1,17,1,1,1 },
	{ 1,1,1,2,10,10,10,10,3,1,1,1,1,18,1,1,1,1,2,10,10,10,10,3,1,1,1,1,18,1 },
	{ 1,1,1,11,6,10,10,9,14,1,1,1,1,17,18,1,1,1,11,6,10,10,9,14,1,1,1,1,17,18 },
	{ 1,1,1,1,2,10,10,3,1,12,13,1,1,1,1,1,1,1,1,2,10,10,3,1,12,13,1,1,1,1 },
	{ 1,18,1,1,11,4,4,14,1,11,16,5,5,5,13,1,18,1,1,11,4,4,14,1,11,16,5,5,5,13 },
	{ 1,18,18,1,1,1,1,1,1,1,2,10,10,10,3,1,18,18,1,1,1,1,1,1,1,2,10,10,10,3 },
	{ 1,17,1,18,18,1,1,1,1,18,11,4,6,10,3,1,17,1,18,18,1,1,1,1,18,11,4,6,10,3 },
	{ 1,1,1,17,1,1,1,17,1,1,18,18,11,4,14,1,1,1,17,1,1,1,17,1,1,18,18,11,4,14 },
	{ 1,1,1,1,17,1,1,1,1,1,1,1,1,1,1,1,1,1,1,17,1,1,1,1,1,1,1,1,1,1 }
};

void create_map(int w, int h)
{
	for (int y=0; y<h; y++) {
		for (int x=0; x<w; x++) {
			map[y][x] = terr_lakes[y%30][x%30];
		}
	}
}

// draw full screen at World position in xpos/ypos
void draw_screen()
{
	int tx=getTileX(xpos);
	int ty=getTileY(ypos);

	for (int i=0; i < (1+SCREENH/TILESIZE); i++) 
	{
		draw_horizontal(tx, ty+i, 1+SCREENW_TILES);
	}
}

void draw_horizontal(int tx, int ty, int len)
{
	int px=getTilePosInScreenX(tx);
	int py=getTilePosInScreenY(ty);

	for (int i=0; i<len; i++)
	{
		vdp_select_bitmap( map[ty][tx+i] );
		vdp_draw_bitmap( px + i*TILESIZE, py );
	}
	
}
void draw_vertical(int tx, int ty, int len)
{
	int px = getTilePosInScreenX(tx);
	int py = getTilePosInScreenY(ty);

	for (int i=0; i<len; i++)
	{
		vdp_select_bitmap( map[ty+i][tx] );
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
				draw_vertical(tx,ty, 1+SCREENH_TILES);
			}
			break;
		case LEFT:
			if ((xpos + SCREENW + step) < (MAPW*TILESIZE))
			{
				xpos += step;
				vdp_scroll_screen(dir, step);
				// draw tiles (tx,ty) to (tx,ty+len)
				int tx=getTileX(xpos + SCREENW -1);
				int ty=getTileY(ypos);
				draw_vertical(tx,ty, 1+SCREENH_TILES);
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
				draw_horizontal(tx,ty, 1+SCREENW_TILES);
			}
			break;
		case DOWN:
			if ((ypos + SCREENH + step) < (MAPH*TILESIZE))
			{
				ypos += step;
				vdp_scroll_screen(dir, step);
				// draw tiles (tx,ty) to (tx+len,ty)
				int tx=getTileX(xpos);
				int ty=getTileY(ypos + SCREENH -1);
				draw_horizontal(tx,ty, 1+SCREENW_TILES);
			}
			break;
		default:
			break;
	}
}
