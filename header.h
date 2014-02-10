#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <cv.h>
#include <highgui.h>

#define PI 3.14159265359
#define TWOPI 6.28318530718
#define DEGTORAD 0.01745329251

#define MAXCAMS 16

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
	uint8_t *sendbuffer;
} strip;

typedef struct
{
	strip *s;
	int numframes;
	uint8_t **r, **g, **b; // [frame] [pixel]
} strip_anim;

typedef struct
{
	int numcams;
	VideoCapture *cap[MAXCAMS];
	float campos[MAXCAMS][3];
	float camfov[MAXCAMS]; // in degrees
	Mat backframe[MAXCAMS];
	Mat foreframe[MAXCAMS];
} scanner;



/* function prototypes */

void waitms (long ms);
static int getLine (char *prmpt, char *buff, size_t sz);

// images.cpp
void map_image (Mat *img, strip *s);
void interp_pixel (Mat *img, float x, float y, uint8_t *r, uint8_t *g, uint8_t *b);

// ledstrip.c
void print_strip (strip *s);
void print_space_coords (strip *s);
void print_img_coords (strip *s);
void read_led_positions (strip *s, char *fname);
void init_strip_cylinder (strip *s, int numpixels, float ledspacing, float r, float zstep);
void clear_strip (strip *s);
void free_strip (strip *s);
void init_strip_sphere (strip *s, int numpixels, float ledspacing, float r, float turns);

// camera.cpp
void display_webcam();
void start_scan (scanner *sc, strip *st);
void add_camera (scanner *s, VideoCapture *cap);
void grab_frame (scanner *s, int cam, int frame);
int find_led (scanner *s, int cam, float *x, float *y);

// comm.c
void openComm (char *fname);
void closeComm ();
void setPixels(strip *s);
void sendShow();

// effects.c
void effect_fadeto (strip *s, float time, int cr, int cg, int cb);
void effect_solid (strip *s, int cr, int cg, int b);
void effect_pulse (strip *s, float rate, float iters, int cr1, int cg1, int cb1, int cr2, int cg2, int cb2);
void effect_swipe (strip *s, int direction, float speed, float width, int cr, int cg, int cb);
float timespec_to_sec (struct timespec *t);
uint8_t bound (float val);
float unit_random ();
