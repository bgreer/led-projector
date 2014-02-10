#include "header.h"
#include <time.h>

/* some functions that create various effects on the led strip
	examples:
		random blinking
X		swipe along strip
X		pulse
		fade existing up/down
X		set to uniform color
		twinkle

*/

void effect_twinkle (strip *s, float speed, float speed_variance, float duration, int cr, int cg, int cb)
{
	struct timespec time0, time1;
	float x;
	int ii;
	float phase[s->numpixels];
	float led_speed[s->numpixels];
	// give each led a random phase and speed
	for (ii=0; ii<s->numpixels; ii++)
	{
		phase[ii] = unit_random() * TWOPI;
		led_speed[ii] = speed + ((unit_random()-0.5) * speed_variance);
	}
	// TODO : linearly ramp up to initial value
	x = 0.0;
	while (x <= duration)
	{
		
	}
}

/* linearly fade from current state to a uniform color
	time, amount of time it take to get there in sec
	c* is final color
*/
void effect_fadeto (strip *s, float time, int cr, int cg, int cb)
{
	struct timespec time0, time1;
	uint8_t orig_r[s->numpixels];
	uint8_t orig_g[s->numpixels];
	uint8_t orig_b[s->numpixels];
	int ii;
	float x;

	// store original values
	memcpy(orig_r, s->r, s->numpixels * sizeof(uint8_t));
	memcpy(orig_g, s->g, s->numpixels * sizeof(uint8_t));
	memcpy(orig_b, s->b, s->numpixels * sizeof(uint8_t));

	clock_gettime(CLOCK_MONOTONIC, &time0);
	x = 0.0;
	while (x <= 1.0)
	{
		clock_gettime(CLOCK_MONOTONIC, &time1);
		x = (timespec_to_sec(&time1) - timespec_to_sec(&time0)) / time;
		for (ii=0; ii<s->numpixels; ii++)
		{
			s->r[ii] = bound(orig_r[ii] + x*(cr-orig_r[ii]));
			s->g[ii] = bound(orig_r[ii] + x*(cr-orig_r[ii]));
			s->b[ii] = bound(orig_r[ii] + x*(cr-orig_r[ii]));
		}
	}
}

void effect_solid (strip *s, int cr, int cg, int b)
{
	memset(s->r, bound(cr), s->numpixels);
	memset(s->r, bound(cr), s->numpixels);
	memset(s->r, bound(cr), s->numpixels);
	setPixels(s);
}

/*
	rate in flashes per second
	iters, number of cycles to do
	c?1 is color 1
	c?2 is color 2
*/
void effect_pulse (strip *s, float rate, float iters, int cr1, int cg1, int cb1, int cr2, int cg2, int cb2)
{
	struct timespec time0, time1;
	float pos, val;
	int ii;

	pos = 0.0;

	clock_gettime(CLOCK_MONOTONIC, &time0);
	while (pos <= iters)
	{
		clock_gettime(CLOCK_MONOTONIC, &time1);
		pos = rate*(timespec_to_sec(&time1) - timespec_to_sec(&time0));
		val = pow(sin(pos*PI),2.0);
		memset(s->r, bound(val*(cr2-cr1) + cr1), s->numpixels);
		memset(s->g, bound(val*(cg2-cg1) + cg1), s->numpixels);
		memset(s->b, bound(val*(cb2-cb1) + cb1), s->numpixels);
		setPixels(s);
	}
	
}

/* 
	direction = -1,1, sets direction of swipe
	speed in leds per second
	width in leds, of gaussian packet
	c* is color value
*/
void effect_swipe (strip *s, int direction, float speed, float width, int cr, int cg, int cb)
{
	struct timespec time0, time1;
	float pos, travelled, currtime, lasttime, origin;
	float gauss[s->numpixels];
	int ii;

	pos = 0;

	if (direction == 1)
		origin = -width;
	else
		origin = (float)(s->numpixels) + width;
	
	travelled = 0.0;
	clock_gettime(CLOCK_MONOTONIC, &time0);
	while (travelled < s->numpixels + width*2)
	{
		// how far along should we be
		clock_gettime(CLOCK_MONOTONIC, &time1);
		currtime = timespec_to_sec(&time1) - timespec_to_sec(&time0);
		pos = origin + direction * speed * currtime;
		travelled = speed*currtime;
		// set up gaussian
		for (ii=0; ii<s->numpixels; ii++)
		{
			gauss[ii] = exp(-pow((ii-pos)/width,2.));
			s->r[ii] = bound(gauss[ii] * cr);
			s->g[ii] = bound(gauss[ii] * cg);
			s->b[ii] = bound(gauss[ii] * cb);
		}
		// send values out to led strip
		setPixels(s);
//		sendShow();
	}
}

float timespec_to_sec (struct timespec *t)
{
	return (float)(t->tv_sec) + 1e-9 * (uint32_t)(t->tv_nsec);
}

// bound a color value between 0 and 255
uint8_t bound (float val)
{
	if (val > 255.0) return 255;
	if (val < 0.0) return 0;
	return (uint8_t) val;
}

float unit_random ()
{
	return ((float)rand())/((float)RAND_MAX);
}

