// ==========================================================
// MNG Loader
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

#include "FreeImage.h"
#include "Utilities.h"

#include <stdio.h>
#include <stdlib.h>

#include "../LibMNG/libmng.h"

// ----------------------------------------------------------
//   Constants + headers
// ----------------------------------------------------------

typedef struct {
	FIBITMAP    *bitmap;    // pointer to the bitmap data
	FreeImage   &freeimage; // pointer to the freeimage access functions
	FreeImageIO &io;        // pointer to the io functions
	fi_handle   file;	    // pointer to the file we're decoding
} mngstuff;

// ----------------------------------------------------------
//   Callbacks for the mng decoder
// ----------------------------------------------------------

mng_ptr
mymngalloc(mng_uint32 size) {
	return (mng_ptr)calloc(1, size);
}

void
mymngfree(mng_ptr p, mng_uint32 size) {
	free(p);
}

mng_bool
mymngopenstream(mng_handle mng) {
	// since the user is responsible for opening and closing the file,
	// we leave the default implementation open

	return MNG_TRUE;
}

mng_bool
mymngclosestream(mng_handle mng) {
	// since the user is responsible for opening and closing the file,
	// we leave the default implementation open

	return MNG_TRUE;
}

mng_bool
mymngreadstream(mng_handle mng, mng_ptr buffer, mng_uint32 size, mng_uint32 *bytesread) {
	mngstuff *mymng = (mngstuff *)mng_get_userdata(mng);

	*bytesread = mymng->io.read_proc(buffer, 1, size, mymng->file);

	return MNG_TRUE;
}

mng_bool
mymngprocessheader(mng_handle mng, mng_uint32 width, mng_uint32 height) {
	// allocate a bitmap with the given dimensions

	FreeImage &freeimage = ((mngstuff *)mng_get_userdata(mng))->freeimage;

	((mngstuff *)mng_get_userdata(mng))->bitmap = freeimage.allocate_proc(width, height, 24, 0xFF, 0xFF00, 0xFF0000);

	// tell the mng decoder about our bit-depth choice

	mng_set_canvasstyle(mng, MNG_CANVAS_BGR8);

	return MNG_TRUE;
}

mng_ptr
mymnggetcanvasline(mng_handle mng, mng_uint32 line) {
	FreeImage &freeimage = ((mngstuff *)mng_get_userdata(mng))->freeimage;

	FIBITMAP *bitmap = ((mngstuff *)mng_get_userdata(mng))->bitmap;

	return freeimage.get_scanline_proc(bitmap, freeimage.get_height_proc(bitmap) - line - 1);
}

mng_bool
mymngrefresh(mng_handle mng, mng_uint32 x, mng_uint32 y, mng_uint32 w, mng_uint32 h) {
	return MNG_TRUE;
}

mng_uint32
mymnggetticks(mng_handle mng) {
	return 0;
}

mng_bool
mymngsettimer(mng_handle mng, mng_uint32 msecs) {
	return MNG_TRUE;
}

mng_bool
mymngerror(mng_handle mng, mng_int32 code, mng_int8 severity, mng_chunkid chunktype, mng_uint32 chunkseq, mng_int32 extra1, mng_int32 extra2, mng_pchar text) {
	throw (const char *)text;
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
	return "MNG";
}

static const char * DLL_CALLCONV
Description() {
	return "Multiple Network Graphics";
}

static const char * DLL_CALLCONV
Extension() {
	return "mng";
}

static const char * DLL_CALLCONV
RegExpr() {
	return NULL;
}

// ----------------------------------------------------------

static void * DLL_CALLCONV
Open(FreeImageIO &io, fi_handle handle, BOOL read) {
	mngstuff *mymng = (mngstuff *)calloc(1, sizeof(*mymng));
	mymng->io = io;
	mymng->file = handle;

	return mymng;
}

static void DLL_CALLCONV
Close(FreeImageIO &io, fi_handle handle, void *data) {
	free((mngstuff *)data);
}

// ----------------------------------------------------------

static FIBITMAP * DLL_CALLCONV
Load(FreeImage &freeimage, FreeImageIO &io, fi_handle handle, int page, int flags, void *data) {
	if (handle != NULL) {
		try {
			// allocate our stream data structure

			mngstuff *mymng = (mngstuff *)data;

			// set up the mng decoder for our stream

			mng_handle mng = mng_initialize(mymng, mymngalloc, mymngfree, MNG_NULL);

			if (mng == MNG_NULL)
				throw "could not initialize libmng";			

			// set the callbacks

			mng_setcb_errorproc(mng, mymngerror);
			mng_setcb_openstream(mng, mymngopenstream);
			mng_setcb_closestream(mng, mymngclosestream);
			mng_setcb_readdata(mng, mymngreadstream);
			mng_setcb_processheader(mng, mymngprocessheader);
			mng_setcb_getcanvasline(mng, mymnggetcanvasline);
			mng_setcb_refresh(mng, mymngrefresh);
			mng_setcb_gettickcount(mng, mymnggetticks);
			mng_setcb_settimer(mng, mymngsettimer);

			// read in the bitmap

			mng_readdisplay(mng);

			// temp store the newly created bitmap

			FIBITMAP *bitmap = mymng->bitmap;

			// cleanup and return the temp stored bitmap

			mng_cleanup(&mng);

			return bitmap;
		} catch (const char *message) {
			freeimage.output_message_proc(s_format_id, message);
		}
	}

	return NULL;
}	

// ==========================================================
//   Init
// ==========================================================

void DLL_CALLCONV
InitMNG(Plugin &plugin, int format_id) {
	s_format_id = format_id;

	plugin.format_proc = Format;
	plugin.description_proc = Description;
	plugin.extension_proc = Extension;
	plugin.regexpr_proc = RegExpr;
	plugin.open_proc = Open;
	plugin.close_proc = Close;
	plugin.load_proc = Load;
}
