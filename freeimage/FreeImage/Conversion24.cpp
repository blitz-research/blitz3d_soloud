// ==========================================================
// Bitmap conversion routines
//
// Design and implementation by
// - Floris van den Berg (flvdberg@wxs.nl)
// - Dale Larson (dlarson@norsesoft.com)
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
#include "Utilities.h"

// ----------------------------------------------------------
//  internal conversions X to 24 bits
// ----------------------------------------------------------

void DLL_CALLCONV
FreeImage_ConvertLine1To24(BYTE *target, BYTE *source, int width_in_pixels, RGBQUAD *palette) {
	for (int cols = 0; cols < width_in_pixels; cols++) {
		BYTE index = (source[cols >> 3] & (0x80 >> (cols & 0x07))) != 0 ? 1 : 0;

		target[0] = palette[index].rgbBlue;
		target[1] = palette[index].rgbGreen;
		target[2] = palette[index].rgbRed;

		target += 3;
	}
}

void DLL_CALLCONV
FreeImage_ConvertLine4To24(BYTE *target, BYTE *source, int width_in_pixels, RGBQUAD *palette) {
	BOOL low_nibble = FALSE;
	int x = 0;

	for (int cols = 0; cols < width_in_pixels; ++cols ) {
		if (low_nibble) {
			target[0] = palette[LOWNIBBLE(source[x])].rgbBlue;
			target[1] = palette[LOWNIBBLE(source[x])].rgbGreen;
			target[2] = palette[LOWNIBBLE(source[x])].rgbRed;

			x++;
		} else {
			target[0] = palette[HINIBBLE(source[x]) >> 4].rgbBlue;
			target[1] = palette[HINIBBLE(source[x]) >> 4].rgbGreen;
			target[2] = palette[HINIBBLE(source[x]) >> 4].rgbRed;
		}

		low_nibble = !low_nibble;

		target += 3;
	}
}

void DLL_CALLCONV
FreeImage_ConvertLine8To24(BYTE *target, BYTE *source, int width_in_pixels, RGBQUAD *palette) {
	for (int cols = 0; cols < width_in_pixels; cols++) {
		target[0] = palette[source[cols]].rgbBlue;
		target[1] = palette[source[cols]].rgbGreen;
		target[2] = palette[source[cols]].rgbRed;

		target += 3;
	}
}

void DLL_CALLCONV
FreeImage_ConvertLine16To24_555(BYTE *target, BYTE *source, int width_in_pixels) {
	WORD *bits = (WORD *)source;

	for (int cols = 0; cols < width_in_pixels; cols++) {
		target[2] = (((bits[cols] & 0x7C00) >> 10) * 0xFF) / 0x1F;
		target[1] = (((bits[cols] & 0x3E0) >> 5) * 0xFF) / 0x1F;
		target[0] = ((bits[cols] & 0x1F) * 0xFF) / 0x1F;

		target += 3;
	}
}

void DLL_CALLCONV
FreeImage_ConvertLine16To24_565(BYTE *target, BYTE *source, int width_in_pixels) {
	WORD *bits = (WORD *)source;

	for (int cols = 0; cols < width_in_pixels; cols++) {
		target[2] = (((bits[cols] & 0xF800) >> 11) * 0xFF) / 0x1F;
		target[1] = (((bits[cols] & 0x7E0) >> 5) * 0xFF) / 0x3F;
		target[0] = ((bits[cols] & 0x1F) * 0xFF) / 0x1F;

		target += 3;
	}
}

void DLL_CALLCONV
FreeImage_ConvertLine32To24(BYTE *target, BYTE *source, int width_in_pixels) {
	for (int cols = 0; cols < width_in_pixels; cols++) {
		target[0] = source[0];
		target[1] = source[1];
		target[2] = source[2];

		target += 3;
		source += 4;
	}
}

// ----------------------------------------------------------
//   smart convert X to 24 bits
// ----------------------------------------------------------

FIBITMAP * DLL_CALLCONV
FreeImage_ConvertTo24Bits(FIBITMAP *dib) {
	if (dib != NULL) {
		int width = FreeImage_GetWidth(dib);
		int height = FreeImage_GetHeight(dib);

		switch (FreeImage_GetBPP(dib)) {
			case 1 :
			{
				FIBITMAP *new_dib = FreeImage_Allocate(width, height, 24, 0xFF, 0xFF00, 0xFF0000);

				if (new_dib != NULL)
					for (int rows = 0; rows < height; rows++)
						FreeImage_ConvertLine1To24(FreeImage_GetScanLine(new_dib, rows), FreeImage_GetScanLine(dib, rows), width, FreeImage_GetPalette(dib));					
				
				return new_dib;
			}

			case 4 :
			{
				FIBITMAP *new_dib = FreeImage_Allocate(width, height, 24, 0xFF, 0xFF00, 0xFF0000);

				if (new_dib != NULL)
					for (int rows = 0; rows < height; rows++)
						FreeImage_ConvertLine4To24(FreeImage_GetScanLine(new_dib, rows), FreeImage_GetScanLine(dib, rows), width, FreeImage_GetPalette(dib));
			
				return new_dib;
			}
				
			case 8 :
			{
				FIBITMAP *new_dib = FreeImage_Allocate(width, height, 24, 0xFF, 0xFF00, 0xFF0000);

				if (new_dib != NULL)
					for (int rows = 0; rows < height; rows++)
						FreeImage_ConvertLine8To24(FreeImage_GetScanLine(new_dib, rows), FreeImage_GetScanLine(dib, rows), width, FreeImage_GetPalette(dib));
									
				return new_dib;
			}

			case 16 :
			{
				FIBITMAP *new_dib = FreeImage_Allocate(width, height, 24, 0xFF, 0xFF00, 0xFF0000);

				if (new_dib != NULL) {
					for (int rows = 0; rows < height; rows++) {
						if ((FreeImage_GetRedMask(dib) == 0x1F) && (FreeImage_GetGreenMask(dib) == 0x3E0) && (FreeImage_GetBlueMask(dib) == 0x7C00)) {
							FreeImage_ConvertLine16To24_555(FreeImage_GetScanLine(new_dib, rows), FreeImage_GetScanLine(dib, rows), width);
						} else {
							FreeImage_ConvertLine16To24_565(FreeImage_GetScanLine(new_dib, rows), FreeImage_GetScanLine(dib, rows), width);
						}
					}
				}

				return new_dib;
			}

			case 32 :
			{
				FIBITMAP *new_dib = FreeImage_Allocate(width, height, 24, 0xFF, 0xFF00, 0xFF0000);

				if (new_dib != NULL)
					for (int rows = 0; rows < height; rows++)
						FreeImage_ConvertLine32To24(FreeImage_GetScanLine(new_dib, rows), FreeImage_GetScanLine(dib, rows), width);
				
				return new_dib;
			}
		}
	}

	return NULL;
}
