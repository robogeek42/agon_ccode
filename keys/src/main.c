#include "agon/vdp_vdu.h"
#include "agon/vdp_key.h"
#include <mos_api.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define CHUNK_SIZE 1024

void wait() { char k=getchar(); if (k=='q') exit(0); }

void key_event_handler( KEY_EVENT key_event );
void game_loop();
void wait_clock( clock_t ticks );


int main(/*int argc, char *argv[]*/)
{
	vdp_vdu_init();
	if ( vdp_key_init() == -1 ) return 1;
	vdp_set_key_event_handler( key_event_handler );

	vdp_mode(1);
	vdp_logical_scr_dims(false);
	game_loop();

	return 0;
}

void game_loop()
{
	int exit=0;
	do {
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
	printf("Key 0x%X Mod 0x%X Asc 0x%X Down 0x%X\n",key_event.code, key_event.mods, key_event.ascii, key_event.down);
}

void wait_clock( clock_t ticks )
{
	clock_t ticks_now = clock();

	do {
		vdp_update_key_state();
	} while ( clock() - ticks_now < ticks );
}

