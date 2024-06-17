// ==========================================================
// Kodak PhotoCD Loader
//
// Design and implementation by
// - Floris van den Berg (flvdberg@wxs.nl)
// 
// Based on pascal code developed by Alex Kwak
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

#pragma warning (disable : 4244)

#include <stdlib.h>
#include <memory.h> 

#include "FreeImage.h"
#include "Utilities.h"

// ==========================================================
// Internal functions
// ==========================================================

inline float
fix(float a) {
	return (a < 0) ? 0 : (a > 255) ? 255 : a;
}

static void
YUV2RGB(int y, int cb, int cr, int &r, int &g, int &b) {
	const float c = 256;

	float c11 = 0.0054980  * c;
	float c12 = 0.0000001  * c;
	float c13 = 0.0051681  * c;
	float c21 = 0.0054980  * c;
	float c22 = -0.0015446 * c;
	float c23 = -0.0026325 * c;
	float c31 = 0.0054980  * c;
	float c32 = 0.0079533  * c;
	float c33 = 0.0000001  * c;

	r = fix(round(c11 * y + c12 * (cb - 156) + c13 * (cr - 137)));
	g = fix(round(c21 * y + c22 * (cb - 156) + c23 * (cr - 137)));
	b = fix(round(c31 * y + c32 * (cb - 156) + c33 * (cr - 137)));
}

static BOOL
VerticalOrientation(FreeImageIO &io, fi_handle handle) {
	char buffer[128];

	io.read_proc(buffer, 128, 1, handle);

	return (buffer[72] & 63) == 8;
}

// ==========================================================
// Plugin Interface
// ==========================================================

static int s_format_id;

// ==========================================================
// Plugin Implementation
// ==========================================================

static const char * DLL_CALLCONV
Format() {
	return "PCD";
}

static const char * DLL_CALLCONV
Description() {
	return "Kodak PhotoCD";
}

static const char * DLL_CALLCONV
Extension() {
	return "pcd";
}

static const char * DLL_CALLCONV
RegExpr() {
	return NULL;
}

// ----------------------------------------------------------

static FIBITMAP * DLL_CALLCONV
Load(FreeImage &freeimage, FreeImageIO &io, fi_handle handle, int page, int flags, void *data) {
	int width;
	int height;
	int line;
	int pitch;
	int bpp = 24;
	int scan_line_add   = 1;
	int start_scan_line = 0;

	// to make absolute seeks possible we store the current position in the file
	
	long offset_in_file = io.tell_proc(handle);
	long seek;

	switch (flags) {
		case PCD_BASEDIV4 :
			seek = 0x2000;
			width = 192;
			height = 128;
			line = CalculateLine(width, bpp);
			pitch = CalculatePitch(line);
			break;

		case PCD_BASEDIV16 :
			seek = 0xB800;
			width = 384;
			height = 256;
			line = CalculateLine(width, bpp);
			pitch = CalculatePitch(line);
			break;

		default :
			seek = 0x30000;
			width = 768;
			height = 512;
			line = CalculateLine(width, bpp);
			pitch = CalculatePitch(line);
			break;
	}

	// allocate the dib and write out the header

	FIBITMAP *dib = freeimage.allocate_proc(width, height, bpp, 0xFF, 0xFF00, 0xFF0000);
	
	// check if the PCD is bottom-up

	if (VerticalOrientation(io, handle)) {
		scan_line_add = -1;
		start_scan_line = height - 1;		
	}

	// temporary stuff to load PCD

	BYTE *y1   = (BYTE *)malloc(width);
	BYTE *y2   = (BYTE *)malloc(width);
	BYTE *cbcr = (BYTE *)malloc(width);
	BYTE *yl[] = { y1, y2 };

	// seek to the part where the bitmap data begins

	io.seek_proc(handle, offset_in_file, SEEK_SET);
	io.seek_proc(handle, seek, SEEK_CUR);

	// read the data

	for (int y = 0; y < height / 2; ++y) {
		io.read_proc(y1, width, 1, handle);
		io.read_proc(y2, width, 1, handle);
		io.read_proc(cbcr, width, 1, handle);

		for (int i = 0; i < 2; ++i) {
			for (int x = 0; x < width; ++x) {
				int r, g, b;

				YUV2RGB(yl[i][x], cbcr[x / 2], cbcr[(width / 2) + (x / 2)], r, g, b);

				*(freeimage.get_scanline_proc(dib, start_scan_line) + (x * 3) + 0) = b;
				*(freeimage.get_scanline_proc(dib, start_scan_line) + (x * 3) + 1) = g;
				*(freeimage.get_scanline_proc(dib, start_scan_line) + (x * 3) + 2) = r;
			}

			start_scan_line += scan_line_add;
		}
	}

	free(cbcr);
	free(y2);
	free(y1);

	return dib;
}

// ==========================================================
//   Init
// ==========================================================

void DLL_CALLCONV
InitPCD(Plugin &plugin, int format_id) {
	s_format_id = format_id;

	plugin.format_proc = Format;
	plugin.description_proc = Description;
	plugin.extension_proc = Extension;
	plugin.regexpr_proc = RegExpr;
	plugin.load_proc = Load;
}
