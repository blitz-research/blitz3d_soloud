// ==========================================================
// FreeImage implementation
//
// Design and implementation by
// - Floris van den Berg (flvdberg@wxs.nl)
// - Hervé Drolon (drolon@iut.univ-lehavre.fr)
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

#pragma warning (disable : 4786)

#include <stdlib.h>

#include "FreeImage.h"
#include "FreeImageIO.h"
#include "Utilities.h"

// ----------------------------------------------------------

FI_STRUCT (FREEIMAGEHEADER) {
	unsigned red_mask;
	unsigned green_mask;
	unsigned blue_mask;
	BOOL transparent;
	BYTE transparency_count;
	BYTE transparent_table[256];
	BYTE filler[3];
};

// ----------------------------------------------------------

FIBITMAP * DLL_CALLCONV
FreeImage_Allocate(int width, int height, int bpp, unsigned red_mask, unsigned green_mask, unsigned blue_mask) {
	FIBITMAP *bitmap = (FIBITMAP *)malloc(sizeof(FIBITMAP));

	if (bitmap != NULL) {
		height = abs(height);

		int dib_size = sizeof(FREEIMAGEHEADER) + sizeof(BITMAPINFOHEADER);
		dib_size += sizeof(RGBQUAD) * CalculateUsedColors(bpp);
		dib_size += CalculatePitch(CalculateLine(width, bpp)) * height;

		bitmap->data = (BYTE *)malloc(dib_size);

		if (bitmap->data != NULL) {
			memset(bitmap->data, 0, dib_size);

			// write out the FREEIMAGEHEADER

			FREEIMAGEHEADER *fih    = (FREEIMAGEHEADER *)bitmap->data;
			fih->red_mask           = red_mask;
			fih->green_mask         = green_mask;
			fih->blue_mask          = blue_mask;
			fih->transparent        = FALSE;
			fih->transparency_count = 0;

			// write out the BITMAPINFOHEADER

			BITMAPINFOHEADER *bih   = FreeImage_GetInfoHeader(bitmap);
			bih->biSize             = sizeof(BITMAPINFOHEADER);
			bih->biWidth            = width;
			bih->biHeight           = height;
			bih->biPlanes           = 1;
			bih->biCompression      = 0;
			bih->biBitCount         = bpp;
			bih->biClrUsed          = CalculateUsedColors(bpp);
			bih->biClrImportant     = bih->biClrUsed;

			return bitmap;
		} else {
			free(bitmap);
		}
	}

	return NULL;
}

void DLL_CALLCONV
FreeImage_Free(FIBITMAP *dib) {
	if (dib != NULL) {
		if (dib->data != NULL)
			free(dib->data);

		free(dib);
	}
}

void DLL_CALLCONV
FreeImage_Unload(FIBITMAP *dib) {
	FreeImage_Free(dib);
}

// ----------------------------------------------------------

FREE_IMAGE_COLOR_TYPE DLL_CALLCONV
FreeImage_GetColorType(FIBITMAP *dib) {
	RGBQUAD *rgb;

	switch (FreeImage_GetBPP(dib)) {
		case 1:
		{
			rgb = FreeImage_GetPalette(dib);

			if ((rgb->rgbRed == 0) && (rgb->rgbGreen == 0) && (rgb->rgbBlue == 0)) {
				rgb++;

				if ((rgb->rgbRed == 255) && (rgb->rgbGreen == 255) && (rgb->rgbBlue == 255))
					return FIC_MINISBLACK;				
			}

			if ((rgb->rgbRed == 255) && (rgb->rgbGreen == 255) && (rgb->rgbBlue == 255)) {
				rgb++;

				if ((rgb->rgbRed == 0) && (rgb->rgbGreen == 0) && (rgb->rgbBlue == 0))
					return FIC_MINISWHITE;				
			}

			return FIC_PALETTE;
		}

		case 4:
		case 8:	// Check if the DIB has a color or a greyscale palette
		{
			rgb = FreeImage_GetPalette(dib);

			for (unsigned i = 0; i < FreeImage_GetColorsUsed(dib); i++) {
				if ((rgb->rgbRed != rgb->rgbGreen) || (rgb->rgbRed != rgb->rgbBlue))
					return FIC_PALETTE;		

				// The DIB has a color palette if the greyscale isn't a linear ramp

				if (rgb->rgbRed != i)
					return FIC_PALETTE;				

				rgb++;
			}

			return FIC_MINISBLACK;
		}
		
		case 16:
		case 24:
			return FIC_RGB;

		case 32:
		{
			for (unsigned y = 0; y < FreeImage_GetHeight(dib); ++y) {
				rgb = (RGBQUAD *)FreeImage_GetScanLine(dib, y);

				for (unsigned x = 0; x < FreeImage_GetWidth(dib); ++x)
					if (rgb[x].rgbReserved != 0)
						return FIC_RGBALPHA;			
			}

			return FIC_RGB;
		}
				
		default :
			return FIC_MINISBLACK;
	}
}

// ----------------------------------------------------------

unsigned DLL_CALLCONV
FreeImage_GetRedMask(FIBITMAP *dib) {
	return dib ? ((FREEIMAGEHEADER *)dib->data)->red_mask : 0;
}

unsigned DLL_CALLCONV
FreeImage_GetGreenMask(FIBITMAP *dib) {
	return dib ? ((FREEIMAGEHEADER *)dib->data)->green_mask : 0;
}

unsigned DLL_CALLCONV
FreeImage_GetBlueMask(FIBITMAP *dib) {
	return dib ? ((FREEIMAGEHEADER *)dib->data)->blue_mask : 0;
}

BOOL DLL_CALLCONV
FreeImage_IsTransparent(FIBITMAP *dib) {
	return (dib != NULL) ? ((FREEIMAGEHEADER *)dib->data)->transparent : FALSE;
}

BYTE * DLL_CALLCONV
FreeImage_GetTransparencyTable(FIBITMAP *dib) {
	return dib ? ((FREEIMAGEHEADER *)dib->data)->transparent_table : NULL;
}

void DLL_CALLCONV
FreeImage_SetTransparent(FIBITMAP *dib, BOOL enabled) {
	if (dib) {
		if ((FreeImage_GetBPP(dib) == 8) || (FreeImage_GetBPP(dib) == 32)) {
			((FREEIMAGEHEADER *)dib->data)->transparent = enabled;
		} else {
			((FREEIMAGEHEADER *)dib->data)->transparent = FALSE;
		}
	}
}

unsigned DLL_CALLCONV
FreeImage_GetTransparencyCount(FIBITMAP *dib) {
	return dib ? ((FREEIMAGEHEADER *)dib->data)->transparency_count : 0;
}

void DLL_CALLCONV
FreeImage_SetTransparencyTable(FIBITMAP *dib, BYTE *table, BYTE count) {
	if (dib) {
		if (FreeImage_GetBPP(dib) == 8) {
			((FREEIMAGEHEADER *)dib->data)->transparent = TRUE;
			((FREEIMAGEHEADER *)dib->data)->transparency_count = count;

			if (table != NULL) {
				memcpy(((FREEIMAGEHEADER *)dib->data)->transparent_table, table, count);
			} else {
				memset(((FREEIMAGEHEADER *)dib->data)->transparent_table, 0xff, count);
			}
		} else {
			((FREEIMAGEHEADER *)dib->data)->transparency_count = 0;
		}
	}
}

// ----------------------------------------------------------

unsigned DLL_CALLCONV
FreeImage_GetWidth(FIBITMAP *dib) {
	return dib ? FreeImage_GetInfoHeader(dib)->biWidth : 0;
}

unsigned DLL_CALLCONV
FreeImage_GetHeight(FIBITMAP *dib) {
	return (dib) ? FreeImage_GetInfoHeader(dib)->biHeight : 0;
}

unsigned DLL_CALLCONV
FreeImage_GetBPP(FIBITMAP *dib) {
	return dib ? FreeImage_GetInfoHeader(dib)->biBitCount : 0;
}

unsigned DLL_CALLCONV
FreeImage_GetLine(FIBITMAP *dib) {
	return dib ? ((FreeImage_GetWidth(dib) * FreeImage_GetBPP(dib)) + 7) / 8 : 0;
}

unsigned DLL_CALLCONV
FreeImage_GetPitch(FIBITMAP *dib) {
	return dib ? FreeImage_GetLine(dib) + 3 & ~3 : 0;
}

unsigned DLL_CALLCONV
FreeImage_GetColorsUsed(FIBITMAP *dib) {
	return dib ? FreeImage_GetInfoHeader(dib)->biClrUsed : 0;
}

BYTE * DLL_CALLCONV
FreeImage_GetBits(FIBITMAP *dib) {
	return dib ? ((BYTE *)FreeImage_GetInfoHeader(dib) + sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * FreeImage_GetInfoHeader(dib)->biClrUsed) : NULL;
}

BYTE * DLL_CALLCONV
FreeImage_GetBitsRowCol(FIBITMAP *dib, int col, int row) {
	if (dib) {
		int width = FreeImage_GetWidth(dib);

		if (FreeImage_GetBPP(dib) == 4)
			width /= 2;
		else if (FreeImage_GetBPP(dib) == 1)
			width /= 8;

		return FreeImage_GetBits(dib) + ((FreeImage_GetHeight(dib) - row - 1) * FreeImage_GetPitch(dib)) + (col * FreeImage_GetBPP(dib) / 8);
	} else {
		return NULL;
	}
}

BYTE * DLL_CALLCONV
FreeImage_GetScanLine(FIBITMAP *dib, int scanline) {
	return (dib) ? CalculateScanLine(FreeImage_GetBits(dib), FreeImage_GetPitch(dib), scanline) : NULL;
}

unsigned DLL_CALLCONV
FreeImage_GetDIBSize(FIBITMAP *dib) {
	return (dib) ? sizeof(BITMAPINFOHEADER) + (FreeImage_GetColorsUsed(dib) * sizeof(RGBQUAD)) + (FreeImage_GetPitch(dib) * FreeImage_GetHeight(dib)) : 0;
}

RGBQUAD * DLL_CALLCONV
FreeImage_GetPalette(FIBITMAP *dib) {
	return (dib && FreeImage_GetBPP(dib) < 16) ? (RGBQUAD *)((BYTE *)dib->data + sizeof(FREEIMAGEHEADER) + sizeof(BITMAPINFOHEADER)) : NULL;
}

unsigned DLL_CALLCONV
FreeImage_GetDotsPerMeterX(FIBITMAP *dib) {
	return FreeImage_GetInfoHeader(dib)->biXPelsPerMeter;
}

unsigned DLL_CALLCONV
FreeImage_GetDotsPerMeterY(FIBITMAP *dib) {
	return (dib) ? FreeImage_GetInfoHeader(dib)->biYPelsPerMeter : 0;
}

BITMAPINFOHEADER * DLL_CALLCONV
FreeImage_GetInfoHeader(FIBITMAP *dib) {
	return (dib) ? (BITMAPINFOHEADER *)((BYTE *)dib->data + sizeof(FREEIMAGEHEADER)) : NULL;
}

BITMAPINFO * DLL_CALLCONV
FreeImage_GetInfo(FIBITMAP *dib) {
	return (dib) ? (BITMAPINFO *)((BYTE *)dib->data + sizeof(FREEIMAGEHEADER)) : NULL;
}
