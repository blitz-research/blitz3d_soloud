// ==========================================================
// Bitmap conversion routines
//
// Design and implementation by
// - Floris van den Berg (flvdberg@wxs.nl)
// - Hervé Drolon (drolon@iut.univ-lehavre.fr)
// - Jani Kajala (janik@remedy.fi)
//
// This file is part of FreeImage 2
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
// ==========================================================

#include "FreeImage.h"

// ----------------------------------------------------------

#define GREY1(r,g,b) (BYTE)(((WORD)r*77 + (WORD)g*150 + (WORD)b*29) >> 8)	// .299R + .587G + .114B
#define GREY2(r,g,b) (BYTE)(((WORD)r*169 + (WORD)g*256 + (WORD)b*87) >> 9)	// .33R + 0.5G + .17B

// ----------------------------------------------------------
//  internal conversions X to 8 bits
// ----------------------------------------------------------

void DLL_CALLCONV
FreeImage_ConvertLine1To8(BYTE *target, BYTE *source, int width_in_pixels) {
	for (int cols = 0; cols < width_in_pixels; cols++)
		target[cols] = (source[cols >> 3] & (0x80 >> (cols & 0x07))) != 0;	
}

void DLL_CALLCONV
FreeImage_ConvertLine4To8(BYTE *target, BYTE *source, int width_in_pixels) {
	int count_new = 0;
	int count_org = 0;
	BOOL hinibble = TRUE;

	while (count_new < width_in_pixels) {
		if (hinibble) {
			target[count_new] = (source[count_org] & 0xf0) >> 4;
		} else {
			target[count_new] = (source[count_org] & 0x0f);

			count_org++;
		}

		hinibble = !hinibble;

		count_new++;
	}
}

void DLL_CALLCONV
FreeImage_ConvertLine16To8_555(BYTE *target, BYTE *source, int width_in_pixels) {
	WORD *bits = (WORD *)source;

	for (int cols = 0; cols < width_in_pixels; cols++) {
		target[cols] = GREY1((((bits[cols] & 0x7C00) >> 10) * 0xFF) / 0x1F,
			                (((bits[cols] & 0x3E0) >> 5) * 0xFF) / 0x1F,
							((bits[cols] & 0x1F) * 0xFF) / 0x1F);
	}
}

void DLL_CALLCONV
FreeImage_ConvertLine16To8_565(BYTE *target, BYTE *source, int width_in_pixels) {
	WORD *bits = (WORD *)source;

	for (int cols = 0; cols < width_in_pixels; cols++)
		target[cols] = GREY1((((bits[cols] & 0xF800) >> 11) * 0xFF) / 0x1F,
			        (((bits[cols] & 0x7E0) >> 5) * 0xFF) / 0x3F,
					((bits[cols] & 0x1F) * 0xFF) / 0x1F);	
}

void DLL_CALLCONV
FreeImage_ConvertLine24To8(BYTE *target, BYTE *source, int width_in_pixels) {
	for (int cols = 0; cols < width_in_pixels; cols++) {
		target[cols] = GREY1(source[2], source[1], source[0]);

		source += 3;
	}
}

void DLL_CALLCONV
FreeImage_ConvertLine32To8(BYTE *target, BYTE *source, int width_in_pixels) {
	for (int cols = 0; cols < width_in_pixels; cols++) {
		target[cols] = GREY1(source[2], source[1], source[0]);

		source += 4;
	}
}

// ----------------------------------------------------------
//   smart convert X to 8 bits
// ----------------------------------------------------------

FIBITMAP * DLL_CALLCONV
FreeImage_ConvertTo8Bits(FIBITMAP *dib) {
	if(dib) {
		int width  = FreeImage_GetWidth(dib);
		int height = FreeImage_GetHeight(dib);

		switch(FreeImage_GetBPP(dib)) {
			case 1:
			{
				FIBITMAP *new_dib = FreeImage_Allocate(width, height, 8);

				if (new_dib != NULL) {
					// Copy the palette

					RGBQUAD *new_pal = FreeImage_GetPalette(new_dib);
					RGBQUAD *old_pal = FreeImage_GetPalette(dib);

					for (int i = 0; i < 2; i++) {
						new_pal[i].rgbRed	= old_pal[i].rgbRed;
						new_pal[i].rgbGreen = old_pal[i].rgbGreen;
						new_pal[i].rgbBlue	= old_pal[i].rgbBlue;
					}

					// Expand and copy the bitmap data

					for (int rows = 0; rows < height; rows++)
						FreeImage_ConvertLine1To8(FreeImage_GetScanLine(new_dib, rows), FreeImage_GetScanLine(dib, rows), width);
					
				}

				return new_dib;
			}

			case 4 :
			{
				FIBITMAP *new_dib = FreeImage_Allocate(width, height, 8);

				if (new_dib != NULL) {
					// Copy the palette

					RGBQUAD *new_pal = FreeImage_GetPalette(new_dib);
					RGBQUAD *old_pal = FreeImage_GetPalette(dib);

					for (int i = 0; i < 16; i++) {
						new_pal[i].rgbRed	= old_pal[i].rgbRed;
						new_pal[i].rgbGreen = old_pal[i].rgbGreen;
						new_pal[i].rgbBlue	= old_pal[i].rgbBlue;
					}

					// Expand and copy the bitmap data

					for (int rows = 0; rows < height; rows++)
						FreeImage_ConvertLine4To8(FreeImage_GetScanLine(new_dib, rows), FreeImage_GetScanLine(dib, rows), width);					
				}

				return new_dib;
			}

			case 16 :
			{
				FIBITMAP *new_dib = FreeImage_Allocate(width, height, 8);

				if (new_dib != NULL) {
					// Build a greyscale palette

					RGBQUAD *new_pal = FreeImage_GetPalette(new_dib);

					for (int i = 0; i < 256; i++) {
						new_pal[i].rgbRed	= i;
						new_pal[i].rgbGreen = i;
						new_pal[i].rgbBlue	= i;
					}

					// Expand and copy the bitmap data

					for (int rows = 0; rows < height; rows++) {
						if ((FreeImage_GetRedMask(dib) == 0x1F) && (FreeImage_GetGreenMask(dib) == 0x3E0) && (FreeImage_GetBlueMask(dib) == 0x7C00)) {
							FreeImage_ConvertLine16To8_555(FreeImage_GetScanLine(new_dib, rows), FreeImage_GetScanLine(dib, rows), width);
						} else {
							FreeImage_ConvertLine16To8_565(FreeImage_GetScanLine(new_dib, rows), FreeImage_GetScanLine(dib, rows), width);
						}
					}
					
					return new_dib;
				}

				return NULL;
			}

			case 24 :
			{
				FIBITMAP *new_dib = FreeImage_Allocate(width, height, 8);

				if (new_dib != NULL) {
					// Build a greyscale palette

					RGBQUAD *new_pal = FreeImage_GetPalette(new_dib);

					for (int i = 0; i < 256; i++) {
						new_pal[i].rgbRed	= i;
						new_pal[i].rgbGreen = i;
						new_pal[i].rgbBlue	= i;
					}

					// Expand and copy the bitmap data

					for (int rows = 0; rows < height; rows++)
						FreeImage_ConvertLine24To8(FreeImage_GetScanLine(new_dib, rows), FreeImage_GetScanLine(dib, rows), width);					
				}

				return new_dib;
			}

			case 32 :
			{
				FIBITMAP *new_dib = FreeImage_Allocate(width, height, 8);

				if (new_dib != NULL) {
					// Build a greyscale palette

					RGBQUAD *new_pal = FreeImage_GetPalette(new_dib);

					for (int i = 0; i < 256; i++) {
						new_pal[i].rgbRed	= i;
						new_pal[i].rgbGreen = i;
						new_pal[i].rgbBlue	= i;
					}

					// Expand and copy the bitmap data

					for (int rows = 0; rows < height; rows++)
						FreeImage_ConvertLine32To8(FreeImage_GetScanLine(new_dib, rows), FreeImage_GetScanLine(dib, rows), width);
				}
				
				return new_dib;
			}
		}
	}

	return NULL;
}
