#include "agon/vdp_vdu.h"
#include "agon/vdp_key.h"
#include <mos_api.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "keydefines.h"
#include "../../common/util.h"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

typedef struct {
	int curr_x;
	int curr_y;
	int intensity;
	int global_scale;
	uint16_t insptr; 		// instruction pointer
} DVG_STATE;

DVG_STATE state = {0,0,0,0,0};

// RAM for drawing instructions
// ROM starts at 0x0800
#define DVGRAM_SIZE 4096
uint8_t dvgram[DVGRAM_SIZE];

#define STACK_SIZE 16 // only 8 addresses
uint8_t stack[STACK_SIZE];
uint8_t sptr = 0;

bool bDecode = true;
bool bDraw = false;

void game_loop();
uint16_t dvg_next_command(uint16_t addr);
void inject_test(int test_id, uint16_t addr);
void run_tests();

bool bEnd = false;

int last_x = 0;
int last_y = 0;
int current_x = 0;
int current_y = 0;

void move_to(int x, int y)
{
	vdp_plot(4, x, y);
}

void draw_to(int x, int y)
{
	vdp_plot(5, x, y);
}

void draw_relative(int dx, int dy)
{
	vdp_plot(1, dx, dy);
}

void move_relative(int dx, int dy)
{
	vdp_plot(0, dx, dy);
}

int main(/* int argc, char *argv[] */)
{
	vdp_vdu_init();
	if ( vdp_key_init() == -1 ) return 1;
	vdp_set_key_event_handler( key_event_handler );

	vdp_mode(0);
	vdp_logical_scr_dims(true);

	vdp_gcol(0, 15);
	vdp_gcol(0, 128);
	vdp_clear_graphics();

	// load the asteroids ROM into 0x800
	uint8_t ret = mos_load( "asteroids_data_raw.bin", (uint24_t) &(dvgram[0x800]),  1822 );
	if ( ret != 0 )
	{
		printf("Error loading asteroids_data_raw.bin\n");
	}

	run_tests();

	return 0;
}

void game_loop()
{
	bool bExit = false;
	clock_t key_ticks = clock() + 10;
	int key_rate = 10;
	do {
		bExit=false;
		vdp_update_key_state();

		if ( key_ticks < clock() && vdp_check_key_press( KEY_LEFT ) )
		{
			key_ticks = clock() + key_rate;
		}
		if ( key_ticks < clock() && vdp_check_key_press( KEY_RIGHT ) )
		{
			key_ticks = clock() + key_rate;
		}
		if ( key_ticks < clock() && vdp_check_key_press( KEY_UP ) )
		{
			key_ticks = clock() + key_rate;
		}
		if ( key_ticks < clock() && vdp_check_key_press( KEY_DOWN ) )
		{
			key_ticks = clock() + key_rate;
		}

		if ( vdp_check_key_press(0x2d) ) // x
		{
			bExit = true;
		}

	} while (!bExit);

}


// scaling factor from https://computerarcheology.com/Arcade/Asteroids/DVG.html
// always a divisor
int get_divisor_from_scale(int scale)
{
	int mult = 1;
	switch (scale) {
		case 0: return 512;
		case 1: return 256;
		case 2: return 128;
		case 3: return 64;
		case 4: return 32;
		case 5: return 16;
		case 6: return 8;
		case 7: return 4;
		case 8: return 2;
		default:
		case 9: return 1;
	}
	return mult;
}

// This is the scaling given in https://www.philpem.me.uk/_media/elec/vecgen/vecgen.pdf
// but it seems different to the other one used above, adn doesn't work with e.g. test pattern
void decode_scale(int scale, int *pfactor, bool *pmult)
{
	int factor = 0; bool mult=true;
	switch (scale) {
		case 0b1111: factor=2; mult=false; break;
		case 0b1110: factor=4; mult=false; break;
		case 0b1101: factor=8; mult=false; break;
		case 0b1100: factor=16; mult=false; break;
		case 0b1011: factor=32; mult=false; break;
		case 0b1010: factor=64; mult=false; break;
		case 0b1001: factor=128; mult=false; break;
		case 0b1000: factor=256; mult=false; break;
		case 0b0000: factor=0; break;
		case 0b0001: factor=2; break;
		case 0b0010: factor=4; break;
		case 0b0011: factor=8; break;
		case 0b0100: factor=16; break;
		case 0b0101: factor=32; break;
		case 0b0110: factor=64; break;
		case 0b0111: factor=128; break;
	}
	(*pfactor) = factor;
	(*pmult) = mult;
}

void set_brightness(int bri)
{
	if (bri==0) {
		vdp_gcol(0,0);
	} else if (bri < 6) {
		vdp_gcol(0,8);
	} else if (bri < 11) {
		vdp_gcol(0,7);
	} else {
		vdp_gcol(0,15);
	}
	
}

void push_addr(uint16_t addr)
{
	stack[sptr++] = addr & 0xFF;
	stack[sptr++] = ( addr & 0xFF00 ) >> 8;
}
uint16_t pop_addr()
{
	uint16_t addr = 0;
	if (sptr>1)
	{
		addr = stack[--sptr] << 8;
		addr |= stack[--sptr];
	}
	return addr;
}

/* VEC
 * Draw a line from the current (x,y) coordinate.
 */
void do_cmd_VEC(uint8_t w1_lo, uint8_t w1_hi, uint8_t w2_lo, uint8_t w2_hi)
{
	int x = w2_lo + ((int)(w2_hi & 0x3) << 8); 
	int sgn_x = (w2_hi & 0x4)==0 ? 1 : -1;
	x *= sgn_x;

	int y = w1_lo + ((int)(w1_hi & 0x3) << 8); 
	int sgn_y = (w1_hi & 0x4)==0 ? 1 : -1;
	y *= sgn_y;

	int local_scale = (w1_hi & 0xF0) >> 4;
	int bri   = (w2_hi & 0xF0) >> 4;
	
	int div = get_divisor_from_scale(local_scale + state.global_scale);
	int xadj = x / div;
	int yadj = y / div;

#if 0
	int factor = 0; bool gmult = false;
	decode_scale(state.global_scale, &factor, &gmult);
	if (mult) {
		xadj *= factor;
		yadj *= factor;
	} else {
		xadj /= factor;
		yadj /= factor;
	}
#endif
	
	if (bDecode)
	{
		printf("VEC loc_scale=%d bri=%d X=%d Y=%d (%d, %d)\n", local_scale, bri, x, y, xadj, yadj);
	}
	if (bDraw)
	{
		set_brightness(bri);
		if (bri==0)
		{
			move_relative(xadj, yadj);
		} else {
			draw_relative(xadj,yadj);
		}
	}
}

/* CUR - also known as LABS (Load Absolute) in https://www.philpem.me.uk/_media/elec/vecgen/vecgen.pdf
 * Set the current (x,y) and global scale-factor.
 */
void do_cmd_CUR(uint8_t w1_lo, uint8_t w1_hi, uint8_t w2_lo, uint8_t w2_hi)
{
	int x = w2_lo + ((int)(w2_hi & 0x3) << 8); 

	int y = w1_lo + ((int)(w1_hi & 0x3) << 8); 

	int global_scale = (w2_hi & 0xF0) >> 4;

	state.global_scale = global_scale;

	if (bDecode)
	{
		printf("CUR global scale=%d X=%d Y=%d\n", global_scale, x, y);
	}
	if (bDraw)
	{
		move_to(x,y);
	}
}

/* HALT
 * End the current drawing list.
 */
void do_cmd_HALT()
{
	if (bDecode)
	{
		printf("HALT \n");
	}
}


/* JSR
 * Jump to a vector subroutine.
 * Note that there is room in the internal "stack" for only FOUR levels of nested subroutine calls. Be careful.
 * Note that the target address is the WORD address -- not the byte address.
 */
uint16_t do_cmd_JSR(uint8_t lo, uint8_t hi, uint16_t current_addr)
{
	uint16_t word_addr = lo | ((hi & 0x0F) << 8);
	uint16_t byte_addr = ((word_addr-0x800) << 1) + 0x800;
	
	if (bDecode)
	{
		printf("JSR word-addr : 0x%04X byte-addr : 0x%04X \n", word_addr, byte_addr);
	}

	push_addr(current_addr);

	return byte_addr;
}

/* RTS
 * Return from current vector subroutine.
 */
uint16_t do_cmd_RTS()
{
	uint16_t byte_addr = 0;

	if (bDecode)
	{
		printf("RTS \n");
	}
	if (sptr == 0)
	{
		bEnd = true;
	} else {
		byte_addr = pop_addr();
	}

	return byte_addr;
}

/* JMP
 * Jump to a new location in the vector program.
 * Note that the target address is the WORD address -- not the byte address.
 */
uint16_t do_cmd_JMP(uint8_t lo, uint8_t hi)
{
	uint16_t word_addr = lo | ((hi & 0x0F) << 8);
	uint16_t byte_addr = ((word_addr-0x800) << 1) + 0x800;
	
	if (bDecode)
	{
		printf("JMP word-addr : 0x%04X byte-addr : 0x%04X \n", word_addr, byte_addr);
	}

	return byte_addr;
}

/* SVEC
 * Use a "short" notation to draw a vector. 
 */
void do_cmd_SVEC(uint8_t lo, uint8_t hi)
{
	int x = lo & 0x03;
	int sgn_x = (lo & 0x4)==0 ? 1 : -1;
	x *= sgn_x;
	int y = hi & 0x03;
	int sgn_y = (hi & 0x4)==0 ? 1 : -1;
	y *= sgn_y;
	
	int Ss = ((lo & 0x8) >> 2) | ( ((hi & 0x8) >> 3) ); // Ss (0=*2, 1=*4, 2=*8, 3=*16);
	int mult = 1;
	switch (Ss) {
		case 0: mult=2; break;
		case 1: mult=4; break;
		case 2: mult=8; break;
		case 3: mult=16; break;
	};

	int bri = (lo & 0xF0) >> 4;

	int xadj = x * mult;
	int yadj = y * mult;

	//int div = get_divisor_from_scale(state.global_scale);
	//xadj /= div;
	//yadj /= div;

#if 0
	int factor = 0; bool gmult = false;
	decode_scale(state.global_scale, &factor, &gmult);

	if (gmult) {
		xadj *= factor;
		yadj *= factor;
	} else {
		xadj /= factor;
		yadj /= factor;
	}
#endif

	if (bDecode)
	{
		printf("SVEC scale=%02d(*%02d) bri=%02d x=%3d y=%3d (%.3f, %.3f)\n", Ss, mult, bri, x, y, (float)xadj, (float)yadj);
	}
	if (bDraw)
	{
		set_brightness(bri);
		if (bri==0) {
			move_relative(xadj, yadj);
		} else {
			draw_relative(xadj, yadj);
		}
	}
}

/* 
 * Read and execute the command at "addr"
 * Return the instruction pointer
 */
uint16_t dvg_next_command(uint16_t addr)
{
	uint16_t next_addr = addr + 2;
	uint16_t a = addr;

	// Little-endian words: lo byte followed by hi byte
	// always have at least one word (b2b1)
	uint8_t b1 = dvgram[addr++];
	uint8_t b2 = dvgram[addr++];
	// and possibly a second word depending on command
	uint8_t b3, b4;

	uint8_t command = (b2 & 0xF0) >> 4;  // upper nibble is command

	/*
		0 - 9 : VEC  -- a full vector command
		A : CUR  -- set the current (x,y) and global scale-factor
		B : HALT -- end of commands
		C : JSR  -- jump to a vector program subroutine
		D : RTS  -- return from a vector program subroutine
		E : JMP  -- jump to a location in the vector program
		F : SVEC -- a short vector command
	*/
	// adjust so case statement operates on 7 types of command: 0-6
	if (command < 0xA) {
		command = 0; 
	} else {
		command = command - 9;
	}

	if (bDecode) {
		printf("0x%04X %02X %02X ", a, b1, b2);
	}
	switch (command) {
		case 0: b3 = dvgram[addr++];
				b4 = dvgram[addr++];
				if (bDecode) {
					printf("%02X %02X ", b3, b4);
				}
				next_addr += 2;
				do_cmd_VEC(b1, b2, b3, b4);
				break;
		case 1: b3 = dvgram[addr++];
				b4 = dvgram[addr++];
				if (bDecode) {
					printf("%02X %02X ", b3, b4);
				}
				next_addr += 2;
				do_cmd_CUR(b1, b2, b3, b4);
				break;
		case 2: 
				if (bDecode) {
					printf("      ");
				}
				do_cmd_HALT();
				break;
		case 3: 
				if (bDecode) {
					printf("      ");
				}
				next_addr = do_cmd_JSR(b1, b2, addr);
				break;
		case 4:
				if (bDecode) {
					printf("      ");
				}
				next_addr = do_cmd_RTS();
				break;
		case 5:
				if (bDecode) {
					printf("      ");
				}
				next_addr = do_cmd_JMP(b1, b2);
				break;
		case 6: 
				if (bDecode) {
					printf("      ");
				}
				do_cmd_SVEC(b1, b2);
				break;
	}

	return next_addr;
}

void run(uint16_t addr)
{
	bDraw   = true;
	bDecode = false;
	bEnd = false;
	state.insptr = addr;
	while ( !bEnd )
	{
		state.insptr = dvg_next_command(state.insptr);
	}
}

void run_debug(uint16_t addr)
{
	bDraw   = true;
	bDecode = true;
	bEnd = false;
	state.insptr = addr;
	while ( !bEnd )
	{
		state.insptr = dvg_next_command(state.insptr);
	}
}

void run_decode_only(uint16_t addr)
{
	bDraw   = false;
	bDecode = true;
	bEnd = false;
	state.insptr = addr;
	while ( !bEnd )
	{
		state.insptr = dvg_next_command(state.insptr);
	}
}

void inject_test(int test_id, uint16_t addr)
{
	if (test_id == 1) {
		dvgram[addr++] = 0xFE;
		dvgram[addr++] = 0x87;
		dvgram[addr++] = 0xFE;
		dvgram[addr++] = 0x73;
	}
	if (test_id == 2) {
		dvgram[addr++] = 0x7F;
		dvgram[addr++] = 0xA3;
		dvgram[addr++] = 0xFF;
		dvgram[addr++] = 0x03;
	}
	if (test_id == 3) {
		dvgram[addr++] = 0x00;
		dvgram[addr++] = 0xB0;
	}
	if (test_id == 4) {
		dvgram[addr++] = 0xE4;
		dvgram[addr++] = 0xCA;
	}
	if (test_id == 5) {
		dvgram[addr++] = 0x00;
		dvgram[addr++] = 0xD0;
	}
	if (test_id == 6) {
		dvgram[addr++] = 0x0A;
		dvgram[addr++] = 0xEA;
	}
	if (test_id == 7) {
		dvgram[addr++] = 0x70;
		dvgram[addr++] = 0xFF;
	}

/*
	; Rock Pattern 1
	09E6: 08 F9           SVEC    scale=03(*16)  bri=0     x=0       y=1       (0.0000, 16.0000)
	09E8: 79 F9           SVEC    scale=03(*16)  bri=7     x=1       y=1       (16.0000, 16.0000)
	09EA: 79 FD           SVEC    scale=03(*16)  bri=7     x=1       y=-1      (16.0000, -16.0000)
	09EC: 7D F6           SVEC    scale=02(*8)   bri=7     x=-1      y=-2      (-8.0000, -16.0000)
	09EE: 79 F6           SVEC    scale=02(*8)   bri=7     x=1       y=-2      (8.0000, -16.0000)
	09F0: 8F F6           SVEC    scale=02(*8)   bri=8     x=-3      y=-2      (-24.0000, -16.0000)
	09F2: 8F F0           SVEC    scale=02(*8)   bri=8     x=-3      y=0       (-24.0000, 0.0000)
	09F4: 7D F9           SVEC    scale=03(*16)  bri=7     x=-1      y=1       (-16.0000, 16.0000)
	09F6: 78 FA           SVEC    scale=03(*16)  bri=7     x=0       y=2       (0.0000, 32.0000)
	09F8: 79 F9           SVEC    scale=03(*16)  bri=7     x=1       y=1       (16.0000, 16.0000)
	09FA: 79 FD           SVEC    scale=03(*16)  bri=7     x=1       y=-1      (16.0000, -16.0000)
	09FC: 00 D0           RTS      
*/
	if (test_id == 8) {
		dvgram[addr++] = 0x08; dvgram[addr++] = 0xF9;
		dvgram[addr++] = 0x79; dvgram[addr++] = 0xF9;
		dvgram[addr++] = 0x79; dvgram[addr++] = 0xFD;
		dvgram[addr++] = 0x7D; dvgram[addr++] = 0xF6;
		dvgram[addr++] = 0x79; dvgram[addr++] = 0xF6;
		dvgram[addr++] = 0x8F; dvgram[addr++] = 0xF6;
		dvgram[addr++] = 0x8F; dvgram[addr++] = 0xF0;
		dvgram[addr++] = 0x7D; dvgram[addr++] = 0xF9;
		dvgram[addr++] = 0x78; dvgram[addr++] = 0xFA;
		dvgram[addr++] = 0x79; dvgram[addr++] = 0xF9;
		dvgram[addr++] = 0x79; dvgram[addr++] = 0xFD;
		dvgram[addr++] = 0x00; dvgram[addr++] = 0xD0;
	}

	if (test_id == 9)
	{
	}
}

void run_tests()
{
	vdp_clear_screen();

	// tests for each instruction as per doc
	// load each test into 0x000 and just decode it
	// doesn't use "run" function
	bDecode = true;
	bDraw = false;
	for (int i=1; i<8; i++)
	{
		inject_test(i, 0x0000);
		state.insptr = 0x0000;
		state.insptr = dvg_next_command(state.insptr);
	}
	
	wait_for_any_key();
	vdp_clear_screen();

	// Rock 1 - write instructions to draw a rock (not from ROM) to 0x000
	state.global_scale = 1;
	inject_test(8, 0x0000);
	move_to(512,512);
	run_debug(0x0000); // show instructions as well

	wait_for_any_key();
	vdp_clear_screen();

	// Test pattern from ROM - directly exec subroutine
	state.global_scale = 7;
	move_to(0,0);
	run(0x0800); // show instructions

	wait_for_any_key();
	vdp_clear_screen();

	// Call ROM routine to draw "BANK ERROR"
	run(0x0888);
	// and again larger in a different part of the screen
	state.global_scale = 3;
	move_to(500,700);
	run(0x0890);

	wait_for_any_key();
	vdp_clear_screen();

	// draw the shrapnel pattern 1
	state.global_scale = 0;
	move_to(500,500);
	run_debug(0x0900);

}
