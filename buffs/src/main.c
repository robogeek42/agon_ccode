/*
  vim:ts=4
  vim:sw=4
*/
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

#define W(A) A%256,A/256
int main( int argc, char *argv[] )
{
	if ( vdp_key_init() == -1 ) return 1;
	vdp_set_key_event_handler( key_event_handler );

	//int m = input_int(0, 0, "Choose Mode : ");
	int mode = 0;
	if ( argc > 1 )
	{
		mode = atoi(argv[1]);
	}
	vdp_mode(mode);
	//vdp_logical_scr_dims(false);

	COL(1); TAB(0,1); printf("BUFFERS TEST");
	COL(3); TAB(0,2); printf("============");
	COL(2); TAB(0,3); printf("Mode %d",mode);
	COL(15); TAB(0,5); 

	// buffer IDs less than 256 for convenience
	int bID_draw = 1001;
	int bID_cnt1 = 1002;
	int bID_cmd  = 1003;
	int bID_cnt2 = 1004;
	int bID_X    = 1005;
	int bID_Y    = 1006;

	int startx = 10; int starty = 80;
	int cols   = 16; int rows   = 4;
	int xinc   = 80; int yinc   = 80;
	int rectw  = 60; int recth  = 60;

	// drawing sequence
	char data_draw[] = { 
		//        X     Y
		25, 0x04, W(0), W(0),   // move X,Y
	    18, 0, 0,               // gcol 0, COL
		25, 0x61, W(rectw), W(recth)  // rectangle relative 20x20
	};
	// offsets where variables sit in the draw buffer
#define XOFF 2
#define YOFF 4
#define COFF 8

	// commands
	char data_command[] = {
		23,0,0xA0, W(bID_draw), 5, 0xE2, W(XOFF), W(2), W(bID_X),W(0), // Set draw X from var-X
		23,0,0xA0, W(bID_draw), 5, 0xE2, W(YOFF), W(2), W(bID_Y),W(0), // Set draw Y from var-Y
		23,0,0xA0, W(bID_draw), 1,                                  // call draw buffer
		
		23,0,0xA0, W(bID_X),    5, 0xC4, W(0), W(2), W(xinc),       // 16-bit add 30 to X
		23,0,0xA0, W(bID_cnt1), 5,    3, 0, 0, -1,                  // decrement counter
		23,0,0xA0, W(bID_draw), 5,    3, COFF, 0, 1,                // inc Colour
		23,0,0xA0, W(bID_cmd),  6, 0, W(bID_cnt1), W(0),            // cond-call cmd if cnt1!=0
														            // 
		23,0,0xA0, W(bID_Y),    5, 0xC4, W(0), W(2), W(yinc),       // 16-bit add 30 to Y
		23,0,0xA0, W(bID_X),    5, 0xC2, W(0), W(2), W(startx),     // Reset X back to left 
		23,0,0xA0, W(bID_cnt2), 5,    3, W(0), -1,                  // decrement 2nd counter
		23,0,0xA0, W(bID_cnt1), 5,    2, W(0), cols,                // Reset cnt1 to 16
//		23,0,0xA0, W(bID_draw), 5, 2, 9, 0, 1,                      // Reset colour to 1
		23,0,0xA0, W(bID_cmd),  6, 0, W(bID_cnt2), W(0)             // cond-call cmd if cnt2!=0
		
	};

	// clear buffers
	vdp_adv_clear_buffer(bID_draw);
	vdp_adv_write_block_data( bID_draw, sizeof(data_draw), data_draw );
	vdp_adv_clear_buffer(bID_cmd);
	vdp_adv_write_block_data( bID_cmd, sizeof(data_command), data_command );

	// create a buffers for counters and initialise
	vdp_adv_create( bID_cnt1, 1 );
	vdp_adv_adjust( bID_cnt1, 2, 0 ); putch(cols); // write num cols to counter1
	vdp_adv_create( bID_cnt2, 1 );
	vdp_adv_adjust( bID_cnt2, 2, 0 ); putch(rows); // write num rows to counter2
	// create buffers for X,Y values
	// 3 bytes each. 2 for a 16-bit value, 3rd is because of overwrite using add-with-carry
	vdp_adv_create( bID_X, 3 );
	vdp_adv_adjust( bID_X, 0xC2, 0 ); putch(3);putch(0);putch(startx);putch(0);putch(0); // write to X
	vdp_adv_create( bID_Y, 3 );
	vdp_adv_adjust( bID_Y, 0xC2, 0 ); putch(3);putch(0);putch(starty);putch(0);putch(0); // write to Y

	// call the command buffer
	vdp_adv_call_buffer( bID_cmd );

	COL(15);
	vdp_logical_scr_dims(true);
	return 0;
}

