// ==========================================================
// ICO Loader
//
// Design and implementation by
// - Floris van den Berg (flvdberg@wxs.nl)
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

#include <stdlib.h>

#include "FreeImage.h"
#include "Utilities.h"

// ----------------------------------------------------------
//   Constants + headers
// ----------------------------------------------------------

#ifdef WIN32
#pragma pack(push, 1)
#else
#pragma pack(1)
#endif

typedef struct tagIconDirectoryEntry {
    BYTE  bWidth;
    BYTE  bHeight;
    BYTE  bColorCount;
    BYTE  bReserved;
    WORD  wPlanes;
    WORD  wBitCount;
    DWORD dwBytesInRes;
    DWORD dwImageOffset;
} ICONDIRENTRY;

typedef struct tagIconDir {
    WORD          idReserved;
    WORD          idType;
    WORD          idCount;
} ICONHEADER;

#ifdef WIN32
#pragma pack(pop)
#else
#pragma pack(4)
#endif

// ==========================================================
// Plugin Interface
// ==========================================================

static int s_format_id;

// ==========================================================
// Plugin Implementation
// ==========================================================

static const char * DLL_CALLCONV
Format() {
	return "ICO";
}

static const char * DLL_CALLCONV
Description() {
	return "Windows Icon";
}

static const char * DLL_CALLCONV
Extension() {
	return "ico";
}

static const char * DLL_CALLCONV
RegExpr() {
	return NULL;
}

static BOOL DLL_CALLCONV
Validate(FreeImageIO &io, fi_handle handle) {
	ICONHEADER icon_header;

	long read = io.read_proc(&icon_header, 1, sizeof(ICONHEADER), handle);

	return ((icon_header.idReserved == 0) && (icon_header.idType == 1));
}

// ----------------------------------------------------------

static FIBITMAP * DLL_CALLCONV
Load(FreeImage &freeimage, FreeImageIO &io, fi_handle handle, int page, int flags, void *data) {
	if (page == -1)
		page = 0;

	if (handle != NULL) {
		FIBITMAP *dib;

		// we use this offset value to make seemingly absolute seeks relative in the file
	
		long start_of_file = io.tell_proc(handle);

		// read the icon header

		ICONHEADER icon_header;

		io.read_proc(&icon_header, sizeof(ICONHEADER), 1, handle);

		if ((icon_header.idReserved == 0) && (icon_header.idType == 1)) {
			// load the icon descriptions

			ICONDIRENTRY *icon_list = (ICONDIRENTRY *)malloc(icon_header.idCount * sizeof(ICONDIRENTRY));

			io.read_proc(icon_list, icon_header.idCount * sizeof(ICONDIRENTRY), 1, handle);

			// load the specified icon
			
			if (page < icon_header.idCount) {
				// seek to the start of the bitmap data for the icon

				io.seek_proc(handle, start_of_file, SEEK_SET);
				io.seek_proc(handle, icon_list[(int)flags].dwImageOffset, SEEK_CUR);

				// load the BITMAPINFOHEADER

				BITMAPINFOHEADER header;
				io.read_proc(&header, sizeof(BITMAPINFOHEADER), 1, handle);

				// allocate the bitmap

				int width  = header.biWidth;
				int height = header.biHeight / 2; // height == xor + and mask
				int bit_count = header.biBitCount;
				int line   = CalculateLine(width, bit_count);
				int pitch  = CalculatePitch(line);

				// allocate memory for one icon

				dib = freeimage.allocate_proc(width, height, bit_count, 0, 0, 0);

				if (dib == NULL) {
					free(icon_list);

					return NULL;
				}

				// black and white cursors look much better on white

				if (icon_header.idType == 2)
					if (bit_count == 1)
						memset(FreeImage_GetBits(dib), 255, pitch * height);

				// read the palette data

				io.read_proc(freeimage.get_palette_proc(dib), CalculateUsedColors(bit_count) * sizeof(RGBQUAD), 1, handle);				

				// apply the AND and XOR masks

				BYTE *xor_mask      = (BYTE *)malloc(pitch * height);
				BYTE *and_mask      = (BYTE *)malloc(pitch * height);
				BYTE *copy_xor_mask = xor_mask;
				BYTE *copy_and_mask = and_mask;

				io.read_proc(xor_mask, pitch * height, 1, handle);
				io.read_proc(and_mask, pitch * height, 1, handle);

				for (int row = 0; row < height; row++) {
					for (int column = 0; column < line; column++) {
						*(freeimage.get_scanline_proc(dib, row) + column) &= copy_and_mask[column];
						*(freeimage.get_scanline_proc(dib, row) + column) ^= copy_xor_mask[column];
					}

					copy_xor_mask += pitch;
					copy_and_mask += pitch;
				}

				free(icon_list);
				free(and_mask);
				free(xor_mask);

				// bitmap has been loaded successfully!

				return (FIBITMAP *)dib;
			}
		} else {
			freeimage.output_message_proc(s_format_id, "file is not an ICO file");
		}
	}

	return NULL;
}

// ==========================================================
//   Init
// ==========================================================

void DLL_CALLCONV
InitICO(Plugin &plugin, int format_id) {
	s_format_id = format_id;

	plugin.format_proc = Format;
	plugin.description_proc = Description;
	plugin.extension_proc = Extension;
	plugin.regexpr_proc = RegExpr;
	plugin.load_proc = Load;
	plugin.validate_proc = Validate;
}
