// ==========================================================
// PNG Loader and Writer
//
// Design and implementation by
// - Floris van den Berg (flvdberg@wxs.nl)
// - Herve Drolon (drolon@infonie.fr)
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

#include <stdlib.h>
#include <memory.h> 

// ----------------------------------------------------------

#define PNG_ASSEMBLER_CODE_SUPPORTED
#define PNG_BYTES_TO_CHECK 8

// ----------------------------------------------------------

#include "../LibPNG/png.h"

// ----------------------------------------------------------

static FreeImageIO *s_io;
static fi_handle s_handle;

/////////////////////////////////////////////////////////////////////////////
// libpng interface 
// 

static void
_ReadProc(struct png_struct_def *, unsigned char *data, unsigned int size) {
	s_io->read_proc(data, size, 1, s_handle);
}

static void
_WriteProc(struct png_struct_def *, unsigned char *data, unsigned int size) {
	s_io->write_proc(data, size, 1, s_handle);
}

static void
_FlushProc(png_structp png_ptr) {
	// empty flush implementation
}

static void
error_handler(struct png_struct_def *, const char *error) {
	throw error;
}

// in FreeImage warnings disabled

static void
warning_handler(struct png_struct_def *, const char *warning) {
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
	return "PNG";
}

static const char * DLL_CALLCONV
Description() {
	return "Portable Network Graphics";
}

static const char * DLL_CALLCONV
Extension() {
	return "png";
}

static const char * DLL_CALLCONV
RegExpr() {
	return "^.PNG\r";
}

static const char * DLL_CALLCONV
MimeType() {
	return "image/png";
}

static BOOL DLL_CALLCONV
Validate(FreeImageIO &io, fi_handle handle) {
	BYTE png_signature[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
	BYTE signature[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

	io.read_proc(&signature, 1, 8, handle);

	return (memcmp(png_signature, signature, 8) == 0);
}

// ----------------------------------------------------------

static FIBITMAP * DLL_CALLCONV
Load(FreeImage &freeimage, FreeImageIO &io, fi_handle handle, int page, int flags, void *data) {
	png_structp png_ptr;
	png_infop info_ptr;
	png_uint_32 width, height;
	png_colorp png_palette;
	int bpp, color_type, palette_entries;

	FIBITMAP *dib = NULL;
	RGBQUAD *palette;	// pointer to dib palette
	png_bytepp  row_pointers = NULL;
	int i;

	s_io = &io;
	s_handle = handle;

	if (handle) {
		try {		
			// check to see if the file is in fact a PNG file

			unsigned char png_check[PNG_BYTES_TO_CHECK];

			io.read_proc(png_check, 1, PNG_BYTES_TO_CHECK, handle);

			if (png_sig_cmp(png_check, (png_size_t)0, PNG_BYTES_TO_CHECK) != 0)
				return NULL;	// Bad signature
			
			// create the chunk manage structure

			png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, (png_voidp)error_handler, error_handler, warning_handler);

			if (!png_ptr)
				return NULL;			

			// create the info structure

		    info_ptr = png_create_info_struct(png_ptr);

			if (!info_ptr) {
				png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
				return NULL;
			}

			// init the IO

			png_set_read_fn(png_ptr, info_ptr, _ReadProc);

			if (setjmp(png_jmpbuf(png_ptr))) {
				png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
				return NULL;
			}

			// Because we have already read the signature...

			png_set_sig_bytes(png_ptr, PNG_BYTES_TO_CHECK);

			// read the IHDR chunk

			png_read_info(png_ptr, info_ptr);
			png_get_IHDR(png_ptr, info_ptr, &width, &height, &bpp, &color_type, NULL, NULL, NULL);

			// DIB's don't support >8 bits per sample
			// => tell libpng to strip 16 bit/color files down to 8 bits/color

			if (bpp == 16) {
				png_set_strip_16(png_ptr);
				bpp = 8;
			}

			// Set some additional flags

			switch(color_type) {
				case PNG_COLOR_TYPE_RGB:
				case PNG_COLOR_TYPE_RGB_ALPHA:
					// Flip the RGB pixels to BGR (or RGBA to BGRA)

					png_set_bgr(png_ptr);
					break;

				case PNG_COLOR_TYPE_PALETTE:
					// Expand palette images to the full 8 bits from 2 or 4 bits/pixel

					if ((bpp == 2) || (bpp == 4)) {
						png_set_packing(png_ptr);
						bpp = 8;
					}					

					break;

				case PNG_COLOR_TYPE_GRAY:
				case PNG_COLOR_TYPE_GRAY_ALPHA:
					// Expand grayscale images to the full 8 bits from 2 or 4 bits/pixel

					if ((bpp == 2) || (bpp == 4)) {
						png_set_expand(png_ptr);
						bpp = 8;
					}

					break;

				default:
					throw "PNG format not supported";
			}

			// Set the background color to draw transparent and alpha images over.
			// It is possible to set the red, green, and blue components directly
			// for paletted images instead of supplying a palette index.  Note that
			// even if the PNG file supplies a background, you are not required to
			// use it - you should use the (solid) application background if it has one.

			if (color_type != PNG_COLOR_TYPE_RGB_ALPHA) {
				png_color_16 my_background= { 0, 255, 255, 255, 0 };
				png_color_16 *image_background;

				if (png_get_bKGD(png_ptr, info_ptr, &image_background))
					png_set_background(png_ptr, image_background, PNG_BACKGROUND_GAMMA_FILE, 1, 1.0);
				else
					png_set_background(png_ptr, &my_background, PNG_BACKGROUND_GAMMA_SCREEN, 0, 1.0);
			}
			
			// if this image has transparency, store the trns values

			png_bytep trans               = NULL;
			int num_trans                 = 0;
			png_color_16p trans_values    = NULL;
			png_uint_32 transparent_value = png_get_tRNS(png_ptr, info_ptr, &trans, &num_trans, &trans_values);

			// unlike the example in the libpng documentation, we have *no* idea where
			// this file may have come from--so if it doesn't have a file gamma, don't
			// do any correction ("do no harm")

			double gamma = 0;
			double screen_gamma = 2.2;

			if (png_get_gAMA(png_ptr, info_ptr, &gamma))
				png_set_gamma(png_ptr, screen_gamma, gamma);

			// All transformations have been registered; now update info_ptr data

			png_read_update_info(png_ptr, info_ptr);

			// Create a DIB and write the bitmap header
			// set up the DIB palette, if needed

			switch (color_type) {
				case PNG_COLOR_TYPE_RGB:
					png_set_invert_alpha(png_ptr);

					dib = freeimage.allocate_proc(width, height, 24, 0xFF, 0xFF00, 0xFF0000);
					break;

				case PNG_COLOR_TYPE_RGB_ALPHA :
					dib = freeimage.allocate_proc(width, height, 32, 0xFF, 0xFF00, 0xFF0000);
					break;

				case PNG_COLOR_TYPE_PALETTE :
					dib = freeimage.allocate_proc(width, height, bpp, 0, 0, 0);

					png_get_PLTE(png_ptr,info_ptr, &png_palette,&palette_entries);

					palette = freeimage.get_palette_proc(dib);

					// store the palette

					for (i = 0; i < palette_entries; i++) {
						palette[i].rgbRed   = png_palette[i].red;
						palette[i].rgbGreen = png_palette[i].green;
						palette[i].rgbBlue  = png_palette[i].blue;
					}

					// store the transparency table

					if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
						freeimage.set_transparency_table_proc(dib, (BYTE *)trans, num_trans);					

					break;

				case PNG_COLOR_TYPE_GRAY:
				case PNG_COLOR_TYPE_GRAY_ALPHA:
					dib = freeimage.allocate_proc(width, height, bpp, 0, 0, 0);

					palette = freeimage.get_palette_proc(dib);
					palette_entries = 1 << bpp;

					for (i = 0; i < palette_entries; i++) {
						palette[i].rgbRed   =
						palette[i].rgbGreen =
						palette[i].rgbBlue  = (i * 255) / (palette_entries - 1);
					}

					// store the transparency table

					if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
						freeimage.set_transparency_table_proc(dib, (BYTE *)trans, num_trans);					

					break;
			}

			// DIBs *do* support physical resolution

			if (png_get_valid(png_ptr,info_ptr,PNG_INFO_pHYs)) {
				png_uint_32 res_x, res_y;
				
				// We'll overload this var and use 0 to mean no phys data,
				// since if it's not in meters we can't use it anyway

				int res_unit_type = 0;

				png_get_pHYs(png_ptr,info_ptr,&res_x,&res_y,&res_unit_type);

				if (res_unit_type == 1) {
					BITMAPINFOHEADER *bih = freeimage.get_info_header_proc(dib);

					bih->biXPelsPerMeter = res_x;
					bih->biYPelsPerMeter = res_y;
				}
			}

			// set the individual row_pointers to point at the correct offsets

			row_pointers = (png_bytepp)malloc(height * sizeof(png_bytep));

			if (!row_pointers) {
				if (palette)
					png_free(png_ptr, palette);				

				png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

				freeimage.free_proc(dib);
				free(row_pointers);
				return NULL;
			}

			// read in the bitmap bits via the pointer table

			for (png_uint_32 k = 0; k < height; k++)				
				row_pointers[height - 1 - k] = freeimage.get_scanline_proc(dib, k);			

			png_read_image(png_ptr, row_pointers);

			// check if the bitmap contains transparency, if so enable it in the header

			if (freeimage.get_bpp_proc(dib) == 32)
				if (freeimage.get_color_type_proc(dib) == FIC_RGBALPHA)
					freeimage.set_transparent_proc(dib, TRUE);
				else
					freeimage.set_transparent_proc(dib, FALSE);
				
			// cleanup

			if (row_pointers)
				free(row_pointers);

			png_read_end(png_ptr, info_ptr);

			if (png_ptr)
				png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);

			return dib;
		} catch (const char *text) {
			if (png_ptr)
				png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
			
			if (row_pointers)
				free(row_pointers);

			if (dib)
				freeimage.free_proc(dib);			

			freeimage.output_message_proc(s_format_id, text);
			
			return NULL;
		}
	}			

	return NULL;
}

static BOOL DLL_CALLCONV
Save(FreeImage &freeimage, FreeImageIO &io, FIBITMAP *dib, fi_handle handle, int page, int flags, void *data) {
	png_structp png_ptr;
	png_infop info_ptr;
	png_colorp palette = NULL;
	png_uint_32 width, height, bpp;
	BOOL has_alpha_channel = FALSE;

	RGBQUAD *pal;	// pointer to dib palette
	int bit_depth;
	int palette_entries;
	int	interlace_type;

	s_io = &io;
	s_handle = handle;

	if ((dib) && (handle)) {
		try {
			// create the chunk manage structure

			png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, (png_voidp)error_handler, error_handler, warning_handler);

			if (!png_ptr)  {
				return FALSE;
			}

			// Allocate/initialize the image information data.

			info_ptr = png_create_info_struct(png_ptr);

			if (!info_ptr)  {
				png_destroy_write_struct(&png_ptr,  (png_infopp)NULL);
				return FALSE;
			}

			// Set error handling.  REQUIRED if you aren't supplying your own
			// error handling functions in the png_create_write_struct() call.

			if (setjmp(png_jmpbuf(png_ptr)))  {
				// If we get here, we had a problem reading the file

				png_destroy_write_struct(&png_ptr, &info_ptr);

				return FALSE;
			}

			// init the IO

			png_set_write_fn(png_ptr, info_ptr, _WriteProc, _FlushProc);

			// DIBs *do* support physical resolution

			BITMAPINFOHEADER *bih = freeimage.get_info_header_proc(dib);
			png_uint_32 res_x = bih->biXPelsPerMeter;
			png_uint_32 res_y = bih->biYPelsPerMeter;

			if ((res_x > 0) && (res_y > 0))  {
				png_set_pHYs(png_ptr, info_ptr, res_x, res_y, 1);
			}
	
			// Set the image information here.  Width and height are up to 2^31,
			// bit_depth is one of 1, 2, 4, 8, or 16, but valid values also depend on
			// the color_type selected. color_type is one of PNG_COLOR_TYPE_GRAY,
			// PNG_COLOR_TYPE_GRAY_ALPHA, PNG_COLOR_TYPE_PALETTE, PNG_COLOR_TYPE_RGB,
			// or PNG_COLOR_TYPE_RGB_ALPHA.  interlace is either PNG_INTERLACE_NONE or
			// PNG_INTERLACE_ADAM7, and the compression_type and filter_type MUST
			// currently be PNG_COMPRESSION_TYPE_BASE and PNG_FILTER_TYPE_BASE. REQUIRED

			width = freeimage.get_width_proc(dib);
			height = freeimage.get_height_proc(dib);
			bpp = bit_depth = freeimage.get_bpp_proc(dib);			

			if (bit_depth == 16) {
				png_destroy_write_struct(&png_ptr, &info_ptr);

				throw "Format not supported";	// Note: this could be enhanced here...
			}

			bit_depth = (bit_depth > 8) ? 8 : bit_depth;

			interlace_type = PNG_INTERLACE_NONE;	// Default value

			switch (freeimage.get_color_type_proc(dib)) {
				case FIC_MINISWHITE:
					// Invert monochrome files to have 0 as black and 1 as white
					// (no break here)
					png_set_invert_mono(png_ptr);

				case FIC_MINISBLACK:
					png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth, 
						PNG_COLOR_TYPE_GRAY, interlace_type, 
						PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
					break;

				case FIC_PALETTE:
				{
					png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth, 
						PNG_COLOR_TYPE_PALETTE, interlace_type, 
						PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

					// set the palette

					palette_entries = 1 << bit_depth;
					palette = (png_colorp)png_malloc(png_ptr, palette_entries * sizeof (png_color));
					pal = freeimage.get_palette_proc(dib);

					for (int i = 0; i < palette_entries; i++) {
						palette[i].red   = pal[i].rgbRed;
						palette[i].green = pal[i].rgbGreen;
						palette[i].blue  = pal[i].rgbBlue;
					}
					
					png_set_PLTE(png_ptr, info_ptr, palette, palette_entries);

					// You must not free palette here, because png_set_PLTE only makes a link to
					// the palette that you malloced.  Wait until you are about to destroy
					// the png structure.

					break;
				}

				case FIC_RGBALPHA :
					if (freeimage.is_transparent_proc(dib)) {
						has_alpha_channel = TRUE;

						png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth, 
							PNG_COLOR_TYPE_RGBA, interlace_type, 
							PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

						png_set_bgr(png_ptr); // flip BGR pixels to RGB
						break;
					}

					// intentionally no break here...
					
				case FIC_RGB:
					png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth, 
						PNG_COLOR_TYPE_RGB, interlace_type, 
						PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

					png_set_bgr(png_ptr); // flip BGR pixels to RGB
					break;
			}

			// Optional gamma chunk is strongly suggested if you have any guess
			// as to the correct gamma of the image.
			// png_set_gAMA(png_ptr, info_ptr, gamma);

			if ((freeimage.get_bpp_proc(dib) == 8) && (freeimage.is_transparent_proc(dib)) && (freeimage.get_transparency_count_proc(dib) > 0))
				png_set_tRNS(png_ptr, info_ptr, freeimage.get_transparency_table_proc(dib), freeimage.get_transparency_count_proc(dib), NULL);
			
			// Write the file header information.

			png_write_info(png_ptr, info_ptr);

			// write out the image data

			if ((bpp == 32) && (!has_alpha_channel)) {
				BYTE *buffer = (BYTE *)malloc(width * 3);

				for (png_uint_32 k = 0; k < height; k++) {
					freeimage.convert_line_32to24_proc(buffer, freeimage.get_scanline_proc(dib, height - k - 1), width);

					png_write_row(png_ptr, buffer);
				}

				free(buffer);
			} else {
				for (png_uint_32 k = 0; k < height; k++) {
					png_write_row(png_ptr, freeimage.get_scanline_proc(dib, height - k - 1));
				}
			}

			// It is REQUIRED to call this to finish writing the rest of the file
			// Bug with png_flush

			png_write_end(png_ptr, info_ptr);

			// clean up after the write, and free any memory allocated

			png_destroy_write_struct(&png_ptr, &info_ptr);

			if (palette)
				png_free(png_ptr, palette);			

			return TRUE;
		} catch (const char *text) {
			freeimage.output_message_proc(s_format_id, text);
		}
	}

	return FALSE;
}

// ==========================================================
//   Init
// ==========================================================

void DLL_CALLCONV
InitPNG(Plugin &plugin, int format_id) {
	s_format_id = format_id;

	plugin.format_proc = Format;
	plugin.description_proc = Description;
	plugin.extension_proc = Extension;
	plugin.regexpr_proc = RegExpr;
	plugin.pagecount_proc = NULL;
	plugin.pagecapability_proc = NULL;
	plugin.load_proc = Load;
	plugin.save_proc = 0;	//Save;
	plugin.validate_proc = Validate;
	plugin.mime_proc = MimeType;
}
