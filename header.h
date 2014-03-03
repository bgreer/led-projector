#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <cv.h>
#include <highgui.h>

#define PI 3.14159265359
#define TWOPI 6.28318530718
#define DEGTORAD 0.01745329251

#define MAXKEYS 64
#define MAXHANDLES 64
#define MAXEFFECTS 128
#define MAXDATA 5000
#define NUMPIXELS 232
#define SPHERERAD 0.0985

#define MAXCAMS 16

using namespace cv;

/* trying out some data structures */

typedef struct 
{
	int numpixels;
	// these are initialized by specifying the position
	// of the strip in 3d space. 
	float **space_coords; // [pixel] [x,y,z]
	float *angle; // total angle around
	float *lat, *lon;
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

// keyframe struct
typedef struct
{
	float time;
	float value;
} key;


// handle struct
typedef struct
{
	// updated attributes:
	float fade; // overall multiplier for color
	float color1[3];
	float color2[3];
	float attr[10];

	// keyframes
	key fade_keys[MAXKEYS];
	int fade_numkeys;
	key color1_keys[3][MAXKEYS];
	int color1_numkeys;
	key color2_keys[3][MAXKEYS];
	int color2_numkeys;
	key attr_keys[10][MAXKEYS];
	int attr_numkeys[10];

	char file[128];
	int preload;
} handle;

// effect struct
typedef struct effect effect;
struct effect
{
	void (*run)(effect*, handle*, float);// function pointer to effect
	float starttime;
	int handleindex;
	float pixels[3][NUMPIXELS]; // pixel buffer for blending
	float data[MAXDATA]; // extra data
	void *ptr;
	strip *str;
};

// function prototypes
void update_handle (handle *h, float currtime);
float interp_keys (key *k, int numkeys, float currtime);
void load_sequence_file (char *fname, effect *eff, int *numeffects, handle *h, strip *st);
void parse_handle_key (char *line, handle *h, float time);
void parse_effect (char *line, effect *eff, float time);

void effect_solid (effect *eff, handle *h, float currtime);
void effect_pulse (effect *eff, handle *h, float currtime);
void effect_circle (effect *eff, handle *h, float currtime);
void effect_video (effect *eff, handle *h, float currtime);
void effect_packet (effect *eff, handle *h, float currtime);
void effect_ring (effect *eff, handle *h, float currtime);
void effect_flicker (effect *eff, handle *h, float currtime);
void effect_image (effect *eff, handle *h, float currtime);
void effect_slp (effect *eff, handle *h, float currtime);
void effect_march (effect *eff, handle *h, float currtime);
float unit_random ();



/* function prototypes */

uint8_t process_pixelval (float val);
void waitms (long ms);
static int getLine (char *prmpt, char *buff, size_t sz);
float timespec_to_sec (struct timespec *t);

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

