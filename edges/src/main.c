#include "agon/vdp_vdu.h"
#include "agon/vdp_key.h"
#include <mos_api.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define CHUNK_SIZE 1024

int gMode = 8;
int gScreenWidth = 320;
int gScreenHeight = 240;

int xpos=0, ypos=0;
int xoff_prev=0, yoff_prev=0;

void wait() { char k=getchar(); if (k=='q') exit(0); }

void key_event_handler( KEY_EVENT key_event );
void game_loop();
void wait_clock( clock_t ticks );
void draw_screen();
int load_bitmap_file( const char *fname, int width, int height, int bmap_id );


int main(/*int argc, char *argv[]*/)
{
	vdp_vdu_init();
	if ( vdp_key_init() == -1 ) return 1;
	vdp_set_key_event_handler( key_event_handler );

	vdp_mode(gMode);
	vdp_logical_scr_dims(false);

	printf("Load test image ");
	//load just one bitmap for this test 16x16
	load_bitmap_file("img/facetest.rgb2", 16, 16, 0);
	printf(" done.\n");

	printf("\nPress a key to draw test\n");
	printf("(Press ESC to exit)");
	getchar();

	vdp_clear_screen();

	draw_screen();
	game_loop();

	return 0;
}

void game_loop()
{
	int exit=0;
	int dir=0;
	do {
		int change=0;
		if ( vdp_check_key_press( 0x9c ) ) {dir=0; xpos += 1;change=1;}	// right
		if ( vdp_check_key_press( 0x9a ) ) {dir=1; xpos -= 1;change=1;}	// left
		if ( vdp_check_key_press( 0x96 ) ) {dir=2; ypos -= 1;change=1;}	// up
		if ( vdp_check_key_press( 0x98 ) ) {dir=3; ypos += 1;change=1;}	// down
		if (change==1) {
			//printf("Pressed DIR = %d\n",dir);
			vdp_scroll_screen(dir, 1);
		}
		wait_clock( 4 );
		//vdp_update_key_state();
	} while (exit==0);

}

static KEY_EVENT prev_key_event = { 0 };
void key_event_handler( KEY_EVENT key_event )
{
	if ( key_event.code == 0x7d ) {
		vdp_cursor_enable( true );
		exit( 1 ); // Exit program if esc pressed
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

void draw_screen()
{
	vdp_select_bitmap(0);

	// Top Line
	for (int i=0; i<16; i++)
	{
		vdp_draw_bitmap(24+i*16, -16 + i);
	}
	// Bottom Line
	for (int i=0; i<16; i++)
	{
		vdp_draw_bitmap(24+i*16, gScreenHeight -16 + i);
	}
	// Left Line
	for (int i=0; i<16; i++)
	{
		vdp_draw_bitmap(-16 + i, i*16);
	}
	// Right Line
	for (int i=0; i<16; i++)
	{
		vdp_draw_bitmap(gScreenWidth -16 + i, i*16);
	}
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
		printf( "Error opening file \"%sa\". Quitting.\n", fname );
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
		printf(".");

		bytes_remain -= size;
	}
	vdp_adv_consolidate(0xFA00+bmap_id);

	vdp_adv_select_bitmap(0xFA00+bmap_id);
	vdp_adv_bitmap_from_buffer(width, height, 1); // RGBA2
	
	fclose( fp );
	free( buffer );

	return bmap_id;
}

