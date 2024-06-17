// ==========================================================
// JPEG Loader
//
// Design and implementation by
// - Floris van den Berg (flvdberg@wxs.nl)
// - Jan L. Nauta (jln@magentammt.com)
// - Markus Loibl (markus.loibl@epost.de)
//
// Based on code developed by The Independent JPEG Group
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

extern "C" {
#define XMD_H
#undef FAR
#include <setjmp.h>
#include "../LibJPEG/jpeglib.h"
}

// ----------------------------------------------------------

#define INPUT_BUF_SIZE  4096	/* choose an efficiently fread'able size */
#define OUTPUT_BUF_SIZE 4096    /* choose an efficiently fwrite'able size */

typedef struct my_error_mgr * my_error_ptr;

// ----------------------------------------------------------
//   Constants + headers
// ----------------------------------------------------------

METHODDEF(void)
jpeg_error_exit (j_common_ptr cinfo) {
	(*cinfo->err->output_message)(cinfo);

	jpeg_destroy(cinfo);

	throw FIF_JPEG;
}

METHODDEF(void)
jpeg_output_message (j_common_ptr cinfo) {
	char buffer[JMSG_LENGTH_MAX];

	(*cinfo->err->format_message)(cinfo, buffer);

	//FreeImage_OutputMessage(FreeImage_GetFIFFromFormat("JPEG"), buffer);
}

//===========================================================
/*
 * jdatasrc.c
 *
 * Copyright (C) 1994-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains decompression data source routines for the case of
 * reading JPEG data from a file (or any stdio stream).  While these routines
 * are sufficient for most applications, some will want to use a different
 * source manager.
 * IMPORTANT: we assume that fread() will correctly transcribe an array of
 * JOCTETs from 8-bit-wide elements on external storage.  If char is wider
 * than 8 bits on your machine, you may need to do some tweaking.
 */

#include "../libjpeg/jinclude.h"
#include "../libjpeg/jpeglib.h"
#include "../libjpeg/jerror.h"

// Expanded data source object for stdio input --------------

struct my_error_mgr {
	struct jpeg_error_mgr pub;	/* "public" fields */

	jmp_buf setjmp_buffer;	/* for return to caller */
};

// ----------------------------------------------------------

typedef struct {
	struct jpeg_source_mgr pub;	/* public fields */

	fi_handle infile;		/* source stream */
	FreeImageIO *m_io;

	JOCTET * buffer;		/* start of buffer */
	boolean start_of_file;	/* have we gotten any data yet? */
} my_source_mgr;

typedef struct {
	struct jpeg_destination_mgr pub;	/* public fields */

	fi_handle outfile;		/* destination stream */
	FreeImageIO *m_io;

	JOCTET * buffer;		/* start of buffer */
} my_destination_mgr;

// ----------------------------------------------------------

typedef my_source_mgr * freeimage_src_ptr;
typedef my_destination_mgr * freeimage_dst_ptr;

// ----------------------------------------------------------

METHODDEF(void)
init_source (j_decompress_ptr cinfo) {
	freeimage_src_ptr src = (freeimage_src_ptr) cinfo->src;

	/* We reset the empty-input-file flag for each image,
 	 * but we don't clear the input buffer.
	 * This is correct behavior for reading a series of images from one source.
	*/

	src->start_of_file = TRUE;
}

METHODDEF(void)
init_destination (j_compress_ptr cinfo) {
	freeimage_dst_ptr dest = (freeimage_dst_ptr) cinfo->dest;

	dest->buffer = (JOCTET *)
	  (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				  OUTPUT_BUF_SIZE * SIZEOF(JOCTET));

	dest->pub.next_output_byte = dest->buffer;
	dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
}

// ----------------------------------------------------------

/*
 * Fill the input buffer --- called whenever buffer is emptied.
 *
 * In typical applications, this should read fresh data into the buffer
 * (ignoring the current state of next_input_byte & bytes_in_buffer),
 * reset the pointer & count to the start of the buffer, and return TRUE
 * indicating that the buffer has been reloaded.  It is not necessary to
 * fill the buffer entirely, only to obtain at least one more byte.
 *
 * There is no such thing as an EOF return.  If the end of the file has been
 * reached, the routine has a choice of ERREXIT() or inserting fake data into
 * the buffer.  In most cases, generating a warning message and inserting a
 * fake EOI marker is the best course of action --- this will allow the
 * decompressor to output however much of the image is there.  However,
 * the resulting error message is misleading if the real problem is an empty
 * input file, so we handle that case specially.
 *
 * In applications that need to be able to suspend compression due to input
 * not being available yet, a FALSE return indicates that no more data can be
 * obtained right now, but more may be forthcoming later.  In this situation,
 * the decompressor will return to its caller (with an indication of the
 * number of scanlines it has read, if any).  The application should resume
 * decompression after it has loaded more data into the input buffer.  Note
 * that there are substantial restrictions on the use of suspension --- see
 * the documentation.
 *
 * When suspending, the decompressor will back up to a convenient restart point
 * (typically the start of the current MCU). next_input_byte & bytes_in_buffer
 * indicate where the restart point will be if the current call returns FALSE.
 * Data beyond this point must be rescanned after resumption, so move it to
 * the front of the buffer rather than discarding it.
 */

METHODDEF(boolean)
fill_input_buffer (j_decompress_ptr cinfo) {
	freeimage_src_ptr src = (freeimage_src_ptr) cinfo->src;

	size_t nbytes = src->m_io->read_proc(src->buffer, 1, INPUT_BUF_SIZE, src->infile);

	if (nbytes <= 0) {
		if (src->start_of_file)	/* Treat empty input file as fatal error */
			ERREXIT(cinfo, JERR_INPUT_EMPTY);

		WARNMS(cinfo, JWRN_JPEG_EOF);

		/* Insert a fake EOI marker */

		src->buffer[0] = (JOCTET) 0xFF;
		src->buffer[1] = (JOCTET) JPEG_EOI;

		nbytes = 2;
	}

	src->pub.next_input_byte = src->buffer;
	src->pub.bytes_in_buffer = nbytes;
	src->start_of_file = FALSE;

	return TRUE;
}

METHODDEF(boolean)
empty_output_buffer (j_compress_ptr cinfo) {
	freeimage_dst_ptr dest = (freeimage_dst_ptr) cinfo->dest;

	if (dest->m_io->write_proc(dest->buffer, 1, OUTPUT_BUF_SIZE, dest->outfile) != OUTPUT_BUF_SIZE)
		ERREXIT(cinfo, JERR_FILE_WRITE);

	dest->pub.next_output_byte = dest->buffer;
	dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;

	return TRUE;
}

/*
 * Skip data --- used to skip over a potentially large amount of
 * uninteresting data (such as an APPn marker).
 *
 * Writers of suspendable-input applications must note that skip_input_data
 * is not granted the right to give a suspension return.  If the skip extends
 * beyond the data currently in the buffer, the buffer can be marked empty so
 * that the next read will cause a fill_input_buffer call that can suspend.
 * Arranging for additional bytes to be discarded before reloading the input
 * buffer is the application writer's problem.
 */

METHODDEF(void)
skip_input_data (j_decompress_ptr cinfo, long num_bytes) {
	freeimage_src_ptr src = (freeimage_src_ptr) cinfo->src;

	/* Just a dumb implementation for now.  Could use fseek() except
     * it doesn't work on pipes.  Not clear that being smart is worth
	 * any trouble anyway --- large skips are infrequent.
	*/

	if (num_bytes > 0) {
		while (num_bytes > (long) src->pub.bytes_in_buffer) {
		  num_bytes -= (long) src->pub.bytes_in_buffer;

		  (void) fill_input_buffer(cinfo);

		  /* note we assume that fill_input_buffer will never return FALSE,
		   * so suspension need not be handled.
		   */
		}

		src->pub.next_input_byte += (size_t) num_bytes;
		src->pub.bytes_in_buffer -= (size_t) num_bytes;
	}
}

/*
 * Terminate source --- called by jpeg_finish_decompress
 * after all data has been read.  Often a no-op.
 *
 * NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
 * application must deal with any cleanup that should happen even
 * for error exit.
 */

METHODDEF(void)
term_source (j_decompress_ptr cinfo) {
  /* no work necessary here */
}

METHODDEF(void)
term_destination (j_compress_ptr cinfo) {
	freeimage_dst_ptr dest = (freeimage_dst_ptr) cinfo->dest;

	size_t datacount = OUTPUT_BUF_SIZE - dest->pub.free_in_buffer;

	/* Write any data remaining in the buffer */

	if (datacount > 0) {
		if (dest->m_io->write_proc(dest->buffer, 1, datacount, dest->outfile) != datacount)
		  ERREXIT(cinfo, JERR_FILE_WRITE);
	}
}

/*
 * Prepare for input from a stdio stream.
 * The caller must have already opened the stream, and is responsible
 * for closing it after finishing decompression.
 */

GLOBAL(void)
jpeg_freeimage_src (j_decompress_ptr cinfo, fi_handle infile, FreeImageIO &io) {
	freeimage_src_ptr src;

	// allocate memory for the buffer. is released automatically in the end

	if (cinfo->src == NULL) {
		cinfo->src = (struct jpeg_source_mgr *) (*cinfo->mem->alloc_small)
			((j_common_ptr) cinfo, JPOOL_PERMANENT, SIZEOF(my_source_mgr));

		src = (freeimage_src_ptr) cinfo->src;

		src->buffer = (JOCTET *) (*cinfo->mem->alloc_small)
			((j_common_ptr) cinfo, JPOOL_PERMANENT, INPUT_BUF_SIZE * SIZEOF(JOCTET));
	}

	// initialize the jpeg pointer struct with pointers to functions
	
	src = (freeimage_src_ptr) cinfo->src;
	src->pub.init_source = init_source;
	src->pub.fill_input_buffer = fill_input_buffer;
	src->pub.skip_input_data = skip_input_data;
	src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
	src->pub.term_source = term_source;
	src->infile = infile;
	src->m_io = &io;
	src->pub.bytes_in_buffer = 0; /* forces fill_input_buffer on first read */
	src->pub.next_input_byte = NULL; /* until buffer loaded */
}

GLOBAL(void)
jpeg_freeimage_dst (j_compress_ptr cinfo, fi_handle outfile, FreeImageIO &io) {
	freeimage_dst_ptr dest;

	if (cinfo->dest == NULL) {
		cinfo->dest = (struct jpeg_destination_mgr *)(*cinfo->mem->alloc_small)
			((j_common_ptr) cinfo, JPOOL_PERMANENT, SIZEOF(my_destination_mgr));
	}

	dest = (freeimage_dst_ptr) cinfo->dest;
	dest->pub.init_destination = init_destination;
	dest->pub.empty_output_buffer = empty_output_buffer;
	dest->pub.term_destination = term_destination;
	dest->outfile = outfile;
	dest->m_io = &io;
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
	return "JPEG";
}

static const char * DLL_CALLCONV
Description() {
	return "JPEG - JFIF Compliant";
}

static const char * DLL_CALLCONV
Extension() {
	return "jpg,jif,jpeg";
}

static const char * DLL_CALLCONV
RegExpr() {
	return "^\377\330\377";
}

static const char * DLL_CALLCONV
MimeType() {
	return "image/jpeg";
}

static BOOL DLL_CALLCONV
Validate(FreeImageIO &io, fi_handle handle) {
	BYTE jpeg_signature[] = { 0xFF, 0xD8 };
	BYTE signature[2] = { 0, 0 };

	io.read_proc(signature, 1, sizeof(jpeg_signature), handle);

	return (memcmp(jpeg_signature, signature, sizeof(jpeg_signature)) == 0);
}

// ----------------------------------------------------------

static FIBITMAP * DLL_CALLCONV
Load(FreeImage &freeimage, FreeImageIO &io, fi_handle handle, int page, int flags, void *data) {
	if (handle) {
		FIBITMAP *dib = NULL;
	
		try {
			struct jpeg_decompress_struct cinfo;
			struct jpeg_error_mgr jerr;

			// Step 1: allocate and initialize JPEG decompression object

			cinfo.err = jpeg_std_error(&jerr);

			jerr.error_exit     = jpeg_error_exit;
			jerr.output_message = jpeg_output_message;

			jpeg_create_decompress(&cinfo);

			// Step 2: specify data source (eg, a handle)

			jpeg_freeimage_src(&cinfo, handle, io);

			// Step 3: read handle parameters with jpeg_read_header()

			jpeg_read_header(&cinfo, TRUE);

			// Step 4a: set parameters for decompression

			if ((flags != JPEG_ACCURATE)) {
				cinfo.dct_method          = JDCT_IFAST;
				cinfo.do_fancy_upsampling = FALSE;
			}

			// Step 4b: allocate dib and init header

			dib = freeimage.allocate_proc(cinfo.image_width, cinfo.image_height, 8 * cinfo.num_components, 0xFF00, 0xFF, 0xFF0000);

			if (cinfo.num_components == 1) {
				RGBQUAD *colors = freeimage.get_palette_proc(dib);

				for (int i = 0; i < 256; i++) {
					colors[i].rgbRed   = i;
					colors[i].rgbGreen = i;
					colors[i].rgbBlue  = i;
				}
			}

			// Step 4c: handle metrices

			BITMAPINFOHEADER *pInfoHeader = freeimage.get_info_header_proc(dib);

			if (cinfo.density_unit == 1) {
				// dots/inch

				pInfoHeader->biXPelsPerMeter = (int) (((float)cinfo.X_density) / 0.0254000 + 0.5);
				pInfoHeader->biYPelsPerMeter = (int) (((float)cinfo.Y_density) / 0.0254000 + 0.5);
			} else if (cinfo.density_unit == 2) {
				// dots/cm

				pInfoHeader->biXPelsPerMeter = cinfo.X_density * 100;
				pInfoHeader->biYPelsPerMeter = cinfo.Y_density * 100;
			}
			
			// Step 5: start decompressor

			jpeg_start_decompress(&cinfo);

			// Step 6: while (scan lines remain to be read) jpeg_read_scanlines(...);

			while (cinfo.output_scanline < cinfo.output_height) {
				JSAMPROW b = freeimage.get_scanline_proc(dib, cinfo.output_height - cinfo.output_scanline - 1);

				jpeg_read_scanlines(&cinfo, &b, 1);
			}

			// Step 7: finish decompression 

			jpeg_finish_decompress(&cinfo);

			// Step 8: release JPEG decompression object

			jpeg_destroy_decompress(&cinfo);

			return (FIBITMAP *)dib;
		} catch (...) {
			freeimage.unload_proc(dib);
		}		
	}

	return NULL;
}

static BOOL DLL_CALLCONV
Save(FreeImage &freeimage, FreeImageIO &io, FIBITMAP *dib, fi_handle handle, int page, int flags, void *data) {
	if ((dib) && (handle)) {
		try {
			// Check dib format

			WORD bpp = freeimage.get_bpp_proc(dib);

			if ((bpp == 32) && (freeimage.is_transparent_proc(dib)))
				throw "JPEG does not support alpha channels";
			else if (((bpp != 24) && (bpp != 32)) && (!((bpp == 8) && (FreeImage_GetColorType(dib) == FIC_MINISBLACK))))
				throw "only 24-bit highcolor or 8-bit greyscale bitmaps can be saved as JPEG";

			struct jpeg_compress_struct cinfo;
			struct jpeg_error_mgr jerr;

			// Step 1: allocate and initialize JPEG compression object

			cinfo.err = jpeg_std_error(&jerr);

			jerr.error_exit     = jpeg_error_exit;
			jerr.output_message = jpeg_output_message;

			/* Now we can initialize the JPEG compression object. */

			jpeg_create_compress(&cinfo);

			/* Step 2: specify data destination (eg, a file) */

			jpeg_freeimage_dst(&cinfo, handle, io);
			
			/* Step 3: set parameters for compression */											    

			cinfo.image_width = freeimage.get_width_proc(dib);
			cinfo.image_height = freeimage.get_height_proc(dib);

			switch(FreeImage_GetColorType(dib)) {
				case FIC_MINISBLACK :
					cinfo.in_color_space = JCS_GRAYSCALE;
					cinfo.input_components = 1;
					break;

				default :
					cinfo.in_color_space = JCS_RGB;
					cinfo.input_components = 3;
					break;
			}

			jpeg_set_defaults(&cinfo);

			// Set JFIF density parameters from the DIB data

			BITMAPINFOHEADER *pInfoHeader = freeimage.get_info_header_proc(dib);
			cinfo.X_density = (WORD)(pInfoHeader->biXPelsPerMeter / 100.0F + 0.5);
			cinfo.Y_density = (WORD)(pInfoHeader->biYPelsPerMeter / 100.0F + 0.5);
			cinfo.density_unit = 2;	// dots / cm

			// Step 4: set quality
			// the first 7 bits are reserved for low level quality settings
			// the other bits are high level (i.e. enum-ish)

			int quality;

			if ((flags & JPEG_QUALITYBAD) == JPEG_QUALITYBAD) {
				quality = 10;
			} else if ((flags & JPEG_QUALITYAVERAGE) == JPEG_QUALITYAVERAGE) {
				quality = 25;
			} else if ((flags & JPEG_QUALITYNORMAL) == JPEG_QUALITYNORMAL) {
				quality = 50;
			} else if ((flags & JPEG_QUALITYGOOD) == JPEG_QUALITYGOOD) {
				quality = 75;
			} else 	if ((flags & JPEG_QUALITYSUPERB) == JPEG_QUALITYSUPERB) {
				quality = 100;
			} else {
				if ((flags & 0x7F) == 0) {
					quality = 75;
				} else {
					quality = flags & 0x7F;
				}
			}

			jpeg_set_quality(&cinfo, quality, TRUE); /* limit to baseline-JPEG values */

			/* Step 5: Start compressor */

			jpeg_start_compress(&cinfo, TRUE);

			/* Step 6: while (scan lines remain to be written) */

			if (freeimage.get_bpp_proc(dib) == 32) {
				int pitch = CalculatePitch(CalculateLine(freeimage.get_width_proc(dib), 24));

				JSAMPROW b = (BYTE *)malloc(pitch);

				for (unsigned i = 0; i < freeimage.get_height_proc(dib); ++i) {
					freeimage.convert_line_32to24_proc(b, freeimage.get_scanline_proc(dib, freeimage.get_height_proc(dib) - cinfo.next_scanline - 1), freeimage.get_width_proc(dib));

					jpeg_write_scanlines(&cinfo, &b, 1);
				}

				free(b);
			} else {
				while (cinfo.next_scanline < cinfo.image_height) {
					JSAMPROW b = freeimage.get_scanline_proc(dib, freeimage.get_height_proc(dib) - cinfo.next_scanline - 1);

					jpeg_write_scanlines(&cinfo, &b, 1);
				}
			}

			/* Step 7: Finish compression */

			jpeg_finish_compress(&cinfo);

			/* Step 8: release JPEG compression object */

			jpeg_destroy_compress(&cinfo);
			
			return TRUE;

		} catch (char *text) {
			freeimage.output_message_proc(s_format_id, text);

			return FALSE;
		} catch (FREE_IMAGE_FORMAT) {
			return FALSE;
		}
	}

	return FALSE;
}

// ==========================================================
//   Init
// ==========================================================

void DLL_CALLCONV
InitJPEG(Plugin &plugin, int format_id) {
	s_format_id = format_id;

	plugin.format_proc = Format;
	plugin.description_proc = Description;
	plugin.extension_proc = Extension;
	plugin.regexpr_proc = RegExpr;
	plugin.load_proc = Load;
	plugin.save_proc = 0;	//Save;
	plugin.validate_proc = Validate;
	plugin.mime_proc = MimeType;
}
