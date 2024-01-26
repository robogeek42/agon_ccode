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

double sign(double n);
double ray(double X, double Y, double Z, double U, double V, double W, double I);
double fnc(double X, double Y, double Z, double U, double V, double W);


int dither[16] = {
	0, 24, 6, 30, 36, 12, 42, 18, 9, 33, 3, 27, 45, 21, 39, 15
};
int main(/*int argc, char *argv[]*/)
{
	vdp_vdu_init();
	if ( vdp_key_init() == -1 ) return 1;
	vdp_set_key_event_handler( key_event_handler );

	vdp_mode(MODE);
	vdp_logical_scr_dims(false);
	vdp_cursor_enable(false);

	double X=0, Y = -0.1, Z=3; // camera position
	for(int N=8; N<240; N++) // iterate over screen pixel rows
	{
		for(int M=0; M<320; M++) // iterate over screen pixel columns
		{
			double U=(M-159.5f)/160.0f; // x component of ray vector
			double V=(N-127.5f)/160.0f; // y component of ray vector
			double W=1.0f/sqrt((U*U)+(V*V)+1.0f); // z component of ray vector
			U=U*W; V=V*W; // normalise x and y components
			double I = sign(U);
			double C=ray(X,Y,Z,U,V,W,I); // fire ray from X,Y,Z along U,V,W
			int col = 3-(int)(48*sqrt(C)+dither[(M%4)+(N%4)*4]/3) / 16; // set draw colour using ordered dithering
			vdp_gcol(0, col);
			vdp_point(M,248-N); //REM plot pixel
		}
		vdp_update_key_state();
	}

	vdp_logical_scr_dims(true);
	vdp_cursor_enable(true);
	wait();
	return 0;
}

double ray(double X, double Y, double Z, double U, double V, double W, double I)
{
  double E=X-I; double F=Y-I; double G=Z; // vector from sphere centre to ray start
  double P=(U*E)+(V*F)-(W*G); // dot product? Z seems to be flipped
  double D=(P*P)-(E*E)-(F*F)-(G*G)+1.0f;

  if (D<=0) return fnc(X,Y,Z,U,V,W); // didn't hit anything; return colour

  double T=-P-sqrt(D); 
  if (T<=0) return fnc(X,Y,Z,U,V,W); // still didn't hit anything; return colour

  X=X+T*U; Y=Y+T*V; Z=Z-T*W; // new ray start position
  E=X-I; F=Y-I; G=Z; // vector from sphere centre to new ray start
  P=2.0f*((U*E)+(V*F)-(W*G)); // dot product shenanigans?
  U=U-(P*E); V=V-(P*F); W=W+(P*G); // new ray direction vector
  I= 0.0f-I; // we'd hit one sphere, so flip x and y coordinates to give other
  return ray(X,Y,Z,U,V,W,I); // return colour from new ray
}

double fnc(double X, double Y, double Z, double U, double V, double W)
{
  if (V>=0) return V; // facing up at all? return ray Y component for simple sky gradient
  double P=(Y+2.0f)/V; // use height for overall checkerboard scale and y component of vector for perspective
  int c  = (int)(floor(X-U*P) + floor(Z-W*P)) & 1 ;
  return -V*((double)c/2.0f + 0.3f) + 0.2f; // multiply simple gradient by checkerboard
}


double sign(double n) 
{ 
	if (n==0) return 0.0f;
	if (n<0) return -1.0f;
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
