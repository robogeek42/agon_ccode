#include "agon/vdp_vdu.h"
#include "agon/vdp_key.h"
#include <mos_api.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "util.h"
#include "keydefines.h"


void wait() { char k=getchar(); if (k=='q') exit(0); }

void game_loop();

int main(/*int argc, char *argv[]*/)
{
	vdp_vdu_init();
	if ( vdp_key_init() == -1 ) return 1;
	vdp_set_key_event_handler( key_event_handler );

	vdp_mode(0);

	COL(1); TAB(0,1); printf("SOUND TEST");
	COL(2); TAB(0,2); printf("==========");

	COL(15); TAB(0,4); printf("Press P to play\n");
	game_loop();

	return 0;
}

void game_loop()
{
	int exit=0;
	clock_t key_wait_ticks = clock();
	do {
		if ( vdp_check_key_press( 0x26 ) || vdp_check_key_press( 0x2D ) ) exit=1; // q or x

		if ( vdp_check_key_press( KEY_p ) )
		{
			if ( key_wait_ticks < clock() )
			{
				key_wait_ticks = clock()+20;
				printf("play");
			}
		}
		vdp_update_key_state();
	} while (exit==0);

	COL(15);
}


