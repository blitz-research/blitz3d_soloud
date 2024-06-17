// =============================================================
// Quantizer objects and functions
//
// Design and implementation by:
// - Hervé Drolon <drolon@infonie.fr>
//
// This file is part of FreeImage
//
// COVERED CODE IS PROVIDED UNDER THIS LICENSE ON AN "AS IS" BASIS, WITHOUT WARRANTY
// OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, WITHOUT LIMITATION, WARRANTIES
// THAT THE COVERED CODE IS FREE OF DEFECTS, MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE
// OR NON-INFRINGING. THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE COVERED
// CODE IS WITH YOU. SHOULD ANY COVERED CODE PROVE DEFECTIVE IN ANY RESPECT, YOU (NOT
// THE INITIAL DEVELOPER OR ANY OTHER CONTRIBUTOR) ASSUME THE COST OF ANY NECESSARY
// SERVICING, REPAIR OR CORRECTION. THIS DISCLAIMER OF WARRANTY CONSTITUTES AN ESSENTIAL
// PART OF THIS LICENSE. NO USE OF ANY COVERED CODE IS AUTHORIZED HEREUNDER EXCEPT UNDER
// THIS DISCLAIMER.
//
// Use at your own risk!
// =============================================================

// Xiaolin Wu color quantization algorithm
////////////////////////////////////////////////////////////////

#include "FreeImage.h"

////////////////////////////////////////////////////////////////

typedef unsigned char	uint8;
typedef unsigned short	uint16;
typedef unsigned long	uint32;
typedef long			int32;

typedef struct {
    int r0;			 // min value, exclusive
    int r1;			 // max value, inclusive
    int g0;  
    int g1;  
    int b0;  
    int b1;
    int vol;
} Box;


class WuQuantizer
{
  protected:
    float *gm2;
	int32 *wt, *mr, *mg, *mb;
	uint16 *Qadd;

	// DIB data
	uint16 width, height;
	uint16 pitch;
	FIBITMAP *m_dib;

  protected:
    void Hist3D(int32 *vwt, int32 *vmr, int32 *vmg, int32 *vmb, float *m2) ;
	void M3D(int32 *vwt, int32 *vmr, int32 *vmg, int32 *vmb, float *m2);
	int32 Vol(Box *cube, int32 *mmt);
	int32 Bottom(Box *cube, uint8 dir, int32 *mmt);
	int32 Top(Box *cube, uint8 dir, int pos, int32 *mmt);
	float Var(Box *cube);
	float Maximize(Box *cube, uint8 dir, int first, int last , int *cut,
				   int32 whole_r, int32 whole_g, int32 whole_b, int32 whole_w);
	bool Cut(Box *set1, Box *set2);
	void Mark(Box *cube, int label, uint8 *tag);

  public:
	// Constructor - Input parameter: DIB 24-bit to be quantized
    WuQuantizer(FIBITMAP *dib);
	// Destructor
	~WuQuantizer();
	// Quantizer - Return value: quantized 8-bit (color palette) DIB
	FIBITMAP* Quantize();
};

// NEUQUANT Neural-Net quantization algorithm by Anthony Dekker
////////////////////////////////////////////////////////////////

// Input parameters: 
// - void* dib: DIB 24-bit to be quantized
// - int sampling: a sampling factor in range 1..30
//                 1 => slower, 30 => faster. Default value is 15
// Return value: 
// - NULL  if the DIB is not valid or if it's not a 24-bit DIB
// - the quantized 8-bit (color palette) DIB otherwise

FIBITMAP *NNQuantizer(FIBITMAP *dib, int sampling = 15);

