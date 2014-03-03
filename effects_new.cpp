#include "header.h"


// marching
// USES: fade
// color1
// color2
// attr[0] = division type (0=random, 1=hemispheres)
// attr[1] = overall rate
// attr[2] = single group on time, set to 0.5 for only one group at a time, 1.0 for both all the time
// attr[3] = hemisphere theta, only used if attr[0]=1
// attr[4] = turn-on fraction, this is expressed as a fraction of attr[2]
void effect_march(effect *eff, handle *h, float currtime)
{
	int ii;
	float *group;
	float amp, cycle, pos;

	group = &(eff->data[4]);

	// if we've just started, assign groups
	if (eff->data[0] == 0.0)
	{
		eff->data[0] = currtime;
		for (ii=0; ii<NUMPIXELS; ii++)
		{
			switch ((int)(h->attr[0]))
			{
				case 0: // random
					group[ii] = 0.0;
					if (unit_random() < 0.5) group[ii] = 1.0;
					break;
				case 1: // hemisphere
					group[ii] = 0.0;
					if (eff->str->space_coords[ii][0]*cos(h->attr[3]*DEGTORAD) + eff->str->space_coords[ii][1]*sin(h->attr[3]*DEGTORAD) > 0.0) group[ii] = 1.0; 
					break;
			}
		}
	}

	// attr 2 is single-group on-time
	// attr 4 is ramp-up fraction

	// need to know what 'cycle' this is, use fmod?
	// need to know how far we are in that cycle
	cycle = floor((currtime - eff->starttime)*h->attr[1]);
	pos = (currtime - eff->starttime)*h->attr[1] - cycle; // [0,1)
//	printf("%f %f %f %f\n", cycle, pos, h->attr[4], h->attr[2]);

	for (ii=0; ii<NUMPIXELS; ii++)
	{
		if (group[ii] == 0.0)
		{
			if (pos < h->attr[4]*h->attr[2])
				amp = pos / (h->attr[4]*h->attr[2]); // rising
			else if (pos < h->attr[2])
				amp = 1.0 - (pos-h->attr[4]*h->attr[2])/(h->attr[2]*(1.0-h->attr[4])); // decaying
			else
				amp = 0.0; // dead
			eff->pixels[0][ii] = h->color1[0] * h->fade * amp;
			eff->pixels[1][ii] = h->color1[1] * h->fade * amp;
			eff->pixels[2][ii] = h->color1[2] * h->fade * amp;
		} else {
			if ((pos < h->attr[4]*h->attr[2]+0.5 && pos > 0.5) || 
						(pos < h->attr[4]*h->attr[2]-0.5 && pos < 0.5))
				amp = (pos-0.5) / (h->attr[4]*h->attr[2]); // rising TODO fix for second conditional
			else if (pos < h->attr[2]+0.5 && pos > 0.5)
				amp = 1.0 - (pos-0.5-h->attr[4]*h->attr[2])/(h->attr[2]*(1.0-h->attr[4])); // decaying, first segment
			else if (pos < h->attr[2]-0.5 && pos < 0.5)
				amp = 1.0 - (pos+0.5-h->attr[4]*h->attr[2])/(h->attr[2]*(1.0-h->attr[4])); // decaying, next segment
			else
				amp = 0.0; // dead
			eff->pixels[0][ii] = h->color2[0] * h->fade * amp;
			eff->pixels[1][ii] = h->color2[1] * h->fade * amp;
			eff->pixels[2][ii] = h->color2[2] * h->fade * amp;
		}
	}

}

// single-line packet
// USES: fade
// color1
// attr[0] = theta, direction to display the packet to
// attr[1] = width in pixels
// attr[2] = gaussian width
// attr[3] = speed
// attr[4] = loopnumber
void effect_slp (effect *eff, handle *h, float currtime)
{
	int ii;
	float mintheta, maxtheta, loc, amp;

	mintheta = h->attr[0]*DEGTORAD - PI/2. + h->attr[4]*TWOPI;
	maxtheta = h->attr[0]*DEGTORAD + PI/2. + h->attr[4]*TWOPI;

	if (h->attr[3] > 0.0)
		loc = mintheta - (h->attr[1]*0.16) + h->attr[3] * 0.16 * (currtime - eff->starttime);
	else
		loc = maxtheta + (h->attr[1]*0.16) + h->attr[3] * 0.16 * (currtime - eff->starttime);

	for (ii=0; ii<NUMPIXELS; ii++)
	{
		if (eff->str->angle[ii] >= mintheta && eff->str->angle[ii] <= maxtheta)
		{
			amp = exp(-pow((eff->str->angle[ii]-loc)/(h->attr[2]*0.16),2.));
			eff->pixels[0][ii] = h->color1[0] * h->fade * amp;
			eff->pixels[1][ii] = h->color1[1] * h->fade * amp;
			eff->pixels[2][ii] = h->color1[2] * h->fade * amp;
		} else {
			eff->pixels[0][ii] = 0.0;
			eff->pixels[1][ii] = 0.0;
			eff->pixels[2][ii] = 0.0;
		}
	}
}


// USES: fade
// attr[0]=theta in deg, gives angle to project at
// attr[1]=proj, method of projection (0=planar, 1=cylindrical, 2=spherical)
// str=filename, not keyframed (obviously)
void effect_image (effect *eff, handle *h, float currtime)
{
	int ii;
	Mat *frames;
	uint8_t r, g, b;
	float x, y, thisx, thisy, thisz, theta;

	// sort out frame data
	frames = (Mat*) eff->ptr;

	theta = h->attr[0]*DEGTORAD;

//	imshow("Source", frames[0]);
	for (ii=0; ii<NUMPIXELS; ii++)
	{
		r = 0;
		g = 0;
		b = 0;
		thisx = eff->str->space_coords[ii][0];
		thisy = eff->str->space_coords[ii][1];
		thisz = eff->str->space_coords[ii][2];
		// make x and y [0,1]
		switch ((int)(h->attr[1]))
		{
			case 0: // planar
				x = ((thisx*cos(theta) + thisy*sin(theta)) + SPHERERAD) / (2.*SPHERERAD);
				y = (thisz + SPHERERAD) / (2.*SPHERERAD);
				break;
			case 1: // cylindrical
				x = fmod(atan2(thisy,thisx) + theta, TWOPI) / TWOPI;
				y = (thisz + SPHERERAD) / (2.*SPHERERAD);
				break;
			case 2: // spherical
				x = fmod(atan2(thisy,thisx) + theta, TWOPI) / TWOPI;
				y = (atan2(thisz,sqrt(thisx*thisx+thisy*thisy))+PI) / TWOPI;
				break;
		}
		interp_pixel(&(frames[0]), x, y, &r, &g, &b);
		eff->pixels[0][ii] = ((float)r) * h->fade;
		eff->pixels[1][ii] = ((float)g) * h->fade;
		eff->pixels[2][ii] = ((float)b) * h->fade;
	}

}


// USE: fade
// color1
// color2
// color3
// attr[0] = rate of fire
// attr[1] = duration
// attr[2] = turn-on fraction
// if colorX != 0,0,0 use it
// use eff->data for persistent storage
// TODO: use gradual turn-on, add third color option
void effect_flicker (effect *eff, handle *h, float currtime)
{
	int ii, ij, ik, numcolors;
	float p, gamma, fac, f;
	float *cdf, *amp, *type, *direction;

	f = h->attr[2];
	if (f < 1e-2) f = 1e-2;

	amp = &(eff->data[30]);
	type = &(eff->data[40+NUMPIXELS]);
	direction = &(eff->data[41+NUMPIXELS*2]);

	// count colors
	numcolors = 1;
	if (h->color2[0] > 0.0 || h->color2[1] > 0.0 || h->color2[2] > 0.0) numcolors++;

	// WORK OUT PROBABILITIES
	cdf = &(eff->data[3]);
	if (eff->data[0] == 0.0)
		eff->data[0] = currtime;
	
	// only go if its been a bit of time
	if (currtime - eff->data[0] > 1./h->attr[0])
	{
		gamma = h->attr[0] * (currtime - eff->data[0]);
		eff->data[0] = currtime; // store current time for next time
		// only compute new cdf if gamma has changed enough?
		if (eff->data[2] != gamma)
		{
			eff->data[2] = gamma;
			// create new lookup table
			for (ii=0; ii<20; ii++)
			{
				cdf[ii] = 0.0;
				for (ij=0; ij<ii; ij++)
				{
					// compute factorial of ij
					fac = 1.0;
					for (ik=1;ik<=ij; ik++)
						fac *= (float)ik;
					cdf[ii] += pow(gamma, ij) / fac;
				}
				cdf[ii] *= exp(-gamma);
			}
		}
	
		// figure out how many to fire off
		p = unit_random();
		// find which entry in cdf is next highest
		ii = 0;
		while (cdf[ii] < p && ii<19) ii++;
//		printf("%f %d %f %f %f %f\n", gamma, ii, p, cdf[0], cdf[1], cdf[2]);
		// need to launch ii pixels
		for (ij=0; ij<ii; ij++)
		{
			// pick a pixel, small chance of overlap?
			p = unit_random() * NUMPIXELS;
			if (direction[(int)floor(p)] == 0.0)
			{
				amp[(int)floor(p)] = 0.0;
				type[(int)floor(p)] = floor(unit_random()*numcolors);
				direction[(int)floor(p)] = 1.0;
			}
		}
	}

	// clear pixels first
	memset(eff->pixels[0], 0, NUMPIXELS * sizeof(float));
	memset(eff->pixels[1], 0, NUMPIXELS * sizeof(float));
	memset(eff->pixels[2], 0, NUMPIXELS * sizeof(float));

	for (ii=0; ii<NUMPIXELS; ii++)
	{
		if (type[ii] == 0.0)
		{
			eff->pixels[0][ii] = h->color1[0] * h->fade * amp[ii];
			eff->pixels[1][ii] = h->color1[1] * h->fade * amp[ii];
			eff->pixels[2][ii] = h->color1[2] * h->fade * amp[ii];
		} else {
			eff->pixels[0][ii] = h->color2[0] * h->fade * amp[ii];
			eff->pixels[1][ii] = h->color2[1] * h->fade * amp[ii];
			eff->pixels[2][ii] = h->color2[2] * h->fade * amp[ii];
		}
	}

	// store last time
	if (eff->data[1] == 0.0) eff->data[1] = currtime;

	// change amplitudes
	for (ii=0; ii<NUMPIXELS; ii++)
	{
		if (direction[ii] == 1.0)
		{
			amp[ii] += (currtime - eff->data[1]) / (h->attr[1] * f);
			if (amp[ii] >= 1.0)
			{
				amp[ii] = 1.0;
				direction[ii] = -1.0;
			}
		} else if (direction[ii] == -1.0) {
			amp[ii] -= (currtime - eff->data[1]) / (h->attr[1]*(1.-f));
			if (amp[ii] <= 0.0)
			{
				amp[ii] = 0.0;
				direction[ii] = 0.0;
			}
		}
	}
	eff->data[1] = currtime;
}


// USES: fade
// color1
// color2
// attr[0] = start led
// attr[1] = end led
// attr[2] = speed
// attr[3] = length scale of variation
void effect_ring (effect *eff, handle *h, float currtime)
{
	int ii, start, end;
	float arg;

	// clear pixels first
	memset(eff->pixels[0], 0, NUMPIXELS * sizeof(float));
	memset(eff->pixels[1], 0, NUMPIXELS * sizeof(float));
	memset(eff->pixels[2], 0, NUMPIXELS * sizeof(float));
	
	start = (int)h->attr[0];
	end = (int) h->attr[1];

	if (start < 0) start = 0;
	if (start >= NUMPIXELS) start = NUMPIXELS-1;
	if (end < 0) end = 0;
	if (end >= NUMPIXELS) end = NUMPIXELS-1;
	for (ii=start; ii<=end; ii++)
	{
		arg = (currtime - eff->starttime)*h->attr[2] + ii*PI/h->attr[3];
		eff->pixels[0][ii] = ((h->color1[0]-h->color2[0])*pow(sin(arg),2) + h->color2[0])*h->fade;
		eff->pixels[1][ii] = ((h->color1[1]-h->color2[1])*pow(sin(arg),2) + h->color2[1])*h->fade;
		eff->pixels[2][ii] = ((h->color1[2]-h->color2[2])*pow(sin(arg),2) + h->color2[2])*h->fade;
	}
}



// USES: fade
// color1
// attr[0] = width in pixels
// attr[1] = gaussian width in pixels
// attr[2] = speed
// attr[3] = origin
// for hard edges, attr[1] >> attr[0]
void effect_packet (effect *eff, handle *h, float currtime)
{
	int ii, start, end;
	float amp, loc;

	// figure out where it should be centered
	loc = h->attr[3] + h->attr[2] * (currtime - eff->starttime);

	// clear pixels first
	memset(eff->pixels[0], 0, NUMPIXELS * sizeof(float));
	memset(eff->pixels[1], 0, NUMPIXELS * sizeof(float));
	memset(eff->pixels[2], 0, NUMPIXELS * sizeof(float));

	// set pixels around loc
	start = loc-(int)(h->attr[0]);
	end = loc+(int)(h->attr[0]);
	if (start < 0) start = 0;
	if (start >= NUMPIXELS) start = NUMPIXELS-1;
	if (end < 0) end = 0;
	if (end >= NUMPIXELS) end = NUMPIXELS-1;
	for (ii=start; ii<=end; ii++)
	{
		amp = exp(-pow((((float)ii)-loc)/h->attr[1],2.));
		eff->pixels[0][ii] = h->color1[0] * h->fade * amp;
		eff->pixels[1][ii] = h->color1[1] * h->fade * amp;
		eff->pixels[2][ii] = h->color1[2] * h->fade * amp;
	}
}

// USES: fade
// attr[0]=speed
// attr[1]=theta in deg, gives angle to project at
// attr[2]=proj, method of projection (0=planar, 1=cylindrical, 2=spherical)
// str=filename, not keyframed (obviously)
//
// probably need to pre-load the video file and load each frame into a MAT
// then pass an array of Mats as eff->ptr;
// load number of frames into data[0]
void effect_video (effect *eff, handle *h, float currtime)
{
	int ii;
	Mat *frames;
	int numframes, currframe;
	uint8_t r, g, b;
	float x, y, thisx, thisy, thisz, theta;

	// sort out frame data
	frames = (Mat*) eff->ptr;
	numframes = (int)(eff->data[0]);
	// figure out current frame
	currframe = (int) ((currtime - eff->starttime) * h->attr[0]);

	theta = h->attr[1]*DEGTORAD;

	if (currframe < numframes)
	{
		imshow("Source", frames[currframe]);
		for (ii=0; ii<NUMPIXELS; ii++)
		{
			r = 0;
			g = 0;
			b = 0;
			thisx = eff->str->space_coords[ii][0];
			thisy = eff->str->space_coords[ii][1];
			thisz = eff->str->space_coords[ii][2];
			// make x and y [0,1]
			switch ((int)(h->attr[2]))
			{
				case 0: // planar
					x = ((thisx*cos(theta) + thisy*sin(theta)) + SPHERERAD) / (2.*SPHERERAD);
					y = (thisz + SPHERERAD) / (2.*SPHERERAD);
					break;
				case 1: // cylindrical
					x = fmod(atan2(thisy,thisx) + theta, TWOPI) / TWOPI;
					y = (thisz + SPHERERAD) / (2.*SPHERERAD);
					break;
				case 2: // spherical
					x = fmod(atan2(thisy,thisx) + theta, TWOPI) / TWOPI;
					y = (atan2(thisz,sqrt(thisx*thisx+thisy*thisy))+PI) / TWOPI;
					break;
			}
			interp_pixel(&(frames[currframe]), x, y, &r, &g, &b);
			eff->pixels[0][ii] = ((float)r) * h->fade;
			eff->pixels[1][ii] = ((float)g) * h->fade;
			eff->pixels[2][ii] = ((float)b) * h->fade;
		}
	} else {
		// clear pixels
		memset(eff->pixels[0], 0, NUMPIXELS * sizeof(float));
		memset(eff->pixels[1], 0, NUMPIXELS * sizeof(float));
		memset(eff->pixels[2], 0, NUMPIXELS * sizeof(float));
	}
}

// NOTE ON LON/LAT
// given an x,y,z for each led, finding lat/lon is
// lon = atan(y,x) (scale and offset appropriately)
// lat = atan(z,r)

// circle of light
// USES: color1, fade
// attr[0]=lon, attr[1]=lat, attr[2]=radius in degrees
// attr[3] = gaussian width
void effect_circle (effect *eff, handle *h, float currtime)
{
	int ii;
	float lat, lon, r, thisx, thisy, thisz, arg, amp;
	// first, figure out lat/lon for each pixel
//	printf("%f %f %f %f\n", h->attr[2], h->fade, h->attr[0], h->attr[1]);
	for (ii=0; ii<NUMPIXELS; ii++)
	{
		thisx = eff->str->space_coords[ii][0];
		thisy = eff->str->space_coords[ii][1];
		thisz = eff->str->space_coords[ii][2];
		lat = atan2(thisz, sqrt(thisx*thisx+thisy*thisy));
		lon = atan2(thisy, thisx);
		// figure out great-circle distance to target lat-lon
		arg = sin(lat)*sin(h->attr[1] * DEGTORAD) + cos(lat)*cos(h->attr[1] * DEGTORAD)*cos(lon-(h->attr[0] * DEGTORAD));
		r = acos(arg);
		// compare to target distance
//		printf("%d %f %f %f %f\n", ii, lon, lat, r, arg);
		if (fmod(r, PI) <= h->attr[2]*DEGTORAD)
		{
			amp = exp(-pow(r/(h->attr[3]*DEGTORAD),2.));
			eff->pixels[0][ii] = h->color1[0] * h->fade * amp;
			eff->pixels[1][ii] = h->color1[1] * h->fade * amp;
			eff->pixels[2][ii] = h->color1[2] * h->fade * amp;
		} else {
			eff->pixels[0][ii] = 0;
			eff->pixels[1][ii] = 0;
			eff->pixels[2][ii] = 0;
		}
	}
}


// set entire strip to one color
// USES: color1
void effect_solid (effect *eff, handle *h, float currtime)
{
	int ii;

	for (ii=0; ii<NUMPIXELS; ii++)
	{
		eff->pixels[0][ii] = h->color1[0] * h->fade;
		eff->pixels[1][ii] = h->color1[1] * h->fade;
		eff->pixels[2][ii] = h->color1[2] * h->fade;
	}
	
}

// pulse entire strip between two colors
// USES: color1, color2, attr[0]
// attr[0] is cycles per second
void effect_pulse (effect *eff, handle *h, float currtime)
{
	int ii;
	float arg;

	arg = PI * h->attr[0] * (currtime - eff->starttime);

	for (ii=0; ii<NUMPIXELS; ii++)
	{
		eff->pixels[0][ii] = ((h->color1[0]-h->color2[0])*pow(sin(arg),2) + h->color2[0])*h->fade;
		eff->pixels[1][ii] = ((h->color1[1]-h->color2[1])*pow(sin(arg),2) + h->color2[1])*h->fade;
		eff->pixels[2][ii] = ((h->color1[2]-h->color2[2])*pow(sin(arg),2) + h->color2[2])*h->fade;
	}
	
}

float unit_random ()
{
	return ((float)rand())/((float)RAND_MAX);
}

