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
void load_sprites();
void create_map(int w, int h);
void draw_screen();
void select_sprite( int sprite );

int getWorldCoordX(int sx) { return (xpos + sx); }
int getWorldCoordY(int sy) { return (ypos + sy); }
int getTileX(int sx) { return (sx/TILESIZE); }
int getTileY(int sy) { return (sy/TILESIZE); }
int getTilePosInScreenX(int tx) { return ((tx * TILESIZE) - xpos); }
int getTilePosInScreenY(int ty) { return ((ty * TILESIZE) - ypos); }

void scroll_screen(int dir, int step);
void draw_horizontal(int tx, int ty, int len);
void draw_vertical(int tx, int ty, int len);

static int bob_sprite_down = 0;
static int bob_sprite_up = 1;
static int bob_sprite_left = 2;
static int bob_sprite_right = 3;
static int bob_down_index = 40;
static int bob_up_index = 44;
static int bob_left_index = 48;
static int bob_right_index = 52;

int current_sprite = 0;

static int bobX = 0, bobY = 0;

static clock_t anim_time;
static clock_t anim_length = 10;

int main(/* int argc, char *argv[] */)
{
	vdp_vdu_init();
	if ( vdp_key_init() == -1 ) return 1;
	vdp_set_key_event_handler( key_event_handler );

	vdp_mode(MODE);
	vdp_logical_scr_dims(false);

	load_images();
	load_sprites();
	//getchar();
	create_map(MAPW, MAPH);
	
	/* start screen centred */
	xpos = TILESIZE*(MAPW - SCREENW/TILESIZE)/2; 
	ypos = TILESIZE*(MAPH - SCREENH/TILESIZE)/2; 
	bobX = xpos + TILESIZE*(SCREENW/TILESIZE)/2;
	bobY = ypos + TILESIZE*(SCREENH/TILESIZE)/2;

	vdp_select_sprite(current_sprite);
	vdp_move_sprite_to(bobX-xpos, bobY-ypos);
	vdp_show_sprite();

	draw_screen();

	game_loop();

	return 0;
}

void select_sprite( int sprite )
{
	current_sprite = sprite;
	for (int s=0; s<4; s++)
	{
		vdp_select_sprite(s);
		if (s == current_sprite)
		{
			vdp_show_sprite();
		} 
		else 
		{
			vdp_hide_sprite();
		}
	}
	vdp_select_sprite(current_sprite);
	if ((clock() - anim_time) > anim_length) {
		vdp_next_sprite_frame();
		anim_time = clock();
	}
}

void game_loop()
{
	int speed=1;
	anim_time = clock();
	do {
		int dir=-1;
		if ( vdp_check_key_press( 0x9a ) ) {
			dir=RIGHT;
			select_sprite(bob_sprite_left);
			bobX -= speed;
		}
		if ( vdp_check_key_press( 0x9c ) ) {
			dir=LEFT;
			select_sprite(bob_sprite_right);
			bobX += speed;
		}
		if ( vdp_check_key_press( 0x96 ) ) {
			dir=UP;
			select_sprite(bob_sprite_up);
			bobY -= speed;
		}
		if ( vdp_check_key_press( 0x98 ) ) {
			dir=DOWN;
			select_sprite(bob_sprite_down);
			bobY += speed;
		}
		
		vdp_move_sprite_to(bobX-xpos, bobY-ypos);
		vdp_refresh_sprites();

		if (dir>=0) {
			scroll_screen(dir, speed);
		}
		

		wait_clock( 1 );
		//vdp_update_key_state();

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
	printf("Load tiles.\n");
	for (int fn=1; fn<=18; fn++)
	{
		sprintf(fname, "img/ts%02d.rgb2",fn);
		//printf("load %s %d\n",fname, fn);
		load_bitmap_file(fname, TILESIZE,TILESIZE, fn);
	}
	printf("done.\n");
}
void load_sprites() 
{
	char fname[20];
	printf("Load Bob.\n");
	//vdp_reset_sprites();
	for (int fn=1; fn<=16; fn++)
	{
		sprintf(fname, "img/bob/bob%02d.rgb2",fn);
		//printf("load %s %d\n",fname, bob_down_index + fn - 1);
		load_bitmap_file(fname, TILESIZE,TILESIZE, bob_down_index + fn - 1);
	}
	vdp_create_sprite( bob_sprite_down, bob_down_index, 4 );
	vdp_create_sprite( bob_sprite_up, bob_up_index, 4 );
	vdp_create_sprite( bob_sprite_left, bob_left_index, 4 );
	vdp_create_sprite( bob_sprite_right, bob_right_index, 4 );
	vdp_activate_sprites( 4 );
	for (int s=0; s<4; s++)
	{
		vdp_select_sprite( s );
		vdp_hide_sprite();
	}
	printf("done.\n");
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
