#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <cv.h>
#include <highgui.h>

#define PI 3.14159265359
#define TWOPI 6.28318530718

using namespace cv;

/* trying out some data structures */

typedef struct 
{
	int numpixels;
	// these are initialized by specifying the position
	// of the strip in 3d space. 
	float **space_coords; // [pixel] [x,y,z]
	// the space coords are then projected back on to a
	// 2d plane, the image plane
	float **img_coords; // [pixel] [x,y]
	// color values for each pixel
	uint8_t *r, *g, *b; // [pixel]
} strip;

typedef struct
{
	strip *s;
	int numframes;
	uint8_t **r, **g, **b; // [frame] [pixel]
} strip_anim;

/* function prototypes */

// images.cpp
void map_image (Mat *img, strip *s);
void interp_pixel (Mat *img, float x, float y, uint8_t *r, uint8_t *g, uint8_t *b);

// ledstrip.c
void print_strip (strip *s);
void print_space_coords (strip *s);
void print_img_coords (strip *s);
void init_strip_cylinder (strip *s, int numpixels, float ledspacing, float r, float zstep);
void free_strip (strip *s);
