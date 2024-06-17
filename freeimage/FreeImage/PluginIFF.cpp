// ==========================================================
// Deluxe Paint Loader
//
// Design and implementation by
// - Floris van den Berg (flvdberg@wxs.nl)
// - Mark Sibly (marksibly@blitzbasic.com)
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
//  Internal typedefs and structures
// ----------------------------------------------------------

#ifdef WIN32
#pragma pack(push, 1)
#else
#pragma pack(1)
#endif

typedef struct {
    WORD w, h;                    /* raster width & height in pixels */
    WORD x, y;                    /* position for this image */
    BYTE nPlanes;                 /* # source bitplanes */
    BYTE masking;                 /* masking technique */
    BYTE compression;             /* compression algorithm */
    BYTE pad1;                    /* UNUSED.  For consistency, put 0 here.*/
    WORD transparentColor;        /* transparent "color number" */
    BYTE xAspect, yAspect;        /* aspect ratio, a rational number x/y */
    WORD pageWidth, pageHeight;   /* source "page" size in pixels */
} BMHD;

#ifdef WIN32
#pragma pack(pop)
#else
#pragma pack(4)
#endif

// ----------------------------------------------------------

static BOOL big_endian = TRUE;

// ----------------------------------------------------------

static WORD swapWORD( WORD n ){
	return big_endian ? (n >> 8) | (n << 8) : n;
}

static DWORD swapDWORD( DWORD n ){
	return big_endian ? (n >> 24) | (n << 24) | ((n >> 8) & 0xff00) | ((n << 8) & 0xff0000) : n;
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
	return "IFF";
}

static const char * DLL_CALLCONV
Description() {
	return "IFF Interleaved Bitmap";
}

static const char * DLL_CALLCONV
Extension() {
	return "iff,lbm";
}

static const char * DLL_CALLCONV
RegExpr() {
	return NULL;
}

static BOOL DLL_CALLCONV
Validate(FreeImageIO &io, fi_handle handle) {
	unsigned type = 0;

	io.read_proc(&type, 4, 1, handle);

	return (swapDWORD(type) == 'FORM');
}

// ----------------------------------------------------------

static FIBITMAP * DLL_CALLCONV
Load(FreeImage &freeimage, FreeImageIO &io, fi_handle handle, int page, int flags, void *data) {
	if (handle != NULL) {
		FIBITMAP *dib = 0;

		unsigned type, size;

		io.read_proc(&type, 4, 1, handle);

		if (swapDWORD(type) != 'FORM')
			return NULL;

		io.read_proc( &size,4,1,handle );

		size = swapDWORD(size);

		io.read_proc(&type, 4, 1, handle);

		if ((swapDWORD(type) != 'ILBM') && (swapDWORD(type) != 'PBM '))
			return NULL;

		size -= 4;

		unsigned width, height, planes, depth, comp;

		while (size) {
			unsigned ch_type,ch_size;

			io.read_proc(&ch_type, 4, 1, handle);
			ch_type = swapDWORD(ch_type);

			io.read_proc(&ch_size,4,1,handle );
			ch_size = swapDWORD(ch_size);

			unsigned ch_end = io.tell_proc(handle) + ch_size;

			if (ch_type == 'BMHD') {
				if (dib)
					FreeImage_Unload(dib);

				BMHD bmhd;

				io.read_proc(&bmhd, sizeof(bmhd), 1, handle);

				width = swapWORD(bmhd.w);
				height = swapWORD(bmhd.h);
				planes = bmhd.nPlanes;
				comp = bmhd.compression;

				if (planes > 8 && planes != 24)
					return NULL;

				depth = planes > 8 ? 24 : 8;

				unsigned mask = planes > 8 ? 0xff : 0;

				dib = freeimage.allocate_proc(width, height, depth, mask << 16, mask << 8, mask);
			} else if (ch_type == 'CMAP') {
				if (!dib)
					return NULL;

				RGBQUAD *pal = freeimage.get_palette_proc(dib);

				for (unsigned k = 0; k < ch_size / 3;++k ){
					io.read_proc(&pal[k].rgbRed, 1, 1, handle );
					io.read_proc(&pal[k].rgbGreen, 1, 1, handle );
					io.read_proc(&pal[k].rgbBlue, 1, 1, handle );
				}

			} else if (ch_type == 'BODY') {
				if (!dib)
					return NULL;

				if (swapDWORD(type) == 'PBM ') {
					// NON INTERLACED (LBM)

					unsigned line = freeimage.get_line_proc(dib) + 1 & ~1;
					
					for (unsigned i = 0; i < freeimage.get_height_proc(dib); ++i) {
						BYTE *bits = freeimage.get_scanline_proc(dib, freeimage.get_height_proc(dib) - i - 1);

						if (comp == 1) {
							// use RLE compression

							DWORD number_of_bytes_written = 0;
							BYTE rle_count;
							BYTE byte;

							while (number_of_bytes_written < line) {
								io.read_proc(&rle_count, 1, 1, handle);

								if (rle_count < 128) {
									for (int k = 0; k < rle_count + 1; k++) {
										io.read_proc(&byte, 1, 1, handle);

										bits[number_of_bytes_written++] += byte;
									}
								} else if (rle_count > 128) {
									io.read_proc(&byte, 1, 1, handle);

									for (int k = 0; k < 257 - rle_count; k++) {
										bits[number_of_bytes_written++] += byte;
									}
								}
							}
						} else {
							// don't use compression

							io.read_proc(bits, line, 1, handle);
						}
					}

					return dib;
				} else {
					// INTERLACED (ILBM)

					unsigned pixel_size = depth/8;
					unsigned n_width=(width+15)&~15;
					unsigned plane_size = n_width/8;
					unsigned src_size = plane_size * planes;
					unsigned char *src = (unsigned char*)malloc(src_size);
					unsigned char *dest = freeimage.get_bits_proc(dib);

					dest += freeimage.get_pitch_proc(dib) * height;

					for (unsigned y = 0; y < height; ++y) {
						dest -= freeimage.get_pitch_proc(dib);

						// read all planes in one hit,
						// 'coz PSP compresses across planes...

						if (comp) {
							for(unsigned x = 0; x < src_size;){
								signed char t;

								io.read_proc(&t, 1, 1, handle);

								if (t >= 0) {
									++t;

									io.read_proc(src + x, t, 1, handle);

									x += t;
								} else if( t!=-128 ){
									signed char b;

									io.read_proc( &b,1,1,handle );

									t =- t +1;

									memset(src + x, b, t);

									x += t;
								}
							}
						} else {
							io.read_proc(src, src_size, 1, handle);
						}

						// lazy planar->chunky...

						for (unsigned x = 0; x < width; ++x) {
							for (unsigned n = 0; n < planes; ++n) {
								char bit = src[n * plane_size + (x / 8)] >> ((x^7) & 7);
								dest[x * pixel_size + (n / 8)] |= (bit & 1) << (n & 7);
							}
						}

						if (depth == 24){
							for(unsigned x = 0; x < width; ++x){
								char t = dest[x * 3];
								dest[x * 3] = dest[x * 3 + 2];
								dest[x * 3 + 2] = t;
							}
						}
					}

					free(src);

					return dib;
				}
			}

			io.seek_proc(handle, ch_end - io.tell_proc(handle), SEEK_CUR);

			size -= ch_size+8;
		}

		if (dib)
			freeimage.unload_proc(dib);
	}

	return 0;
}

// ==========================================================
//   Init
// ==========================================================

void DLL_CALLCONV
InitIFF(Plugin &plugin, int format_id) {
	s_format_id = format_id;

	plugin.format_proc = Format;
	plugin.description_proc = Description;
	plugin.extension_proc = Extension;
	plugin.regexpr_proc = RegExpr;
	plugin.load_proc = Load;
	plugin.validate_proc = Validate;
}
