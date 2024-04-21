#include "agon/vdp_vdu.h"
#include "agon/vdp_key.h"
#include <mos_api.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../../common/util.h"

#define COL(C) vdp_set_text_colour(C)
#define TAB(X,Y) vdp_cursor_tab(X,Y)

void wait() { char k=getchar(); if (k=='q') exit(0); }

//static volatile SYSVAR *sys_vars = NULL;

int main(/*int argc, char *argv[]*/)
{
	//vdp_vdu_init();
	//sys_vars = (SYSVAR *)mos_sysvars();

	if ( vdp_key_init() == -1 ) return 1;
	vdp_set_key_event_handler( key_event_handler );

	//sys_vars->vpd_pflags = 0;

	vdp_mode(0);
	vdp_logical_scr_dims(false);

	COL(1); TAB(0,1); printf("SYSVARS TEST");
	COL(3); TAB(0,2); printf("============");

	COL(15); TAB(0,4); printf("Using latest AgDev code\n\n");

	//while ( !(sys_vars->vpd_pflags & vdp_pflag_mode ) );

	uint16_t scrwidth = getsysvar_scrwidth();
	uint16_t scrheight = getsysvar_scrheight();
	uint8_t colours = getsysvar_scrColours();

	int delay = 255;
	while (delay--);
	printf("Mode 0 Width %d Height %d, %d colours\n", scrwidth, scrheight, colours);

	// draw 15 coloured boxes

	for (int c=1; c<16; c++)
	{
		vdp_gcol(0,c);
		vdp_move_to( 16*c, 64);
		vdp_filled_rect( 16*(c+1)-1, 64+15 );
	}

	TAB(0,12);
	// read pixels from centre of each box
	COL(2);printf("ReadPixels");
	COL(15);
	for (int i = 0; i< 16; i++)
	{
		uint16_t x, y;
		x = i*16+8;
		y = 74;

		//vdp_point( x, y-12 );

		uint24_t col = vdp_return_pixel_colour( x, y );
		TAB(0,13+i);printf("%d,%d:",x,y);
	        TAB(10,13+i);printf("0x%06x\n", col);
		//vdp_gcol(0,15);
		//wait();
	}
	printf("\n");

	// redfine a palette entry
	printf("Redefine col 1 0000AA Red as FFAA55 Turquoise\n");
	COL(2);printf("Pallete\n");
	COL(15);
	vdp_define_colour( 1, 255, 0x55, 0xAA, 0xFF );

	// print out palette entries
	for (int i=0; i<16; i++) 
	{
		uint24_t pal_col = vdp_return_palette_entry_colour(i);
		uint8_t  pal_ind = vdp_return_palette_entry_index(i);
		uint24_t p1 = getsysvar_scrpixel();
		uint8_t p2 = getsysvar_scrpixelIndex();
		printf("Palette %02d : 0x%06x 0x%02x : 0x%06x 0x%02x \n",i, pal_col, pal_ind, p1,  p2);
	}
	printf("Current Text FG %x BG %x\n",
			vdp_return_palette_entry_colour(128),
			vdp_return_palette_entry_colour(129) );
	COL(15);
	vdp_logical_scr_dims(true);
	return 0;
}

