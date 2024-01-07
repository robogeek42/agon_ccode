#include "globals.h"
#include "colmap.h"

#define FASTNOISE

#ifdef WIKINOISE
// Perlin noise algo lifted from wikipedia
#include "wikinoise.h"
#endif

#ifdef FASTNOISE
// FastNoiseLite - cut oesn to just OpenSimplex2D and Perlin
#include "fastnoise.h"
//#include "FastNoiseLite.h"
#endif

#include "agon/vdp_vdu.h"
#include "agon/vdp_key.h"
#include <mos_api.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int gMode = 8;
int gScreenWidth = 320;
int gScreenHeight = 240;

#ifdef FASTNOISE
fnl_state noise;
#endif

FILE *open_file( const char *fname, const char *mode);
int close_file( FILE *fp );
int read_str(FILE *fp, char *str, char stop);
void show_screen(float mag, int step);
void define_colours();
void key_event_handler( KEY_EVENT key_event );

void wait()
{
	char k=getchar();
	if (k=='q') exit(0);
}

int main(int argc, char *argv[])
{
	int seed=0;
	float mag=16.0;
	int step=4;
	int octaves=0;

	if (argc>1) {
		seed=atoi(argv[1]);
	}
	srand(seed);

	if (argc>2) {
		mag=atof(argv[2]);
	}

	if (argc>3) {
		step=atoi(argv[3]);
	}

	if (argc>4) {
		octaves=atoi(argv[4]);
	}

	vdp_vdu_init();
	if ( vdp_key_init() == -1 ) return 1;
	vdp_set_key_event_handler( key_event_handler );

	vdp_mode(gMode);

	vdp_logical_scr_dims(false);

#ifdef FASTNOISE
	noise = fnlCreateState();
	//noise.noise_type = FNL_NOISE_OPENSIMPLEX2;
	noise.noise_type = FNL_NOISE_PERLIN;
	noise.seed = seed;
	if (octaves>0) {
		noise.fractal_type = FNL_FRACTAL_FBM;
		noise.octaves = octaves;
	}
#endif

	show_screen(mag, step);

	// wait for a key press
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
	
void show_screen(float mag, int step) 
{
	vdp_clear_screen();

	for (int iy=0; iy < gScreenHeight; iy+=step)
	{
		for (int ix=0; ix < gScreenWidth; ix+=step)
		{
			float fx=((float)ix)/mag;
			float fy=((float)iy)/mag;
			//printf("x=%d y=%d fx=%f fy=%f\n",ix,iy,fx,fy);
			
#ifdef WIKINOISE
			float val = (perlin(fx, fy) * 0.5) + 0.5;
#endif
#ifdef FASTNOISE
			float val = (fnlGetNoise2D(&noise, fx, fy) * 0.5) + 0.5;
#endif
			uint8_t rgb2 = (uint8_t)floor(64 * val);

			uint8_t col = rgb2_to_bbc(rgb2);
			//vdp_set_text_colour(col);
			//printf("%02X ",col);

			vdp_gcol(0, col);
			//vdp_gcol(0, rgb2);

			if (step==1) {
				vdp_point(ix, iy);
			} else {
				vdp_move_to(ix,iy); vdp_filled_rect(ix+step,iy+step);
			}
		}
		vdp_update_key_state();
	}
}

void key_event_handler( KEY_EVENT key_event )
{
	if ( key_event.code == 0x7d ) {
		vdp_cursor_enable( true );
		exit( 1 );						// Exit program if esc pressed
	}
}

