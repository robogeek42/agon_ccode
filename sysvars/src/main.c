#include "agon/vdp_vdu.h"
#include "agon/vdp_key.h"
#include <mos_api.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "util.h"

#define COL(C) vdp_set_text_colour(C)
#define TAB(X,Y) vdp_cursor_tab(X,Y)

typedef struct { uint8_t A; uint8_t B; uint8_t CMD; uint16_t x; uint16_t y; } VDU_A_B_CMD_x_y;

static VDU_A_B_CMD_x_y vdu_read_pixel_colour = { 23, 0, 0x84, 0, 0 };
void wait() { char k=getchar(); if (k=='q') exit(0); }

static volatile SYSVAR *sys_vars = NULL;

int main(/*int argc, char *argv[]*/)
{
	//vdp_vdu_init();
	sys_vars = (SYSVAR *)mos_sysvars();

	if ( vdp_key_init() == -1 ) return 1;
	vdp_set_key_event_handler( key_event_handler );

	vdp_mode(0);
	vdp_logical_scr_dims(false);

	COL(1); TAB(0,1); printf("SYSVARS TEST");
	COL(3); TAB(0,2); printf("============");

	putch(23);putch(0);putch(0x86);

	uint16_t scrwidth = getsysvar_scrwidth();
	uint16_t scrheight = getsysvar_scrheight();

	COL(15); TAB(0,4);
	int delay = 255;
	while (delay--);
	printf("Width %d Height %d\n", scrwidth, scrheight);

	// draw 15 boxes

	for (int c=1; c<16; c++)
	{
		vdp_gcol(0,c);
		vdp_move_to( 16*c, 64);
		vdp_filled_rect( 16*(c+1)-1, 64+15 );
	}

	TAB(0,12);
	for (int i = 0; i< 16; i++)
	{
		uint16_t x, y;
		x = i*16+8;
		y = 74;

		//vdp_point( x, y-12 );

		sys_vars->vpd_pflags = 0;

		vdu_read_pixel_colour.x = x;
		vdu_read_pixel_colour.y = y;
		VDP_PUTS( vdu_read_pixel_colour );

		while ( !(sys_vars->vpd_pflags & vdp_pflag_point) ) {
			vdp_update_key_state();
		};

		uint24_t col = getsysvar_scrpixel();
		TAB(0,12+i);printf("%d,%d:",x,y);
	        TAB(10,12+i);printf("0x%06x\n", col);
		vdp_gcol(0,15);
		//wait();
	}

	COL(15);
	vdp_logical_scr_dims(true);
	return 0;
}

