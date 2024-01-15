#include "globals.h"
#include "colmap.h"

// FastNoiseLite - cut oesn to just OpenSimplex2D and Perlin
#include "fastnoise.h"

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

fnl_state noise;

uint8_t* world;

FILE *open_file( const char *fname, const char *mode);
int close_file( FILE *fp );
int read_str(FILE *fp, char *str, char stop);
int load_bitmap_file( const char *fname, int width, int height, int bmap_id );

void key_event_handler( KEY_EVENT key_event );
void wait_clock( clock_t ticks );


void load_images();
void create_world_map(float mag);
uint8_t get_terrain_colour_bbc(uint8_t terrain);
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

typedef struct {
	int seed;
	int mag_factor;
	float mag;
	int octaves;
	int width;
	int height;
	char *filename;
	uint24_t world_addr;
	uint24_t world_arrsize;
} GenParams;

bool gen_menu(GenParams* gp);
bool load_world_map(GenParams *gp);
bool save_world_map(GenParams *gp);

void wait()
{
	char k=getchar();
	if (k=='q') exit(0);
}

int main(int argc, char *argv[])
{
	int seed=0;
	int mag_factor = 2;
	float mag = (float) mag_factor / 10.0;
	int octaves=0;

	if (argc>1) {
		seed=atoi(argv[1]);
	}
	srand(seed);

	if (argc>2) {
		int mag_factor = atoi(argv[2]);
		mag = (float) mag_factor / 10.0;
	}

	if (argc>3) {
		octaves=atoi(argv[3]);
	}

	vdp_vdu_init();
	if ( vdp_key_init() == -1 ) return 1;
	vdp_set_key_event_handler( key_event_handler );

	GenParams genParams;
	genParams.seed = seed;
	genParams.mag = mag;
	genParams.mag_factor = mag_factor;
	genParams.width = gMapWidth;
	genParams.height = gMapHeight;
	genParams.octaves = octaves;
	genParams.filename = NULL;

	genParams.world_arrsize = gMapWidth*gMapHeight ;

	gen_menu(&genParams);

	// allocate map array
	world = (uint8_t *) malloc(sizeof(uint8_t) * gMapWidth * gMapHeight);
	if (world == NULL)
	{
		printf("Out of memory\n");
		return -1;
	}

	genParams.world_addr = (uint24_t) world;

	if (genParams.filename == NULL)
	{
		// Setup FastNoiseLite state 
		noise = fnlCreateState();
		//noise.noise_type = FNL_NOISE_OPENSIMPLEX2;
		noise.noise_type = FNL_NOISE_PERLIN;
		noise.seed = seed;
		if (octaves>0) {
			noise.fractal_type = FNL_FRACTAL_FBM;
			noise.octaves = octaves;
		}
		create_world_map(mag);
		save_world_map(&genParams);

	} else {
		if ( !load_world_map(&genParams) )
		{
			printf("Error loading map.\n");
			return -1;
		}
	}
	wait();

	vdp_mode(MODE);
	vdp_logical_scr_dims(false);

	show_map();
	// wait for a key press
	wait(); 

	load_images();

	game_loop();

	free(genParams.filename);
	free(world);
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
	for (int fn=1; fn<=4; fn++)
	{
		sprintf(fname, "img/t%02d.rgb2",fn);
		//printf("load %s\n",fname);
		load_bitmap_file(fname, TILESIZE,TILESIZE, fn-1);
	}
}

uint8_t  get_terrain_type(float val)
{
	if (val < -0.11) return 1; // water
	if (val <  0.00) return 2; // dirt
	if (val > 0.50) return 3; // mountains
	return 0; // grass
}

uint8_t get_terrain_colour_bbc(uint8_t terrain)
{
	switch (terrain) {
		default:
		case 0: return 32; // green (32) - grass
		case 1: return 12; // blue (12) - water
		case 2: return 42; // tan (42) - dirt
		case 3: return 47; // mauve (47) - mountains
	}
}

void create_world_map(float mag)
{
	printf("create world %dx%d tiles ", gMapWidth, gMapHeight);
	for (int iy=0; iy < gMapHeight; iy++ )
	{
		for (int ix=0; ix < gMapWidth; ix++ )
		{
			float fx=((float)ix)/mag;
			float fy=((float)iy)/mag;
			
			/* Get noise value between -1 and 1 */
			float val = fnlGetNoise2D(&noise, fx, fy);
			//val *= 1;
			uint8_t tt = get_terrain_type(val); 
			world[iy*gMapWidth + ix] = tt;
			//printf("%d,%d: val %f terr %d\n",ix,iy,val, tt);
		}
		vdp_update_key_state();
		if ((iy % 8) == 7) { printf("."); }
	}
	printf(" done.\n");
}

// show generated map (not tiled map) as colours
void show_map()
{
	int xoff = (SCREENW - gMapWidth)/2;
	int yoff = (SCREENH - gMapHeight)/2;
	{
		for (int iy=0; iy < gMapHeight; iy++ )
		{
			for (int ix=0; ix < gMapWidth; ix++ )
			{
				int tt = world[iy*gMapWidth + ix];
				vdp_gcol(0, get_terrain_colour_bbc(tt));
				vdp_point(xoff+ix, yoff+iy);
			}
			vdp_update_key_state();
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
		draw_horizontal(tx, ty+i, 1+(SCREENW/TILESIZE));
	}
}

void draw_horizontal(int tx, int ty, int len)
{
	int px=getTilePosInScreenX(tx);
	int py=getTilePosInScreenY(ty);

	for (int i=0; i<len; i++)
	{
		vdp_select_bitmap( world[ty*gMapWidth + tx+i] );
		vdp_draw_bitmap( px + i*TILESIZE, py );
	}
	
}
void draw_vertical(int tx, int ty, int len)
{
	int px = getTilePosInScreenX(tx);
	int py = getTilePosInScreenY(ty);

	for (int i=0; i<len; i++)
	{
		vdp_select_bitmap( world[(ty+i)*gMapWidth + tx] );
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

bool gen_menu(GenParams* gp)
{
	vdp_mode(3);
	COL(3);
	printf("Seed:\t%d\n",gp->seed);
	printf("Size:\t%dx%d\n",gp->width,gp->height);
	printf("Mag Factor:\t%d\t%f\n",gp->mag_factor,gp->mag);
	
	char fname[200];
	sprintf(fname,"maps/map_%dx%d_%d_%d_%d.data",gp->width,gp->height,gp->seed,gp->mag_factor,gp->octaves);
	printf("Check: %s\n",fname);
	uint8_t fh = mos_fopen(fname, FA_READ);
	if (fh == 0) 
	{
		// no file - not an error
		goto err_return;
	} else {
		FIL* fil = mos_getfil(fh);
		COL(2);
		printf("File found, size : %lu\n",fil->obj.objsize);
		
		if (fil->obj.objsize != gp->world_arrsize) 
		{
			COL(1);
			printf("Error - file size not correct\n");
			mos_fclose(fh);
			goto err_return;
		}
		
		gp->filename = malloc(strlen(fname)+1);
		strcpy(gp->filename, fname);

		mos_fclose(fh);
	}

	COL(15);
	return true;
err_return:
	COL(15);
	return false;
}

bool save_world_map(GenParams *gp)
{
	char fname[200];
	sprintf(fname,"maps/map_%dx%d_%d_%d_%d.data",gp->width,gp->height,gp->seed,gp->mag_factor,gp->octaves);
	uint8_t ret = mos_save( fname, gp->world_addr,  gp->world_arrsize );
	printf("Saved file %s.\nReturn code: %d\n",fname,ret);
	return true;
}

bool load_world_map(GenParams *gp)
{
	printf("Load file %s\n",gp->filename);
	uint8_t ret = mos_load( gp->filename, gp->world_addr, gp->world_arrsize );
	printf("Loaded file %s.\nReturn code: %d\n",gp->filename, ret);
	return true;
}
