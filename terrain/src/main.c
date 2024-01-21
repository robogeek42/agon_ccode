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
uint8_t* tilemap;
static bool bShowTile=true;

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

typedef struct {
	int seed;
	double mag;
	int octaves;
	int width;
	int height;
	char *filename;
	uint24_t world_addr;
	uint24_t world_arrsize;
	float threshold[3];
} GenParams;

typedef struct {
	char fname[20];
	uint8_t id;
	int nb[4];
	uint8_t key;
} TileInfoFile;

uint8_t tileLU[256] = {0};

void create_world_map(GenParams *gp, double mag);
uint8_t get_terrain_type(GenParams *gp, float val);
uint8_t get_terrain_colour_bbc(uint8_t terrain);
bool gen_menu(GenParams* gp);
bool load_world_map(GenParams *gp);
bool save_world_map(GenParams *gp);

void populate_tilemap(GenParams *gp);
void create_tileLU();

void save_map();

#define TILE_INFO_FILE "img/tileinfo.txt"
int readTileInfoFile(char *path, TileInfoFile *tif, int items);

void wait()
{
	char k=getchar();
	if (k=='q') exit(0);
}

int main(int argc, char *argv[])
{
	int seed=0;
	double mag = 0.2;
	int octaves=0;
	float threshold[3] = { -0.15, 0.00, 0.6 }; // defaults

	if (argc <= 1 || (argc > 1 && strcmp(argv[1],"-h")==0)) {
		printf("usage:\n %s: [-h] ", argv[0]);
		printf("[seed] [map width] [map height] [mag] [octaves] [thresholds ... ]\n");
		printf("defaults:\n");
		printf(" Seed: %d\n Map WxH (8x8 tiles): %dx%d\n Mag : %.2f\n Octaves: %d (off)\n Thresholds %.2f %.2f %.2f\n",
				seed, gMapWidth, gMapHeight, mag, octaves, threshold[0], threshold[1], threshold[2]);
		return 0;
	}
	if (argc>1) {
		seed=atoi(argv[1]);
	}
	srand(seed);

	if (argc>2) {
		gMapWidth = atoi(argv[2]);
	}

	if (argc>3) {
		gMapHeight = atoi(argv[3]);
	}

	if (argc>4) {
		mag = my_atof(argv[4]);
	}

	if (argc>5) {
		octaves=atoi(argv[5]);
	}

	if (argc>6) {
		threshold[0]=my_atof(argv[6]);
	}

	if (argc>7) {
		threshold[1]=my_atof(argv[7]);
	}

	if (argc>8) {
		threshold[2]=my_atof(argv[8]);
	}

	vdp_vdu_init();
	if ( vdp_key_init() == -1 ) return 1;
	vdp_set_key_event_handler( key_event_handler );

	GenParams genParams;
	genParams.seed = seed;
	genParams.mag = mag;
	genParams.width = gMapWidth;
	genParams.height = gMapHeight;
	genParams.octaves = octaves;
	genParams.filename = NULL;
	genParams.threshold[0] = threshold[0]; // water lowest
	genParams.threshold[1] = threshold[1]; // dirt threshold followed by grass
	genParams.threshold[2] = threshold[2]; // above this is mountains

	genParams.world_arrsize = gMapWidth*gMapHeight ;

	gen_menu(&genParams);
	printf("Total heap space for maps : 2 x %dk\n",genParams.world_arrsize/1024);

	// allocate world map array
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
		create_world_map(&genParams, mag);
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

	create_tileLU();

	printf("alloc tile map and populate\n");
	// allocate tile map
	tilemap = (uint8_t *) malloc(sizeof(uint8_t) * gMapWidth * gMapHeight);
	if (tilemap == NULL)
	{
		printf("Out of memory\n");
		return -1;
	}

	populate_tilemap(&genParams);
	printf("done\n");

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
		if ( vdp_check_key_press( 0x26 ) || vdp_check_key_press( 0x2D ) ) exit=1; // q or x
		if ( vdp_check_key_press( 0x2C ) ) bShowTile = false; // w = world
		if ( vdp_check_key_press( 0x29 ) ) bShowTile = true; // t = tiles

		if ( vdp_check_key_press( 0x28 ) || vdp_check_key_press( 0x42 ) ) { // sS save
			save_map();
			draw_screen();
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

uint8_t get_terrain_type(GenParams *gp, float val)
{
	if (val < gp->threshold[0]) return 1; // water
	if (val < gp->threshold[1]) return 2; // dirt
	if (val > gp->threshold[2]) return 3; // mountains
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

void create_world_map(GenParams *gp, double mag)
{
	printf("create world %dx%d tiles ", gMapWidth, gMapHeight);
	for (int iy=0; iy < gMapHeight; iy++ )
	{
		for (int ix=0; ix < gMapWidth; ix++ )
		{
			double fx=((double)ix)/mag;
			double fy=((double)iy)/mag;
			
			/* Get noise value between -1 and 1 */
			float val = fnlGetNoise2D(&noise, fx, fy);
			//val *= 1;
			uint8_t tt = get_terrain_type(gp, val); 
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
		if (bShowTile)
		{
			vdp_select_bitmap( tilemap[ty*gMapWidth + tx+i] );
		} else {
			vdp_select_bitmap( world[ty*gMapWidth + tx+i] );
		}
		vdp_draw_bitmap( px + i*TILESIZE, py );
	}
	
}
void draw_vertical(int tx, int ty, int len)
{
	int px = getTilePosInScreenX(tx);
	int py = getTilePosInScreenY(ty);

	for (int i=0; i<len; i++)
	{
		if (bShowTile)
		{
			vdp_select_bitmap( tilemap[(ty+i)*gMapWidth + tx] );
		} else {
			vdp_select_bitmap( world[(ty+i)*gMapWidth + tx] );
		}
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

void get_filename(GenParams *gp, char *str)
{
	sprintf(str,"maps/map_%dx%d_%d_%.4lf_%d_%.3f_%.3f_%.3f.data",
			gp->width,
			gp->height,
			gp->seed,
			gp->mag,
			gp->octaves,
			gp->threshold[0],
			gp->threshold[1],
			gp->threshold[2]);
}
bool gen_menu(GenParams* gp)
{
	vdp_mode(3);
	COL(3);
	printf("Seed:\t%d\n",gp->seed);
	printf("Size:\t%dx%d\n",gp->width,gp->height);
	printf("Mag:\t%lf\n",gp->mag);
	printf("Octaves:\t%d\n",gp->octaves);
	printf("Thresholds: %0.2f %0.2f %0.2f\n",gp->threshold[0],gp->threshold[1],gp->threshold[2]);
	
	char fname[200];
	get_filename(gp, fname);
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
	get_filename(gp, fname);
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

void populate_tilemap(GenParams *gp)
{
	printf("Populate tilemap\n");
	int ws = (int)gp->world_arrsize;

	for (int i=0; i < ws; i++)
	{
		int w = gp->width;

		// indexes of neighbours
		int nb[4] = {-1,-1,-1,-1};

		if (i >= w) {
			nb[0] = i - w; // above
		}
		if ((i % w) < (w - 1)) {
			nb[1] = i + 1; // right
		}
		if (i < (ws - w) ) {
			nb[2] = i + w; // below
		}
		if ((i % w) > 0) {
			nb[3] = i - 1; // left
		}

		// Edge pieces should remain the starting colour for ease
		if (nb[0]<0 || nb[1]<0 || nb[2]<0 || nb[3]<0) 
		{
			tilemap[i] = world[i];
			continue;
		}
		// if we placed a diagonal piece before or above, skip this next tile
		if (tilemap[nb[0]]>3 || tilemap[nb[3]]>3)
		{
			tilemap[i] = world[i];
			continue;
		}

		// get colour of neighbours
		uint8_t nbc[4] = {0,0,0,0};
		for (int n=0; n<4; n++)
		{
			nbc[n] = world[nb[n]];
		}
		
		// calculate tile key based on neighbours with North in LSB
		// key = 0bWWSSEENN
		uint8_t tile_key = 0;
		for (int n=0; n<4; n++) {
			tile_key |= (nbc[n] & 0x3) << 2*n;
		}
		
		// Tile Look-up 256 possibilities of tile neighbours
		// current tile set will obviously match only 24 of these,
		tilemap[i] = tileLU[tile_key];
	

		vdp_update_key_state();
		if (i%20==0) {
			//getch();
		}
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

void create_tileLU()
{
	int items = readTileInfoFile(TILE_INFO_FILE, NULL, 0);
	TileInfoFile *tif = malloc(sizeof(TileInfoFile) * items);
	readTileInfoFile(TILE_INFO_FILE, tif, items);
	printf("Read %d tile info items\n",items);
	
	// calculate a tile key for each tile. Each can be 4 colours
	// north tile bits are in LSB = 0bWWSSEENN
	for (int i=0; i<items; i++)
	{
		tif[i].key = 0;
		for (int n=0; n<4; n++) {
			tif[i].key |= (tif[i].nb[n] & 0x3) << 2*n;
		}
	}

	// Place the diagonals special tiles in the look-up table
	for (int i=0; i<items; i++)
	{
		tileLU[ tif[i].key ] = tif[i].id;
	}
	printf("Create Tile lookup table\n");
	for (unsigned int i=0; i<=255; i++)
	{
		int nb[4];
		nb[0] = ((uint8_t)i & 0x03);
		nb[1] = ((uint8_t)i & 0x0C)>>2; 
		nb[2] = ((uint8_t)i & 0x30)>>4; 
		nb[3] = ((uint8_t)i & 0xC0)>>6; 

		int cnt[4] = {0};
		// get a count of each colour type
		for (int n=0; n<4; n++)
		{
			cnt[nb[n]]++;
		}
		for (int n=0; n<4; n++)
		{
			if (cnt[n]>2) 
			{
				tileLU[i]=n;
				break;
			}
			if (cnt[n]==2 && cnt[n]>cnt[(n+1)%4] &&  cnt[n]>cnt[(n+2)%4] && cnt[n]>cnt[(n+3)%4])
			{
				tileLU[i]=n;
				break;
			}
		}

		/*
		printf("Tile %d : %1d %1d %1d %1d %d\n", i, 
				 nb[0],nb[1],nb[2],nb[3],
				tileLU[i]);
		if (i%22 == 21){wait();}
		*/
	}

}

void save_map()
{
	char mapname[100];
	vdp_cursor_tab(0,0);
	printf("Save TileMap\n\nName? ");
	scanf("%s", mapname);
	if (strlen(mapname)==0)
	{
		return;
	}
	printf("\nSaving tilemap %s :",mapname);

	uint8_t ret = mos_save( mapname, (uint24_t) tilemap,  gMapWidth * gMapHeight );
	printf("Done.  Return code %d.\n",ret);
	wait();

}
