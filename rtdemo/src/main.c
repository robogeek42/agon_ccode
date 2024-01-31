#include "agon/vdp_vdu.h"
#include "agon/vdp_key.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define MODE 10
#define SCREENW  320
#define SCREENH  240

void key_event_handler( KEY_EVENT key_event );
void wait_clock( clock_t ticks );

void wait()
{
	char k=getchar();
	if (k=='q') exit(0);
}

float sign(float n);
float ray(float X, float Y, float Z, float U, float V, float W, float I);
float fnc(float X, float Y, float Z, float U, float V, float W);


int dither[16] = {
	0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5
};
int main(/*int argc, char *argv[]*/)
{
	vdp_vdu_init();
	if ( vdp_key_init() == -1 ) return 1;
	vdp_set_key_event_handler( key_event_handler );

	vdp_mode(MODE);
	vdp_logical_scr_dims(false);
	vdp_cursor_enable(false);

	float X=0.0f, Y = -0.1f, Z=3.0f; // camera position
	for(int N=8; N<=238; N++) // iterate over screen pixel rows
	{
		for(int M=0; M<=319; M++) // iterate over screen pixel columns
		{
			float U,V,W,I,C;
			int col;

			U=(M - 159.5f)/160.0f; // x component of ray vector
			V=(N - 127.5f)/160.0f; // y component of ray vector
			W=1.0f/sqrt(U*U + V*V + 1.0f); // z component of ray vector
			U=U*W; V=V*W; // normalise x and y components
			I = sign(U);
			C=ray(X,Y,Z,U,V,W,I); // fire ray from X,Y,Z along U,V,W
			col = 3-(int)((48*sqrt(C)+dither[(M%4)+(N%4)*4]) / 16); // set draw colour using ordered dithering
			vdp_gcol(0, col);
			vdp_point(M,247-N); //REM plot pixel
		}
		vdp_update_key_state();
	}

	wait();

	vdp_logical_scr_dims(true);
	vdp_cursor_enable(true);
	return 0;
}

float ray(float X, float Y, float Z, float U, float V, float W, float I)
{
	float E,F,G,P,D,T;

	E=X-I; F=Y-I; G=Z; // vector from sphere centre to ray start
	P=(U*E) + (V*F) - (W*G); // dot product? Z seems to be flipped
	D=(P*P) - (E*E) - (F*F) - (G*G) + 1.0f;

	if (D<=0) return fnc(X,Y,Z,U,V,W); // didn't hit anything; return colour

	T = -P - sqrt(D); 
	if (T<=0) return fnc(X,Y,Z,U,V,W); // still didn't hit anything; return colour

	X=X+T*U; Y=Y+T*V; Z=Z-T*W; // new ray start position
	E=X-I; F=Y-I; G=Z; // vector from sphere centre to new ray start
	P=2.0f*((U*E) + (V*F) - (W*G)); // dot product shenanigans?
	U=U-(P*E); V=V-(P*F); W=W+(P*G); // new ray direction vector
	I= 0.0f-I; // we'd hit one sphere, so flip x and y coordinates to give other
	return ray(X,Y,Z,U,V,W,I); // return colour from new ray
}

float fnc(float X, float Y, float Z, float U, float V, float W)
{
	float P;
	int c;

	if (V>=0) return V; // facing up at all? return ray Y component for simple sky gradient
	
	P=(Y+2.0f)/V; // use height for overall checkerboard scale and y component of vector for perspective
	c  = (int)(floor(X-U*P) + floor(Z-W*P)) & 1 ;
	return -V*((float)c/2.0f + 0.3f) + 0.2f; // multiply simple gradient by checkerboard
}


float sign(float n) 
{ 
	if (n==0.0f) return 0.0f;
	if (n<0.0f) return -1.0f;
	return 1.0f;
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
}

void wait_clock( clock_t ticks )
{
	clock_t ticks_now = clock();

	do {
		vdp_update_key_state();
	} while ( clock() - ticks_now < ticks );
}
