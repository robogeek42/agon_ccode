#ifndef _WIKINOISE_H
#define _WIKINOISE_H

// From Wikipedia entry on Perlin Noise 
// https://en.wikipedia.org/wiki/Perlin_noise
//
typedef struct {
	float x;
	float y;
} Vector2;

float interpolate(float a0, float a1, float w);
Vector2 randomGradient(int ix, int iy);
float dotGridGradient(int ix, int iy, float x, float y);
float perlin(float x, float y);

#endif
