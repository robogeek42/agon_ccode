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

bool db = true;
int gMode = 8; 
int gScreenWidth = 320;
int gScreenHeight = 240;

int gMapWidth = 64;
int gMapHeight = 64;
int gTileSize = 8;

int gTileSet=0;

#define SCROLL_RIGHT 0
#define SCROLL_LEFT 1
#define SCROLL_UP 2
#define SCROLL_DOWN 3

#define COL(C) vdp_set_text_colour(C)
#define TAB(X,Y) vdp_cursor_tab(X,Y)

#define KEY_LEFT 0x9A
#define KEY_RIGHT 0x9C
#define KEY_UP 0x96
#define KEY_DOWN 0x98
#define KEY_w 0x2C
#define KEY_a 0x16
#define KEY_s 0x28
#define KEY_d 0x19
#define KEY_W 0x46
#define KEY_A 0x30
#define KEY_S 0x42
#define KEY_D 0x33

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
int getTileX(int sx) { return (sx/gTileSize); }
int getTileY(int sy) { return (sy/gTileSize); }
int getTilePosInScreenX(int tx) { return ((tx * gTileSize) - xpos); }
int getTilePosInScreenY(int ty) { return ((ty * gTileSize) - ypos); }

void draw_screen();
void draw_UI(bool draw);
void scroll_screen(int dir, int step, bool updatepos);
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

int UIboxes=4;
int UIboxW=18;
int UIboxH=18;
int UIpos[4];
bool bShowUI=true;

int bobX=0;
int bobY=0;
bool bShowBob=true;

void wait()
{
	char k=getchar();
	if (k=='q') exit(0);
}

int main(/*int argc, char *argv[]*/)
{
	vdp_vdu_init();
	if ( vdp_key_init() == -1 ) return 1;
	vdp_set_key_event_handler( key_event_handler );

	// custom map which is 256x256 tiles
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

	UIpos[0]=(gScreenWidth-UIboxes*UIboxW)/2;
	UIpos[2]=gScreenWidth-UIpos[0];
	UIpos[1]=gScreenHeight-UIboxH;
	UIpos[3]=gScreenHeight-1;
	
	/* start screen centred */
	xpos = gTileSize*(gMapWidth - gScreenWidth/gTileSize)/2; 
	ypos = gTileSize*(gMapHeight - gScreenHeight/gTileSize)/2; 
	bobX = xpos + gTileSize*(gScreenWidth/gTileSize)/2;
	bobY = ypos + gTileSize*(gScreenHeight/gTileSize)/2;

	// setup complete
	vdp_mode(gMode + (db?128:0));
	vdp_logical_scr_dims(false);
	//vdu_set_graphics_viewport()

	load_images();

	game_loop();

my_exit:
	free(tilemap);
	vdp_mode(0);
	vdp_logical_scr_dims(true);
	vdp_cursor_enable( true );
	return 0;
}

void game_loop()
{
	int exit=0;
	draw_screen();
	if (db) 
	{
		vdp_swap();
		draw_screen();
	}
	do {
		int dir=-1;
		if ( vdp_check_key_press( KEY_LEFT ) ) {dir=SCROLL_RIGHT; }
		if ( vdp_check_key_press( KEY_RIGHT ) ) {dir=SCROLL_LEFT; }
		if ( vdp_check_key_press( KEY_UP ) ) {dir=SCROLL_UP; }
		if ( vdp_check_key_press( KEY_DOWN ) ) {dir=SCROLL_DOWN; }
		if ( vdp_check_key_press( KEY_w ) || vdp_check_key_press( KEY_W ) ) {dir=SCROLL_UP; }
		if ( vdp_check_key_press( KEY_a ) || vdp_check_key_press( KEY_A ) ) {dir=SCROLL_RIGHT; }
		if ( vdp_check_key_press( KEY_s ) || vdp_check_key_press( KEY_S ) ) {dir=SCROLL_DOWN; }
		if ( vdp_check_key_press( KEY_d ) || vdp_check_key_press( KEY_D ) ) {dir=SCROLL_LEFT; }
		if (dir>=0) {
			if (db) {
				draw_UI(false);
				scroll_screen(dir,1,false);
				draw_UI(true);
				//draw_screen();
				vdp_swap();
			}
			draw_UI(false);
			scroll_screen(dir,1,true);
			draw_UI(true);
		}
		if ( vdp_check_key_press( 0x26 ) || vdp_check_key_press( 0x2D ) ) exit=1; // q or x

		if ( vdp_check_key_press( 0x4F ) || vdp_check_key_press( 0x70 ) ) // - or _
		{
			if ( gTileSize == 16 )
			{
				gTileSize = 8;
				gTileSet = 0;
				xpos /= 2; xpos -= gScreenWidth /4;
				ypos /= 2; ypos -= gScreenHeight /4;
				if ( xpos+gScreenWidth > gMapWidth*gTileSize ) xpos=gMapWidth*gTileSize-gScreenWidth;
				if ( ypos+gScreenHeight > gMapHeight*gTileSize ) ypos=gMapHeight*gTileSize-gScreenHeight;
				if ( xpos < 0 ) xpos = 0;
				if ( ypos < 0 ) ypos = 0;
				draw_screen();
				if (db) {
					vdp_swap();
					draw_screen();
				}
			}
		}
		if ( vdp_check_key_press( 0x4E ) || vdp_check_key_press( 0x51 ) ) // = or +
		{
			if ( gTileSize == 8 )
			{
				gTileSize = 16;
				gTileSet = 64;
				xpos *= 2; xpos += gScreenWidth /2;
				ypos *= 2; ypos += gScreenHeight /2;
				if ( xpos+gScreenWidth > gMapWidth*gTileSize ) xpos=gMapWidth*gTileSize-gScreenWidth;
				if ( ypos+gScreenHeight > gMapHeight*gTileSize ) ypos=gMapHeight*gTileSize-gScreenHeight;
				if ( xpos < 0 ) xpos = 0;
				if ( ypos < 0 ) ypos = 0;
				draw_screen();
				if (db) {
					vdp_swap();
					draw_screen();
				}
			}
		}
		//TAB(4,4);printf("%d %d",xpos,ypos);
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
		vdp_mode(0);
		vdp_cursor_enable( true );
		vdp_logical_scr_dims(true);
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
		sprintf(fname, "img/t8/t%02d.rgb2",fn);
		//printf("load %s\n",fname);
		load_bitmap_file(fname, 8, 8, fn-1);
		sprintf(fname, "img/t16/tt%02d.rgb2",fn);
		load_bitmap_file(fname, 16, 16, 64+fn-1);
	}
	for (int fn=1; fn<=16; fn++)
	{
		sprintf(fname, "img/b8/bob%02d.rgb2",fn);
		//printf("load %s\n",fname);
		load_bitmap_file(fname, 8, 8, 128+fn-1);
		sprintf(fname, "img/b16/bob%02d.rgb2",fn);
		load_bitmap_file(fname, 16, 16, 128+16+fn-1);
	}
}

// draw full screen at World position in xpos/ypos
void draw_screen()
{
	int tx=getTileX(xpos);
	int ty=getTileY(ypos);

	for (int i=0; i < (1+gScreenHeight/gTileSize); i++) 
	{
		draw_horizontal(tx, ty+i, 1+(gScreenWidth/gTileSize));
	}

	draw_UI(true);
}

void draw_horizontal(int tx, int ty, int len)
{
	int px=getTilePosInScreenX(tx);
	int py=getTilePosInScreenY(ty);

	for (int i=0; i<len; i++)
	{
		vdp_select_bitmap( tilemap[ty*gMapWidth + tx+i] + gTileSet);
		vdp_draw_bitmap( px + i*gTileSize, py );
		vdp_update_key_state();
	}
	
}
void draw_vertical(int tx, int ty, int len)
{
	int px = getTilePosInScreenX(tx);
	int py = getTilePosInScreenY(ty);

	for (int i=0; i<len; i++)
	{
		vdp_select_bitmap( tilemap[(ty+i)*gMapWidth + tx] + gTileSet);
		vdp_draw_bitmap( px, py + i*gTileSize );
		vdp_update_key_state();
	}
}

/* 0=right, 1=left, 2=up, 3=down */
void scroll_screen(int dir, int step, bool updatepos)
{
	switch (dir) {
		case SCROLL_RIGHT: // scroll screen to right, view moves left
			if (xpos > step)
			{
				xpos -= step;
				vdp_scroll_screen(dir, step);
				// draw tiles (tx,ty) to (tx,ty+len)
				int tx=getTileX(xpos);
				int ty=getTileY(ypos);
				draw_vertical(tx,ty, 1+(gScreenHeight/gTileSize));
				if (!updatepos) xpos += step;
			}
			break;
		case SCROLL_LEFT: // scroll screen to left, view moves right
			if ((xpos + gScreenWidth + step) < (gMapWidth * gTileSize))
			{
				xpos += step;
				vdp_scroll_screen(dir, step);
				// draw tiles (tx,ty) to (tx,ty+len)
				int tx=getTileX(xpos + gScreenWidth -1);
				int ty=getTileY(ypos);
				draw_vertical(tx,ty, 1+(gScreenHeight/gTileSize));
				if (!updatepos) xpos -= step;
			}
			break;
		case SCROLL_UP:
			if (ypos > step)
			{
				ypos -= step;
				vdp_scroll_screen(dir, step);
				// draw tiles (tx,ty) to (tx+len,ty)
				int tx=getTileX(xpos);
				int ty=getTileY(ypos);
				draw_horizontal(tx,ty, 1+(gScreenWidth/gTileSize));
				if (!updatepos) ypos += step;
			}
			break;
		case SCROLL_DOWN:
			if ((ypos + gScreenHeight + step) < (gMapHeight * gTileSize))
			{
				ypos += step;
				vdp_scroll_screen(dir, step);
				// draw tiles (tx,ty) to (tx+len,ty)
				int tx=getTileX(xpos);
				int ty=getTileY(ypos + gScreenHeight -1);
				draw_horizontal(tx,ty, 1+(gScreenWidth/gTileSize));
				if (!updatepos) ypos -= step;
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

void draw_UI(bool draw) 
{
	if (!bShowUI) return;
	if (draw) {
		vdp_set_graphics_colour(0,1);
		vdp_move_to(UIpos[0],UIpos[1]);
		vdp_filled_rect(UIpos[2],UIpos[3]);
		vdp_set_graphics_colour(0,15);
		for (int box=0;box<=UIboxes;box++)
		{
			vdp_move_to(UIpos[0]+box*UIboxW,UIpos[1]);
			vdp_line_to(UIpos[0]+box*UIboxW,UIpos[3]);
		}
		vdp_move_to(UIpos[0],UIpos[1]);
		vdp_line_to(UIpos[2],UIpos[1]);
		vdp_move_to(UIpos[0],UIpos[3]);
		vdp_line_to(UIpos[2],UIpos[3]);
	} else {
		int tx1=getTileX(xpos+UIpos[0]);
		int ty1=getTileY(ypos+UIpos[1]);
		int tx2=getTileX(xpos+UIpos[2]);
		int ty2=getTileY(ypos+UIpos[3]);
		for (int ty=ty1; ty<=ty2; ty++) {
			draw_horizontal(tx1, ty, 1+ (tx2-tx1) );
		}
	}
}

void draw_bob(bool draw)
{
	if (!bShowBob) return;
	if (draw) {
	} else {
	}
}
