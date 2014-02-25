#include "header.h"

// TODO; figure how to handle local storage for an effect (like phases)

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

