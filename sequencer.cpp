#include "header.h"
#include <time.h>



// take a handle, update the current attributes inside
// based on the current time and what keyframes exist
void update_handle (handle *h, float currtime)
{
	int ii;
	int leftkey, rightkey;

	// go through each keyframed attribute
	h->fade = interp_keys(h->fade_keys, h->fade_numkeys, currtime);
	for (ii=0; ii<3; ii++)
		h->color1[ii] = interp_keys(h->color1_keys[ii], h->color1_numkeys, currtime);
	for (ii=0; ii<3; ii++)
		h->color2[ii] = interp_keys(h->color2_keys[ii], h->color2_numkeys, currtime);
	for (ii=0; ii<10; ii++)
		h->attr[ii] = interp_keys(h->attr_keys[ii], h->attr_numkeys[ii], currtime);
}

// helper function to interpolate some keys, given a time
float interp_keys (key *k, int numkeys, float currtime)
{
	int ii, rightkey, leftkey;
	leftkey = -1;
	rightkey = -1;

	if (numkeys == 0) return 0.0;
	if (numkeys == 1) return k[0].value;

	// find left and right
	for (ii=0; ii<numkeys; ii++)
		if (k[ii].time <= currtime) leftkey = ii;
	for (ii=numkeys-1; ii>=0; ii--)
		if (k[ii].time >= currtime) rightkey = ii;

	if (rightkey == leftkey) return k[rightkey].value; // same key

	if (leftkey < 0) return k[rightkey].value; // no left key
	if (rightkey < 0) return k[leftkey].value; // no right key

	// linearly interpolate
	return (k[rightkey].value-k[leftkey].value) * (currtime-k[leftkey].time) / (k[rightkey].time-k[leftkey].time) + k[leftkey].value;
}

// read in a text file containing the sequence
// create a list of effects with their start time and their handle
// create list of handles, add keyframes to them
// assume eff is allocated, return number of effects loaded
// assume h is allocated, return number of handles loaded
void load_sequence_file (char *fname, effect *eff, int *numeffects, handle *h, strip *st)
{
	FILE *fp;
	ssize_t readnum;
	char *line = NULL;
	int index;
	char buffer[128];
	size_t len = 0;
	int ii, ij, curreffect, currhandle;
	float currtime;

	// initialize each handle
	for (ii=0; ii<MAXHANDLES; ii++)
	{
		h[ii].fade = 0.0;
		h[ii].color1[0] = 0.0;
		h[ii].color1[1] = 0.0;
		h[ii].color1[2] = 0.0;
		h[ii].color2[0] = 0.0;
		h[ii].color2[1] = 0.0;
		h[ii].color2[2] = 0.0;
		for (ij=0; ij<10; ij++)
		{
			h[ii].attr[ij] = 0.0;
			h[ii].attr_numkeys[ij] = 0;
		}
		h[ii].fade_numkeys = 0;
		h[ii].color1_numkeys = 0;
		h[ii].color2_numkeys = 0;
		memset(h[ii].file, 0, 128);
		h[ii].preload = 0;
	}
	// initialize each effect
	for (ii=0; ii<MAXEFFECTS; ii++)
	{
		eff[ii].run = NULL;
		eff[ii].starttime = 0.0;
		eff[ii].handleindex = 0;
		for (ij=0; ij<NUMPIXELS; ij++)
		{
			eff[ii].pixels[0][ij] = 0.0;
			eff[ii].pixels[1][ij] = 0.0;
			eff[ii].pixels[2][ij] = 0.0;
		}
		for (ij=0; ij<MAXDATA; ij++)
		{
			eff[ii].data[ij] = 0.0;
		}
	}

	curreffect = 0;
	currhandle = 0;

	// open file
	printf("opening file %s\n", fname);
	fp = fopen(fname, "r");
	// read line-by-line
	while ((readnum = getline(&line, &len, fp)) != -1)
	{
		printf("\n%s", line);
		if (line[0] == '#') continue; // allow for comments
		// (1/5) look for minutes
		ii = 0; // count in line
		index = 0; // count in buffer
		memset(buffer, 0, 128);
		while (line[ii] != ':')
		{
			buffer[index] = line[ii];
			ii++;
			index++;
		}
		currtime = 60.0 * atof(buffer); // add minutes
		
		// (2/5) look for seconds
		index = 0;
		ii++;
		memset(buffer, 0, 128);
		while (line[ii] != ' ')
		{
			buffer[index] = line[ii];
			ii++;
			index++;
		}
		currtime += atof(buffer); // add seconds
		
		// (3/5) determine effect or handle
		ii++;
		if (line[ii] == 'h')
		{
			// (4/5) figure out which handle it is and parse it
			memset(buffer, 0, 128);
			ii++;
			index = 0;
			while (line[ii] != ' ')
			{
				buffer[index] = line[ii];
				ii++;
				index++;
			}
			currhandle = atoi(buffer);
			printf("parsing handle %d\n", currhandle);
			ii++;
			parse_handle_key(line+ii, h+currhandle, currtime);
		} else if (line[ii] == 'e') {
			// search for next space
			while (line[ii] != ' ') ii++;
			ii++;
			// (4/5) use helper function to parse the rest
			printf("parsing effect %d\n", curreffect);
			parse_effect(line+ii, eff+curreffect, currtime);
			// load extra data into every effect
			eff[curreffect].str = st;
			curreffect++;
		} else {
			printf("WARNING: sequence not recognized!\n");
			exit(-1);
		}
	}
	if (line) free(line);
	fclose(fp);

	// conclude
	*numeffects = curreffect;
}

// helper function to parse a string and load data into a handle
void parse_handle_key (char *line, handle *h, float time)
{
	int ii, index, attrindex;
	char buffer[128];
	float val;

	printf("parsing key ");
	// create a new key
	memset(buffer, 0, 128);
	ii = 0;
	index = 0;
	while (line[ii] != ' ')
	{
		buffer[index] = line[ii];
		ii++;
		index++;
	}

	// match handle attribute using ifs
	printf("%s to ", buffer);
	if (strcmp(buffer, "fade") == 0)
	{
		printf("key 0");
		// parse single value
		ii++;
		val = atof(line+ii);
		printf(" with value %f ", val);
		// set key
		h->fade_keys[h->fade_numkeys].time = time;
		h->fade_keys[h->fade_numkeys].value = val;
		h->fade_numkeys++;
	} else if (strcmp(buffer, "color1") == 0) {
		printf("key 1");
		// parse 3 values
		ii++;
		index = 0;
		memset(buffer, 0, 128);
		while (line[ii] != ' ')
		{
			buffer[index] = line[ii];
			ii++;
			index++;
		}
		val = atof(buffer);
		printf(" with values %f ", val);
		h->color1_keys[0][h->color1_numkeys].time = time;
		h->color1_keys[0][h->color1_numkeys].value = val;
		ii++;
		index = 0;
		memset(buffer, 0, 128);
		while (line[ii] != ' ')
		{
			buffer[index] = line[ii];
			ii++;
			index++;
		}
		val = atof(buffer);
		printf("%f ", val);
		h->color1_keys[1][h->color1_numkeys].time = time;
		h->color1_keys[1][h->color1_numkeys].value = val;
		ii++;
		val = atof(line+ii);
		printf("%f ", val);
		h->color1_keys[2][h->color1_numkeys].time = time;
		h->color1_keys[2][h->color1_numkeys].value = val;
		h->color1_numkeys++;
	} else if (strcmp(buffer, "color2") == 0) {
		printf("key 2 ");
		// parse 3 values
		ii++;
		index = 0;
		memset(buffer, 0, 128);
		while (line[ii] != ' ')
		{
			buffer[index] = line[ii];
			ii++;
			index++;
		}
		val = atof(buffer);
		printf(" with values %f", val);
		h->color2_keys[0][h->color2_numkeys].time = time;
		h->color2_keys[0][h->color2_numkeys].value = val;
		ii++;
		index = 0;
		memset(buffer, 0, 128);
		while (line[ii] != ' ')
		{
			buffer[index] = line[ii];
			ii++;
			index++;
		}
		val = atof(buffer);
		printf("%f ", val);
		h->color2_keys[1][h->color2_numkeys].time = time;
		h->color2_keys[1][h->color2_numkeys].value = val;
		ii++;
		val = atof(line+ii);
		printf("%f ", val);
		h->color2_keys[2][h->color2_numkeys].time = time;
		h->color2_keys[2][h->color2_numkeys].value = val;
		h->color2_numkeys++;
	} else if (strcmp(buffer, "attr") == 0) {
		// figure out which attr to set
		printf("key 3");
		ii++;
		index = 0;
		memset(buffer, 0, 128);
		while (line[ii] != ' ')
		{
			buffer[index] = line[ii];
			ii++;
			index++;
		}
		attrindex = atoi(buffer);
		printf(" (attr %d)", attrindex);
		// get value
		ii++;
		val = atof(line+ii);
		printf(" with value %f", val);
		h->attr_keys[attrindex][h->attr_numkeys[attrindex]].value = val;
		printf(" [test %d %f] ", h->attr_numkeys[attrindex], h->attr_keys[attrindex][h->attr_numkeys[attrindex]].value);
		h->attr_keys[attrindex][h->attr_numkeys[attrindex]].time = time;
		h->attr_numkeys[attrindex]++;
	} else if (strcmp(buffer, "file") == 0) {
		printf("key 4");
		ii++;
		index = 0;
		memset(buffer, 0, 128);
		while (line[ii] != ' ' && line[ii] != '\n' && line[ii] != '\r')
		{
			buffer[index] = line[ii];
			ii++;
			index++;
		}
		strcpy(h->file, buffer);

		printf(" with value '%s'", h->file);
		h->preload = 1;
	} else {
		printf("UNKNOWN KEY");
		exit(-1);
	}

	printf(" at time %f\n" , time);
}

// helper function to parse a string and load data into an effect
// assume eff is allocated, but still set to default params
void parse_effect (char *line, effect *eff, float time)
{
	int ii, index, handlenum;
	char buffer[128];

	memset(buffer, 0, 128);
	
	printf("parsed effect ");
	// need to determine:
	// 1 - start time
	// 2 - function pointer
	// 3 - handle pointer
	eff->starttime = time;
	ii = 0;
	index = 0;
	while (line[ii] != ' ')
	{
		buffer[index] = line[ii];
		ii++;
		index++;
	}
	
	// match effect using a buncha ifs
	printf("%s to ", buffer);
	if (strcmp(buffer, "solid") == 0)
	{
		printf("effect 0");
		eff->run = &effect_solid;
	} else if (strcmp(buffer, "pulse") == 0) {
		printf("effect 1");
		eff->run = &effect_pulse;
	} else if (strcmp(buffer, "circle") == 0) {
		printf("effect 2");
		eff->run = &effect_circle;
	} else if (strcmp(buffer, "video") == 0) {
		printf("effect 3");
		eff->run = &effect_video;
	} else if (strcmp(buffer, "packet") == 0) {
		printf("effect 4");
		eff->run = &effect_packet;
	} else if (strcmp(buffer, "ring") == 0) {
		printf("effect 5");
		eff->run = &effect_ring;
	} else if (strcmp(buffer, "flicker") == 0) {
		printf("effect 6");
		eff->run = &effect_flicker;
	} else if (strcmp(buffer, "image") == 0) {
		printf("effect 7");
		eff->run = &effect_image;
	} else if (strcmp(buffer, "slp") == 0) {
		printf("effect 8");
		eff->run = &effect_slp;
	} else {
		printf("UNKNOWN EFFECT");
		exit(-1);
	}
	
	// parse handle number
	ii++;
	handlenum = atoi(line+ii);
	eff->handleindex = handlenum;
	printf(" attached to handle %d\n", handlenum);
}

/* notes
 * 
 * want to be able to line up a sequence of effects in a text file and have 
 * the code run them together. Not only do I need to be able to line up a
 * few effects in series, but I need to be able to modify the parameters of
 * the effects while they play and possibly mix effects together. Ideally,
 * the sequence file should look like this:
 *
 * # initial declarations, to do before sequence starts
 * # set up handles 
 * h1 color1 255 255 255
 * h1 color2 0 0 0
 * h1 duration 20
 * # sequence begin
 * # effects are given a handle and a start time
 * 0:00	effect swipe 1
 * # handle attributes are set to change at certain keyframes
 * 0:00 h1 color2 0 0 0
 * 0:10 h1 color2 20 0 0
 * 0:20 h1 color2 40 20 20
 * # need to make sequencer look ahead to linearly interpolate attributes
 * h2 duration 5
 * h2 color2 0 0 0
 * 0:30 effect fadeto h2
 * # try out a twinkling that fades in, changes color, and fades out
 *
 * 0:35 h3 duration 30
 * 0:35 h3 color1 0 0 0
 * 0:35 h3 color2 0 0 0
 * 0:35 h3 speed 1.0
 * # start effect
 * 0:35 effect twinkle 3
 * # add keyframe for color
 * 0:40 h3 color1 100 100 100
 * 0:40 h3 color2 0 0 0
 * 0:45 h3 color1 100 100 100
 * 0:45 h3 color2 0 0 0
 * 0:55 h3 color1 100 0 0
 * 0:55 h3 color2 0 20 20
 * 1:00 h3 color1 0 0 0
 * 1:00 h3 color2 0 20 20
 * 1:05 h3 color1 0 0 0
 * 1:05 h3 color2 0 0 0
 *
 *
 *
 *
 * each effect has a 'handle' struct that controls the timing, color, speed, etc
 * the parameters in this can be modified as the effect goes on in order to fade colors
 * or speed things up
 *
 * every attribute can be keyframed (time, value). when the effect is called to update the
 * pixel buffer, the sequencer must determine the appropriate attributes by searching for 
 * the nearest keyframes and interpolating between them.
 *
 * at each time step, the sequencer must do the following:
 * 1 - update current attributes in every handle based on keyframes
 * 2 - look for new effects to add to the 'currently-running' list
 * 3 - for each effect currently running, execute it and add the result to the pixel buffer
 * 4 - send the final pixel buffer out to the led drivers
 *
 * one of the more complicated effects is the video effect:
 * 0:00 h4 video something.avi
 * 2:30 effect video 4
 * the video attribute can not be keyframed, of course. since the sequencer will probably run
 * at a different framerate than the video, this could be complicated. it might be possible
 * to pre-load every necessary movie and keep each frame in memory. then the appropriate
 * frame can be grabbed by taking the nearest one to (currtime-starttime)*fps
 * */
