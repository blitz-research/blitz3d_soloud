// ==========================================================
// FreeImage Qt
//
// Design and implementation by
// - Floris van den Berg (flvdberg@wxs.nl)
//
// Based on tiffIO code written by Markus L. Noga
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

#ifndef FREEIMAGEQT_H
#define FREEIMAGEQT_H

// ----------------------------------------------------------

class QString;

// Bitmap types -------------------------------------------------------------

struct FIBITMAP { void *data; };

// FreeImage type FI_ENUM (------------------------------------------------------

enum FREE_IMAGE_FORMAT {
	FIF_UNKNOWN = -1,
	FIF_BMP = 0,
	FIF_ICO,
	FIF_JPEG,
	FIF_JNG,
	FIF_KOALA,
	FIF_LBM,
	FIF_MNG,
	FIF_PBM,
	FIF_PBMRAW,
	FIF_PCD,
	FIF_PCX,
	FIF_PGM,
	FIF_PGMRAW,
	FIF_PNG,
	FIF_PPM,
	FIF_PPMRAW,
	FIF_RAS,
	FIF_TARGA,
	FIF_TIFF,
	FIF_WBMP,
};

// ----------------------------------------------------------

typedef void (*FreeImage_OutputMessageFunction)(FREE_IMAGE_FORMAT fif, const char *msg);
void FIQT_SetOutputMessage(FreeImage_OutputMessageFunction omf);

// ----------------------------------------------------------

bool FIQT_Register(bool new_formats_only = false);
bool FIQT_IsLoaded();
void FIQT_Unregister();
const char *FIQT_GetVersion();
const char *FIQT_GetCopyrightMessage();

// ----------------------------------------------------------

int FIQT_GetFIFCount();
FREE_IMAGE_FORMAT FIQT_GetFIFFromFormat(const char *format);
const char *FIQT_GetFormatFromFIF(FREE_IMAGE_FORMAT fif);
const char *FIQT_GetFIFDescription(FREE_IMAGE_FORMAT fif);
const char *FIQT_GetFIFExtensionList(FREE_IMAGE_FORMAT fif);
bool FIQT_FIFSupportsReading(FREE_IMAGE_FORMAT fif);
bool FIQT_FIFSupportsWriting(FREE_IMAGE_FORMAT fif);
FREE_IMAGE_FORMAT FIQT_GetFIFFromFilename(const char *filename);
const char *FIQT_GetFileTypeFromExtension(const char *format);
const char *FIQT_GetFileTypeFromFormat(FREE_IMAGE_FORMAT fif); // this function is deprecated


#endif //!FREEIMAGEQT_H
