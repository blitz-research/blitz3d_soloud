// ==========================================================
// GetType
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

#include <stdio.h>
#include <string.h>

#include "FreeImage.h"
#include "FreeImageIO.h"
#include "Plugin.h"

// ----------------------------------------------------------

FREE_IMAGE_FORMAT DLL_CALLCONV
FreeImage_GetFileType(const char *filename, int size) {
	FreeImageIO io;
	SetDefaultIO(&io);
	
	FILE *handle = fopen(filename, "rb");

	if (handle != NULL) {
		FREE_IMAGE_FORMAT format = FreeImage_GetFileTypeFromHandle(&io, (fi_handle)handle, size);

		fclose(handle);

		return format;
	} else {
		return FIF_UNKNOWN;
	}
}

// ----------------------------------------------------------

FREE_IMAGE_FORMAT DLL_CALLCONV
FreeImage_GetFileTypeFromHandle(FreeImageIO *io, fi_handle handle, int size) {
	if (handle != NULL) {
		for (int i = FreeImage_GetFIFCount() - 1; i >= 0; --i) {
			if (FreeImage_Validate((FREE_IMAGE_FORMAT)i, *io, handle)) {
				return (FREE_IMAGE_FORMAT)i;
			}
		}
	}

	return FIF_UNKNOWN;
}

// ALIASES FOR BACKWARDS COMPATIBILITY ----------------------

const char * DLL_CALLCONV
FreeImage_GetFileTypeFromFormat(FREE_IMAGE_FORMAT fif) {
	return FreeImage_GetFormatFromFIF(fif);
}

FREE_IMAGE_FORMAT DLL_CALLCONV
FreeImage_GetFileTypeFromExt(const char *filename) {
	return FreeImage_GetFIFFromFilename(filename);
}
