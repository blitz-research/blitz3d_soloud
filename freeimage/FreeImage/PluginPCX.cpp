// ==========================================================
// PCX Loader
//
// Design and implementation by
// - Floris van den Berg (flvdberg@wxs.nl)
// - Jani Kajala (janik@remedy.fi)
// - Markus Loibl (markus.loibl@epost.de)
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

typedef struct tagPCXHEADER {
	BYTE  manufacturer;		// Magic number (0x0A = ZSoft Z)
	BYTE  version;			// Version	0 == 2.5
							//          2 == 2.8 with palette info
							//          3 == 2.8 without palette info
							//          5 == 3.0 with palette info
	BYTE  encoding;			// Encoding: 0 = uncompressed, 1 = PCX rle compressed
	BYTE  bpp;				// Bits per pixel per plane (only 1 or 8)
	WORD  window[4];		// left, upper, right,lower pixel coord.
	WORD  hdpi;				// Horizontal resolution
	WORD  vdpi;				// Vertical resolution
	BYTE  color_map[48];	// Colormap for 16-color images
	BYTE  reserved;
	BYTE  planes;			// Number of planes (1, 3 or 4)
	WORD  bytes_per_line;	// Bytes per row (always even)
	WORD  palette_info;		// Palette information (1 = color or b&w; 2 = gray scale)
	WORD  h_screen_size;
	WORD  v_screen_size;
	BYTE  filler[54];		// Reserved filler
} PCXHEADER;
		
#ifdef WIN32
#pragma pack(pop)
#else
#pragma pack(4)
#endif

// ==========================================================
// Internal functions
// ==========================================================

static WORD
readline(FreeImageIO &io, fi_handle handle, BYTE *buffer, WORD length, BOOL rle) {
	// -----------------------------------------------------------//
	// Read either run-length encoded or normal image data        //
	//                                                            //
	//       THIS IS HOW RUNTIME LENGTH ENCODING WORKS IN PCX:    //
	//                                                            //
	//  1) If the upper 2 bits of a byte are set,                 //
	//     the lower 6 bits specify the count for the next byte   //
	//                                                            //
	//  2) If the upper 2 bits of the byte are clear,             //
	//     the byte is actual data with a count of 1              //
	//                                                            //
	//  Note that a scanline always has an even number of bytes   //
	// -------------------------------------------------------------

	BYTE count = 0, value = 0;
	WORD written = 0;

	if (rle) {
		// run-length encoded read

		while (length--) {
			if (count == 0) {
				io.read_proc(&value, 1, 1, handle);

				if ((value & 0xC0) == 0xC0) {
					count = value & 0x3F;
					io.read_proc(&value, 1, 1, handle);
				} else {
					count = 1;
				}
			}

			count--;

			*(buffer + written++) = value;
		}
	} else {
		// normal read

		written = io.read_proc(buffer, length, 1, handle);
	}

	return written;
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
	return "PCX";
}

static const char * DLL_CALLCONV
Description() {
	return "Zsoft Paintbrush";
}

static const char * DLL_CALLCONV
Extension() {
	return "pcx";
}

static const char * DLL_CALLCONV
RegExpr() {
	return NULL;
}

static BOOL DLL_CALLCONV
Validate(FreeImageIO &io, fi_handle handle) {
	BYTE pcx_signature = 0x0A;
	BYTE signature = 0;

	io.read_proc(&signature, 1, 1, handle);

	return (pcx_signature == signature);
}

// ----------------------------------------------------------

static FIBITMAP * DLL_CALLCONV
Load(FreeImage &freeimage, FreeImageIO &io, fi_handle handle, int page, int flags, void *data) {
	FIBITMAP *dib = NULL;
	BYTE *bits;			// Pointer to dib data
	RGBQUAD *pal;		// Pointer to dib palette
	BYTE *line = NULL;	// PCX raster line
	WORD linelength;	// Length of raster line in bytes
	WORD pitch;			// Length of DIB line in bytes
	BOOL rle;			// True if the file is run-length encoded

	if (handle) {
		try {
			// process the header

			PCXHEADER header;

			io.read_proc(&header, sizeof(PCXHEADER), 1, handle);

			// check PCX identifier

			if ((header.manufacturer != 0x0A) || (header.version > 5))
				throw "Invalid PCX file";

			// allocate a new DIB

			WORD width = header.window[2] - header.window[0] + 1;
			WORD height = header.window[3] - header.window[1] + 1;
			WORD bitcount = header.bpp * header.planes;

			if (bitcount == 24)
				dib = freeimage.allocate_proc(width, height, bitcount, 0xFF, 0xFF00, 0xFF0000);
			else
				dib = freeimage.allocate_proc(width, height, bitcount, 0, 0, 0);

			// if the dib couldn't be allocated, throw an error

			if (!dib)
				throw "DIB allocation failed";			

			// metrics handling code

			BITMAPINFOHEADER *pInfoHeader = freeimage.get_info_header_proc(dib);
			pInfoHeader->biXPelsPerMeter = (int) (((float)header.hdpi) / 0.0254000 + 0.5);
			pInfoHeader->biYPelsPerMeter = (int) (((float)header.vdpi) / 0.0254000 + 0.5);

			// Set up the palette if needed
			// ----------------------------

			switch(bitcount) {
				case 1:
				{
					pal = freeimage.get_palette_proc(dib);
					pal[0].rgbRed = pal[0].rgbGreen = pal[0].rgbBlue = 0;
					pal[1].rgbRed = pal[1].rgbGreen = pal[1].rgbBlue = 255;
					break;
				}

				case 4:
				{
					pal = freeimage.get_palette_proc(dib);

					BYTE *pColormap = &header.color_map[0];

					for(int i = 0; i < 16; i++) {
						pal[i].rgbRed   = pColormap[0];
						pal[i].rgbGreen = pColormap[1];
						pal[i].rgbBlue  = pColormap[2];
						pColormap += 3;
					}

					break;
				}

				case 8:
				{
					BYTE palette_id;

					io.seek_proc(handle, -769L, SEEK_END);
					io.read_proc(&palette_id, 1, 1, handle);

					if (palette_id == 0x0C) {
						BYTE *cmap = (BYTE*)malloc(768 * sizeof(BYTE));
						io.read_proc(cmap, 768, 1, handle);

						pal = freeimage.get_palette_proc(dib);
						BYTE *pColormap = &cmap[0];

						for(int i = 0; i < 256; i++) {
							pal[i].rgbRed   = pColormap[0];
							pal[i].rgbGreen = pColormap[1];
							pal[i].rgbBlue  = pColormap[2];
							pColormap += 3;
						}

						free(cmap);
					}

					// wrong palette ID, perhaps a gray scale is needed ?

					else if (header.palette_info == 2) {
						pal = freeimage.get_palette_proc(dib);

						for(int i = 0; i < 256; i++) {
							pal[i].rgbRed   = i;
							pal[i].rgbGreen = i;
							pal[i].rgbBlue  = i;
						}
					}

					io.seek_proc(handle, (long)sizeof(PCXHEADER), SEEK_SET);
				}
				break;
			}

			// calculate the line length for the PCX and the DIB

			linelength = header.bytes_per_line * header.planes;
			pitch = freeimage.get_pitch_proc(dib);

			// run-length encoding ?

			rle = (header.encoding == 1) ? TRUE : FALSE;

			// load image data
			// ---------------

			line = (BYTE*) malloc(linelength * sizeof(BYTE));
			bits = freeimage.get_scanline_proc(dib, height - 1);

			if ((header.planes == 1) && ((header.bpp == 1) || (header.bpp == 8))) {
				BYTE skip;
				WORD written;

				for (WORD y = 0; y < height; y++) {
					written = readline(io, handle, line, linelength, rle);
					memcpy(bits, line, linelength);

					// skip trailing garbage at the end of the scanline

					for (int count = written; count < linelength; count++)
						io.read_proc(&skip, sizeof(BYTE), 1, handle);					

					bits -= pitch;
				}
			}
			else if ((header.planes == 4) && (header.bpp == 1)) {
				BYTE bit, index, mask, skip;
				BYTE *buffer;
				WORD x, y, written;
				buffer = (BYTE*)malloc(width * sizeof(BYTE));

				for (y = 0; y < height; y++) {
					written = readline(io, handle, line, linelength, rle);

					// build a nibble using the 4 planes

					memset(buffer, 0, width * sizeof(BYTE));

					for(int plane = 0; plane < 4; plane++) {
						bit = 1 << plane;

						for(x = 0; x < width; x++) {
							index = (x / 8) + plane * header.bytes_per_line;
							mask = 0x80 >> (x & 0x07);
							buffer[x] |= (line[index] & mask) ? bit : 0;
						}
					}

					// then write the DIB row

					for (x = 0; x < width / 2; x++)
						bits[x] = (buffer[2*x] << 4) | buffer[2*x+1];

					// skip trailing garbage at the end of the scanline

					for (int count = written; count < linelength; count++)
						io.read_proc(&skip, sizeof(BYTE), 1, handle);

					bits -= pitch;
				}

				free(buffer);
			} else if((header.planes == 3) && (header.bpp == 8)) {
				BYTE *pline;

				for (WORD y = 0; y < height; y++) {
					readline(io, handle, line, linelength, rle);

					// convert the plane stream to BGR (RRRRGGGGBBBB -> BGRBGRBGRBGR)

					pline = line;

					for (int plane = 2; plane >= 0; plane--) {
						for (WORD x = 0; x < width; x++)
							bits[x * 3 + plane] = pline[x];						

						pline += header.bytes_per_line;
					}

					bits -= pitch;
				}
			} else {
				throw "Unable to read this file";
			}

			free(line);
			return dib;

		} catch (char *text) {
			// free allocated memory

			if (dib != NULL)
				freeimage.free_proc(dib);

			if (line != NULL)
				free(line);

			freeimage.output_message_proc(s_format_id, text);

			return NULL;
		}
	}
	
	return NULL;
}

// ==========================================================
//   Init
// ==========================================================

void DLL_CALLCONV
InitPCX(Plugin &plugin, int format_id) {
	s_format_id = format_id;

	plugin.format_proc = Format;
	plugin.description_proc = Description;
	plugin.extension_proc = Extension;
	plugin.validate_proc = Validate;
	plugin.regexpr_proc = RegExpr;
	plugin.load_proc = Load;
}
