// ==========================================================
// PNM (PPM, PGM, PBM) Loader and Writer
//
// Design and implementation by
// - Floris van den Berg (flvdberg@wxs.nl)
// - Hervé Drolon (drolon@infonie.fr)
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

#include <stdio.h>		// for sprintf()
#include <string.h>

#include "FreeImage.h"
#include "Utilities.h"

// ==========================================================
// Internal functions
// ==========================================================

static int
GetInt(FreeImageIO &io, fi_handle handle) {
	// get an integer value from the actual position pointed by handle

    char c = 0;
	BOOL firstchar;

    // skip forward to start of next number

    io.read_proc(&c, 1, 1, handle);

    while (1) {
        // eat comments

        if (c == '#') {
			// if we're at a comment, read to end of line

            firstchar = TRUE;

            while (1) {
				io.read_proc(&c, 1, 1, handle);

				if (firstchar && c == ' ') {
					// loop off 1 sp after #

					firstchar = FALSE;
				} else if(c == '\n') {
					break;
				}
			}
		}

        if (c >= '0' && c <='9') {
			// we've found what we were looking for

            break;
		}

        io.read_proc(&c, 1, 1, handle);
    }

    // we're at the start of a number, continue until we hit a non-number

    int i = 0;

    while (1) {
        i = (i*10) + (c - '0');

        io.read_proc(&c, 1, 1, handle);

        if (c < '0' || c > '9')
            break;
    }

    return i;
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
	return "PNM";
}

static const char * DLL_CALLCONV
Description() {
	return "Portable Network Media";
}

static const char * DLL_CALLCONV
Extension() {
	return "pbm,pgm,ppm";
}

static const char * DLL_CALLCONV
RegExpr() {
	return NULL;
}

static BOOL DLL_CALLCONV
Validate(FreeImageIO &io, fi_handle handle) {
	BYTE pbm_id1[] = { 0x50, 0x31 };
	BYTE pbm_id2[] = { 0x50, 0x34 };
	BYTE pgm_id1[] = { 0x50, 0x32 };
	BYTE pgm_id2[] = { 0x50, 0x35 };
	BYTE ppm_id1[] = { 0x50, 0x33 };
	BYTE ppm_id2[] = { 0x50, 0x36 };
	BYTE signature[2] = { 0, 0 };

	io.read_proc(signature, 1, sizeof(pbm_id1), handle);

	if (memcmp(pbm_id1, signature, sizeof(pbm_id1)) == 0)
		return TRUE;

	if (memcmp(pbm_id1, signature, sizeof(pbm_id2)) == 0)
		return TRUE;

	if (memcmp(pbm_id1, signature, sizeof(pgm_id1)) == 0)
		return TRUE;

	if (memcmp(pbm_id1, signature, sizeof(pgm_id2)) == 0)
		return TRUE;

	if (memcmp(pbm_id1, signature, sizeof(ppm_id1)) == 0)
		return TRUE;

	if (memcmp(pbm_id1, signature, sizeof(ppm_id2)) == 0)
		return TRUE;

	return FALSE;
}

// ----------------------------------------------------------

static FIBITMAP * DLL_CALLCONV
Load(FreeImage &freeimage, FreeImageIO &io, fi_handle handle, int page, int flags, void *data) {
	char id_one, id_two;
	WORD x, y;
	FIBITMAP *dib;
    BYTE *bits;		// pointer to dib data
	RGBQUAD *pal;	// pointer to dib palette
	int i, max, level;

	if (!handle)
		return NULL;

	try {
		// Read the first two bytes of the file to determine the file format
		// "P1" = ascii bitmap, "P2" = ascii greymap, "P3" = ascii pixmap,
		// "P4" = raw bitmap, "P5" = raw greymap, "P6" = raw pixmap

		io.read_proc(&id_one, 1, 1, handle);
		io.read_proc(&id_two, 1, 1, handle);

		if (id_one != 'P' || id_two < '1' || id_two > '6') {
			throw "Invalid magic number";
		}

		// Read the header information

		int width  = GetInt(io, handle);
		int height = GetInt(io, handle);

		// Create a new DIB

		switch (id_two) {
			case '1':
			case '4':
				dib = freeimage.allocate_proc(width, height, 1, 0, 0, 0);
				break;

			case '2':
			case '5':
				dib = freeimage.allocate_proc(width, height, 8, 0, 0, 0);
				break;

			case '3':
			case '6':
				dib = freeimage.allocate_proc(width, height, 24, 0xFF, 0xFF00, 0xFF0000);
				break;
		}

		if (dib == NULL)
			throw "DIB allocation failed";

		// Read the image...

		int pitch = freeimage.get_pitch_proc(dib);

		switch(id_two)  {
			case '1':
			case '4':
				// write the palette data

				pal = freeimage.get_palette_proc(dib);
				pal[0].rgbRed = pal[0].rgbGreen = pal[0].rgbBlue = 0;
				pal[1].rgbRed = pal[1].rgbGreen = pal[1].rgbBlue = 255;

				// write the bitmap data

				if (id_two == '1') {	// ASCII bitmap
					for (y = 0; y < height; y++) {				
						bits = freeimage.get_bits_proc(dib) + (height - 1 - y) * pitch;

						for (x = 0; x < width; x++) {
							if (GetInt(io, handle) == 0)
								bits[x >> 3] |= (0x80 >> (x & 0x7));
							else
								bits[x >> 3] &= (0xFF7F >> (x & 0x7));
						}
					}
				}  else {		// Raw bitmap
					int line = CalculateLine(width, 1);

					for (y = 0; y < height; y++) {				
						bits = freeimage.get_bits_proc(dib) + (height - 1 - y) * pitch;

						for (x = 0; x < line; x++) {
							io.read_proc(&bits[x], 1, 1, handle);

							bits[x] = ~bits[x];
						}
					}
				}

				return dib;

			case '2':
			case '5':
				// Build a greyscale palette
				
				pal = freeimage.get_palette_proc(dib);

				for (i = 0; i < 256; i++) {
					pal[i].rgbRed	=
					pal[i].rgbGreen =
					pal[i].rgbBlue	= i;
				}

				max = GetInt(io, handle);	// read the 'max' value

				// write the bitmap data

				if(id_two == '2') {		// ASCII greymap
					for (y = 0; y < height; y++) {				
						bits = freeimage.get_bits_proc(dib) + (height - 1 - y) * pitch;
						for (x = 0; x < width; x++) {
							level = GetInt(io, handle);
							bits[x] = (BYTE)((255 * level) / max);
						}
					}
				} else {		// Raw greymap
					level = 0;

					for (y = 0; y < height; y++) {				
						bits = freeimage.get_bits_proc(dib) + (height - 1 - y) * pitch;

						for (x = 0; x < width; x++) {
							io.read_proc(&level, 1, 1, handle);
							bits[x] = (BYTE)((255 * level) / max);
						}
					}
				}

				return dib;

			case '3':
			case '6':
				max = GetInt(io, handle);	// read the 'max' value

				// write the bitmap data

				if (id_two == '3') {		// ASCII pixmap
					for (y = 0; y < height; y++) {				
						bits = freeimage.get_bits_proc(dib) + (height - 1 - y) * pitch;

						for (x = 0; x < width; x++) {
							bits[2] = (BYTE)((255 * GetInt(io, handle)) / max);	// R
							bits[1] = (BYTE)((255 * GetInt(io, handle)) / max);	// G
							bits[0] = (BYTE)((255 * GetInt(io, handle)) / max);	// B

							bits += 3;
						}
					}
				}  else {			// Raw pixmap
					level = 0;

					for (y = 0; y < height; y++) {				
						bits = freeimage.get_bits_proc(dib) + (height - 1 - y) * pitch;

						for (x = 0; x < width; x++) {
							io.read_proc(&level, 1, 1, handle); 
							bits[2] = (BYTE)((255 * level) / max);	// R

							io.read_proc(&level, 1, 1, handle);
							bits[1] = (BYTE)((255 * level) / max);	// G

							io.read_proc(&level, 1, 1, handle);
							bits[0] = (BYTE)((255 * level) / max);	// B

							bits += 3;
						}
					}
				}

				return dib;
		}

	} catch(char *text)  {
		switch(id_two)  {
			case '1':
			case '4':
				freeimage.output_message_proc(s_format_id, text);
				break;
			case '2':
			case '5':
				freeimage.output_message_proc(s_format_id, text);
				break;
			case '3':
			case '6':
				freeimage.output_message_proc(s_format_id, text);
				break;
		}

		return NULL;
	}
		
	return NULL;
}

static BOOL DLL_CALLCONV
Save(FreeImage &freeimage, FreeImageIO &io, FIBITMAP *dib, fi_handle handle, int page, int flags, void *data) {
	// ----------------------------------------------------------
	//   PNM Saving
	// ----------------------------------------------------------
	//
	// Output format :
	//
	// Bit depth		flags			file format
	// -------------    --------------  -----------
	// 1-bit / pixel	PNM_SAVE_ASCII	PBM (P1)
	// 1-bit / pixel	PNM_SAVE_RAW	PBM (P4)
	// 8-bit / pixel	PNM_SAVE_ASCII	PGM (P2)
	// 8-bit / pixel	PNM_SAVE_RAW	PGM (P5)
	// 24-bit / pixel	PNM_SAVE_ASCII	PPM (P3)
	// 24-bit / pixel	PNM_SAVE_RAW	PPM (P6)
	// ----------------------------------------------------------

    int magic, bpp;
	WORD x, y, width, height;
    BYTE *bits;		// pointer to dib data
	
	if ((dib) && (handle)) {
		bpp = freeimage.get_bpp_proc(dib);
		width = freeimage.get_width_proc(dib);
		height = freeimage.get_height_proc(dib);

		// Find the appropriate magic number for this file type

		magic = 0;

		switch (bpp) {
			case 1 :
				magic = 1;	// PBM file (B & W)
				break;
			case 8 : 			
				magic = 2;	// PGM file	(Greyscale)
				break;

			case 24 :
				magic = 3;	// PPM file (RGB)
				break;

			case 32 :
				if (!freeimage.is_transparent_proc(dib))
					magic = 3;
				else
					throw "PNM does not support alpha channels";

				break;

			default:
				return FALSE;	// Invalid bit depth
		}

		if (flags == PNM_SAVE_RAW)
			magic += 3;

		// Write the header info

		char buffer[20];
		sprintf(buffer, "P%d\n%d %d\n", magic, width, height);
		io.write_proc(&buffer, strlen(buffer), 1, handle);

		if (bpp != 1) {
			sprintf(buffer, "255\n");
			io.write_proc(&buffer, strlen(buffer), 1, handle);
		}

		// Write the image data
		///////////////////////

		int pitch = (bpp == 32) ? CalculatePitch(CalculateLine(width, 24)) : freeimage.get_pitch_proc(dib);

		switch(bpp)  {
			case 32 :
			case 24 :            // 24-bit RGB, 3 bytes per pixel
			{
				if (flags == PNM_SAVE_RAW)  {
					BYTE *buffer = (BYTE *)malloc(pitch);

					for (y = 0; y < height; y++) {
						// get a pointer to the (converted) scanline

						if (bpp == 32)
							freeimage.convert_line_32to24_proc(buffer, freeimage.get_scanline_proc(dib, height - y - 1), width);
						else
							memcpy(buffer, freeimage.get_scanline_proc(dib, height - 1 - y), pitch);

						// write the scanline to disc

						BYTE *bits = buffer;

						for (x = 0; x < width; x++) {
							io.write_proc(&bits[2], 1, 1, handle);	// R
							io.write_proc(&bits[1], 1, 1, handle);	// G
							io.write_proc(&bits[0], 1, 1, handle);	// B

							bits += 3;
						}
					}

					free(buffer);
				} else {
					int length = 0;

					BYTE *buffer = (BYTE *)malloc(pitch);

					for (y = 0; y < height; y++) {
						// get a pointer to the (converted) scanline

						if (bpp == 32)
							freeimage.convert_line_32to24_proc(buffer, freeimage.get_scanline_proc(dib, height - 1 - y), width);
						else
							memcpy(buffer, freeimage.get_scanline_proc(dib, height - 1 - y), pitch);

						// write the scanline to disc

						BYTE *bits = buffer;
						
						for (x = 0; x < width; x++) {
							char buffer[20];

							sprintf(buffer, "%3d %3d %3d ", bits[2], bits[1], bits[0]);

							io.write_proc(&buffer, strlen(buffer), 1, handle);

							length += 12;

							if(length > 58) {
								sprintf(buffer, "\n");

								io.write_proc(&buffer, strlen(buffer), 1, handle);

								length = 0;
							}

							bits += 3;
						}					
					}

					free(buffer);
				}

				break;
			}

			case 8:		// 8-bit greyscale
			{
				if (flags == PNM_SAVE_RAW)  {
					for (y = 0; y < height; y++) {
						bits = freeimage.get_bits_proc(dib) + (height - 1 - y) * pitch;

						for (x = 0; x < width; x++) {
							io.write_proc(&bits[x], 1, 1, handle);
						}
					}
				} else {
					int length = 0;

					for (y = 0; y < height; y++) {
						bits = freeimage.get_bits_proc(dib) + (height - 1 - y) * pitch;

						for (x = 0; x < width; x++) {
							sprintf(buffer, "%3d ", bits[x]);

							io.write_proc(&buffer, strlen(buffer), 1, handle);

							length += 4;

							if (length > 66)	{
								sprintf(buffer, "\n");

								io.write_proc(&buffer, strlen(buffer), 1, handle);

								length = 0;
							}
						}
					}
				}

				break;
			}

			case 1:		// 1-bit B & W
			{
				RGBQUAD *pal = freeimage.get_palette_proc(dib);
				int color;

				if (flags == PNM_SAVE_RAW)  {
					for(y = 0; y < height; y++) {
						bits = freeimage.get_bits_proc(dib) + (height - 1 - y) * pitch;

						for(x = 0; x < freeimage.get_line_proc(dib); x++)
							io.write_proc(&bits[x], 1, 1, handle);
					}
				} else  {
					int length = 0;

					for (y = 0; y < height; y++) {
						bits = freeimage.get_bits_proc(dib) + (height - 1 - y) * pitch;

						for (x = 0; x < freeimage.get_line_proc(dib) * 8; x++)	{
							color = (bits[x>>3] & (0x80 >> (x & 0x07))) != 0;

							sprintf(buffer, "%c ", color ? '1':'0');

							io.write_proc(&buffer, strlen(buffer), 1, handle);

							length += 2;

							if (length > 68) {
								sprintf(buffer, "\n");

								io.write_proc(&buffer, strlen(buffer), 1, handle);

								length = 0;
							}
						}
					}
				}
			}
			
			break;
		}

		return TRUE;
	}

	return FALSE;
}

// ==========================================================
//   Init
// ==========================================================

void DLL_CALLCONV
InitPNM(Plugin &plugin, int format_id) {
	s_format_id = format_id;

	plugin.format_proc = Format;
	plugin.description_proc = Description;
	plugin.extension_proc = Extension;
	plugin.regexpr_proc = RegExpr;
	plugin.load_proc = Load;
	plugin.save_proc = Save;
	plugin.validate_proc = Validate;
}

