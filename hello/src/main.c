#include "agon/vdp_vdu.h"
#include "agon/vdp_key.h"
#include <mos_api.h>
#include <stdio.h>
#include <stdlib.h>

void wait() { char k=getchar(); if (k=='q') exit(0); }

void key_event_handler( KEY_EVENT key_event );

#define COL(C) vdp_set_text_colour(C)
#define TAB(X,Y) vdp_cursor_tab(X,Y)

int main(/*int argc, char *argv[]*/)
{
	vdp_vdu_init();
	if ( vdp_key_init() == -1 ) return 1;
	vdp_set_key_event_handler( key_event_handler );

	vdp_mode(0);
	//vdp_logical_scr_dims(false);

	COL(1); TAB(10,1); printf("Simple C test");
	COL(2); TAB(10,2); printf("=============");
	COL(3); TAB(2,4); printf("Hello World!");

	wait();

	return 0;
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

