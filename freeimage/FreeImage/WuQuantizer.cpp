///////////////////////////////////////////////////////////////////////
//	    C Implementation of Wu's Color Quantizer (v. 2)
//	    (see Graphics Gems vol. II, pp. 126-133)
//
// Author:	Xiaolin Wu
// Dept. of Computer Science
// Univ. of Western Ontario
// London, Ontario N6A 5B7
// wu@csd.uwo.ca
// 
// Algorithm: Greedy orthogonal bipartition of RGB space for variance
// 	   minimization aided by inclusion-exclusion tricks.
// 	   For speed no nearest neighbor search is done. Slightly
// 	   better performance can be expected by more sophisticated
// 	   but more expensive versions.
// 
// The author thanks Tom Lane at Tom_Lane@G.GP.CS.CMU.EDU for much of
// additional documentation and a cure to a previous bug.
// 
// Free to distribute, comments and suggestions are appreciated.
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// History
// -------
// July 2000: C++ Implementation of Wu's Color Quantizer
//            and adaptation for the FreeImage 2 Library
//            Author: Hervé Drolon (drolon@iut.univ-lehavre.fr)
///////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <memory.h> 

#include "Quantizers.h"
#include "FreeImage.h"

///////////////////////////////////////////////////////////////////////

// 3D array indexation

#define INDEX(r, g, b)	((r << 10) + (r << 6) + r + (g << 5) + g + b)

#define MAXCOLOR	256
#define	RED		2
#define	GREEN	1
#define BLUE	0


// Constructor / Destructor

WuQuantizer::WuQuantizer(FIBITMAP *dib)
{
	width = FreeImage_GetWidth(dib);
	height = FreeImage_GetHeight(dib);
	pitch = FreeImage_GetPitch(dib);
	m_dib = dib;

	gm2 = NULL;
	wt = mr = mg = mb = NULL;
	Qadd = NULL;

	// Allocate 3D arrays
	gm2 = (float*)malloc(33 * 33 * 33 * sizeof(float));
	wt = (int32*)malloc(33 * 33 * 33 * sizeof(int32));
	mr = (int32*)malloc(33 * 33 * 33 * sizeof(int32));
	mg = (int32*)malloc(33 * 33 * 33 * sizeof(int32));
	mb = (int32*)malloc(33 * 33 * 33 * sizeof(int32));

	// Allocate Qadd
	Qadd = (uint16 *)malloc(sizeof(uint16) * width * height);

	if(!gm2 || !wt || !mr || !mg || !mb || !Qadd)
	{
		if(gm2)	free(gm2);
		if(wt)	free(wt);
		if(mr)	free(mr);
		if(mg)	free(mg);
		if(mb)	free(mb);
		if(Qadd)  free(Qadd);
		throw "Not enough memory";
	}
	memset(gm2, 0, 35937 * sizeof(float));
	memset(wt, 0, 35937 * sizeof(int32));
	memset(mr, 0, 35937 * sizeof(int32));
	memset(mg, 0, 35937 * sizeof(int32));
	memset(mb, 0, 35937 * sizeof(int32));
	memset(Qadd, 0, sizeof(uint16) * width * height);
}

WuQuantizer::~WuQuantizer()
{
	if(gm2)	free(gm2);
	if(wt)	free(wt);
	if(mr)	free(mr);
	if(mg)	free(mg);
	if(mb)	free(mb);
	if(Qadd)  free(Qadd);
}


// Histogram is in elements 1..HISTSIZE along each axis,
// element 0 is for base or marginal value
// NB: these must start out 0!

// Build 3-D color histogram of counts, r/g/b, c^2
void WuQuantizer::Hist3D(int32 *vwt, int32 *vmr, int32 *vmg, int32 *vmb, float *m2)
{
	int ind;
	int inr, ing, inb, table[256];
	int i;
	uint16 y, x;

	for(i = 0; i < 256; i++)
		table[i] = i * i;

	for(y = 0; y < height; y++)
	{
		uint8 *bits = FreeImage_GetBits(m_dib) + (y * pitch);
		for(x = 0; x < width; x++)
		{
			inr = (bits[2] >> 3) + 1;
			ing = (bits[1] >> 3) + 1;
			inb = (bits[0] >> 3) + 1;
			Qadd[y*width + x] = ind = INDEX(inr, ing, inb);
			// [inr][ing][inb]
			vwt[ind]++;
			vmr[ind] += bits[2];
			vmg[ind] += bits[1];
			vmb[ind] += bits[0];
			m2[ind] += (float)(table[bits[2]] + table[bits[1]] + table[bits[0]]);
			bits += 3;
		}
	}
}


// At conclusion of the histogram step, we can interpret
// wt[r][g][b] = sum over voxel of P(c)
// mr[r][g][b] = sum over voxel of r*P(c)  ,  similarly for mg, mb
// m2[r][g][b] = sum over voxel of c^2*P(c)
// Actually each of these should be divided by 'ImageSize' to give the usual
// interpretation of P() as ranging from 0 to 1, but we needn't do that here.


// We now convert histogram into moments so that we can rapidly calculate
// the sums of the above quantities over any desired box.

// Compute cumulative moments
void WuQuantizer::M3D(int32 *vwt, int32 *vmr, int32 *vmg, int32 *vmb, float *m2)
{
	uint16 ind1, ind2;
	uint8 i, r, g, b;
	int32 line, line_r, line_g, line_b;
	int32 area[33], area_r[33], area_g[33], area_b[33];
	float line2, area2[33];

    for(r = 1; r <= 32; r++)
	{
		for(i = 0; i <= 32; i++)
		{
			area2[i] = 0;
			area[i] = area_r[i] = area_g[i] = area_b[i] = 0;
		}
		for(g = 1; g <= 32; g++)
		{
			line2 = 0;
			line = line_r = line_g = line_b = 0;
			for(b = 1; b <= 32; b++)
			{			 
				ind1 = INDEX(r, g, b); // [r][g][b]
				line += vwt[ind1];
				line_r += vmr[ind1]; 
				line_g += vmg[ind1]; 
				line_b += vmb[ind1];
				line2 += m2[ind1];
				area[b] += line;
				area_r[b] += line_r;
				area_g[b] += line_g;
				area_b[b] += line_b;
				area2[b] += line2;
				ind2 = ind1 - 1089; // [r-1][g][b]
				vwt[ind1] = vwt[ind2] + area[b];
				vmr[ind1] = vmr[ind2] + area_r[b];
				vmg[ind1] = vmg[ind2] + area_g[b];
				vmb[ind1] = vmb[ind2] + area_b[b];
				m2[ind1] = m2[ind2] + area2[b];
			}
		}
	}
}

// Compute sum over a box of any given statistic
int32 WuQuantizer::Vol( Box *cube, int32 *mmt ) 
{
    return( mmt[INDEX(cube->r1, cube->g1, cube->b1)] 
		  - mmt[INDEX(cube->r1, cube->g1, cube->b0)]
		  - mmt[INDEX(cube->r1, cube->g0, cube->b1)]
		  + mmt[INDEX(cube->r1, cube->g0, cube->b0)]
		  - mmt[INDEX(cube->r0, cube->g1, cube->b1)]
		  + mmt[INDEX(cube->r0, cube->g1, cube->b0)]
		  + mmt[INDEX(cube->r0, cube->g0, cube->b1)]
		  - mmt[INDEX(cube->r0, cube->g0, cube->b0)] );
}

// The next two routines allow a slightly more efficient calculation
// of Vol() for a proposed subbox of a given box.  The sum of Top()
// and Bottom() is the Vol() of a subbox split in the given direction
// and with the specified new upper bound.


// Compute part of Vol(cube, mmt) that doesn't depend on r1, g1, or b1
// (depending on dir)

int32 WuQuantizer::Bottom(Box *cube, uint8 dir, int32 *mmt)
{
    switch(dir)
	{
		case RED:
			return( - mmt[INDEX(cube->r0, cube->g1, cube->b1)]
				    + mmt[INDEX(cube->r0, cube->g1, cube->b0)]
					+ mmt[INDEX(cube->r0, cube->g0, cube->b1)]
					- mmt[INDEX(cube->r0, cube->g0, cube->b0)] );
			break;
		case GREEN:
			return( - mmt[INDEX(cube->r1, cube->g0, cube->b1)]
				    + mmt[INDEX(cube->r1, cube->g0, cube->b0)]
					+ mmt[INDEX(cube->r0, cube->g0, cube->b1)]
					- mmt[INDEX(cube->r0, cube->g0, cube->b0)] );
			break;
		case BLUE:
			return( - mmt[INDEX(cube->r1, cube->g1, cube->b0)]
				    + mmt[INDEX(cube->r1, cube->g0, cube->b0)]
					+ mmt[INDEX(cube->r0, cube->g1, cube->b0)]
					- mmt[INDEX(cube->r0, cube->g0, cube->b0)] );
			break;
	}

	return 0;
}


// Compute remainder of Vol(cube, mmt), substituting pos for
// r1, g1, or b1 (depending on dir)

int32 WuQuantizer::Top(Box *cube, uint8 dir, int pos, int32 *mmt)
{
    switch(dir)
	{
		case RED:
			return( mmt[INDEX(pos, cube->g1, cube->b1)] 
				   -mmt[INDEX(pos, cube->g1, cube->b0)]
				   -mmt[INDEX(pos, cube->g0, cube->b1)]
				   +mmt[INDEX(pos, cube->g0, cube->b0)] );
			break;
		case GREEN:
			return( mmt[INDEX(cube->r1, pos, cube->b1)] 
				   -mmt[INDEX(cube->r1, pos, cube->b0)]
				   -mmt[INDEX(cube->r0, pos, cube->b1)]
				   +mmt[INDEX(cube->r0, pos, cube->b0)] );
			break;
		case BLUE:
			return( mmt[INDEX(cube->r1, cube->g1, pos)]
				   -mmt[INDEX(cube->r1, cube->g0, pos)]
				   -mmt[INDEX(cube->r0, cube->g1, pos)]
				   +mmt[INDEX(cube->r0, cube->g0, pos)] );
			break;
	}

	return 0;
}

// Compute the weighted variance of a box 
// NB: as with the raw statistics, this is really the variance * ImageSize 

float
WuQuantizer::Var(Box *cube) {
    float dr = (float) Vol(cube, mr); 
    float dg = (float) Vol(cube, mg); 
    float db = (float) Vol(cube, mb);
    float xx =  gm2[INDEX(cube->r1, cube->g1, cube->b1)] 
			-gm2[INDEX(cube->r1, cube->g1, cube->b0)]
			 -gm2[INDEX(cube->r1, cube->g0, cube->b1)]
			 +gm2[INDEX(cube->r1, cube->g0, cube->b0)]
			 -gm2[INDEX(cube->r0, cube->g1, cube->b1)]
			 +gm2[INDEX(cube->r0, cube->g1, cube->b0)]
			 +gm2[INDEX(cube->r0, cube->g0, cube->b1)]
			 -gm2[INDEX(cube->r0, cube->g0, cube->b0)];

    return (xx - (dr*dr+dg*dg+db*db)/(float)Vol(cube,wt));    
}

// We want to minimize the sum of the variances of two subboxes.
// The sum(c^2) terms can be ignored since their sum over both subboxes
// is the same (the sum for the whole box) no matter where we split.
// The remaining terms have a minus sign in the variance formula,
// so we drop the minus sign and MAXIMIZE the sum of the two terms.

float
WuQuantizer::Maximize(Box *cube, uint8 dir, int first, int last , int *cut, int32 whole_r, int32 whole_g, int32 whole_b, int32 whole_w) {
	int32 half_r, half_g, half_b, half_w;
	int i;
	float temp;

    int32 base_r = Bottom(cube, dir, mr);
    int32 base_g = Bottom(cube, dir, mg);
    int32 base_b = Bottom(cube, dir, mb);
    int32 base_w = Bottom(cube, dir, wt);

    float max = 0.0;

    *cut = -1;

    for (i = first; i < last; i++) {
		half_r = base_r + Top(cube, dir, i, mr);
		half_g = base_g + Top(cube, dir, i, mg);
		half_b = base_b + Top(cube, dir, i, mb);
		half_w = base_w + Top(cube, dir, i, wt);

        // now half_x is sum over lower half of box, if split at i

		if (half_w == 0) {		// subbox could be empty of pixels!
			continue;			// never split into an empty box
		} else {
			temp = ((float)half_r*half_r + (float)half_g*half_g + (float)half_b*half_b)/half_w;
		}

		half_r = whole_r - half_r;
		half_g = whole_g - half_g;
		half_b = whole_b - half_b;
		half_w = whole_w - half_w;

        if (half_w == 0) {		// subbox could be empty of pixels!
			continue;			// never split into an empty box
		} else {
			temp += ((float)half_r*half_r + (float)half_g*half_g + (float)half_b*half_b)/half_w;
		}

    	if (temp > max) {
			max=temp;
			*cut=i;
		}
    }

    return max;
}

bool
WuQuantizer::Cut(Box *set1, Box *set2) {
	uint8 dir;
	int cutr, cutg, cutb;

    int32 whole_r = Vol(set1, mr);
    int32 whole_g = Vol(set1, mg);
    int32 whole_b = Vol(set1, mb);
    int32 whole_w = Vol(set1, wt);

    float maxr = Maximize(set1, RED, set1->r0+1, set1->r1, &cutr, whole_r, whole_g, whole_b, whole_w);    
	float maxg = Maximize(set1, GREEN, set1->g0+1, set1->g1, &cutg, whole_r, whole_g, whole_b, whole_w);    
	float maxb = Maximize(set1, BLUE, set1->b0+1, set1->b1, &cutb, whole_r, whole_g, whole_b, whole_w);

    if ((maxr >= maxg) && (maxr >= maxb)) {
		dir = RED;

		if (cutr < 0) {
			return false; // can't split the box
		}
    } else if ((maxg >= maxr) && (maxg>=maxb)) {
		dir = GREEN;
	} else {
		dir = BLUE;
	}

	set2->r1 = set1->r1;
    set2->g1 = set1->g1;
    set2->b1 = set1->b1;

    switch (dir) {
		case RED:
			set2->r0 = set1->r1 = cutr;
			set2->g0 = set1->g0;
			set2->b0 = set1->b0;
			break;

		case GREEN:
			set2->g0 = set1->g1 = cutg;
			set2->r0 = set1->r0;
			set2->b0 = set1->b0;
			break;

		case BLUE:
			set2->b0 = set1->b1 = cutb;
			set2->r0 = set1->r0;
			set2->g0 = set1->g0;
			break;
    }

    set1->vol = (set1->r1-set1->r0)*(set1->g1-set1->g0)*(set1->b1-set1->b0);
    set2->vol = (set2->r1-set2->r0)*(set2->g1-set2->g0)*(set2->b1-set2->b0);

    return true;
}


void
WuQuantizer::Mark(Box *cube, int label, uint8 *tag) {
    for (int r = cube->r0 + 1; r <= cube->r1; r++) {
		for (int g = cube->g0 + 1; g <= cube->g1; g++) {
			for (int b = cube->b0 + 1; b <= cube->b1; b++) {
				tag[INDEX(r, g, b)] = label;
			}
		}
	}
}

// Wu Quantization algorithm

FIBITMAP *
WuQuantizer::Quantize() {
	uint8 *tag = NULL;

	try {
		int PalSize = 256;	// Color look-up table size
		Box	cube[MAXCOLOR];
		int	next;
		int32 i, weight;
		int k;
		float vv[MAXCOLOR], temp;
		
		// Compute 3D histogram

		Hist3D(wt, mr, mg, mb, gm2);

		// Compute moments

		M3D(wt, mr, mg, mb, gm2);

		cube[0].r0 = cube[0].g0 = cube[0].b0 = 0;
		cube[0].r1 = cube[0].g1 = cube[0].b1 = 32;
		next = 0;

		for (i = 1; i < PalSize; i++) {
			if(Cut(&cube[next], &cube[i])) {
				// volume test ensures we won't try to cut one-cell box
				vv[next] = (cube[next].vol > 1) ? Var(&cube[next]) : 0;
				vv[i] = (cube[i].vol > 1) ? Var(&cube[i]) : 0;
			} else {
				  vv[next] = 0.0;   // don't try to split this box again
				  i--;              // didn't create box i
			}

			next = 0; temp = vv[0];

			for (k = 1; k <= i; k++) {
				if (vv[k] > temp) {
					temp = vv[k]; next = k;
				}
			}

			if (temp <= 0.0) {
				  PalSize = i + 1;

				  // Error: "Only got 'PalSize' boxes"

				  break;
			}
		}

		// Partition done

		// the space for array gm2 can be freed now

		free(gm2);

		gm2 = NULL;

		// Allocate a new dib

		FIBITMAP *new_dib = FreeImage_Allocate(width, height, 8);

		if (new_dib == NULL) {
			throw "Not enough memory";
		}

		// create an optimized palette

		RGBQUAD *new_pal = FreeImage_GetPalette(new_dib);

		tag = (uint8*) malloc(33 * 33 * 33 * sizeof(uint8));

		if (tag == NULL) {
			throw "Not enough memory";
		}

		for (k = 0; k < PalSize; k++) {
			Mark(&cube[k], k, tag);
			weight = Vol(&cube[k], wt);

			if (weight) {
				new_pal[k].rgbRed	= (uint8)(Vol(&cube[k], mr) / weight);
				new_pal[k].rgbGreen = (uint8)(Vol(&cube[k], mg) / weight);
				new_pal[k].rgbBlue	= (uint8)(Vol(&cube[k], mb) / weight);
			} else {
				// Error: bogus box 'k'

				new_pal[k].rgbRed = new_pal[k].rgbGreen = new_pal[k].rgbBlue = 0;		
			}
		}

		int npitch = FreeImage_GetPitch(new_dib);

		for (uint16 y = 0; y < height; y++) {
			uint8 *new_bits = FreeImage_GetBits(new_dib) + (y * npitch);

			for (uint16 x = 0; x < width; x++) {
				new_bits[x] = tag[Qadd[y*width + x]];
			}
		}

		// output 'new_pal' as color look-up table contents,
		// 'new_bits' as the quantized image (array of table addresses).

		free(tag);

		return (FIBITMAP*) new_dib;
	} catch(...) {
		free(tag);
	}

	return NULL;
}
