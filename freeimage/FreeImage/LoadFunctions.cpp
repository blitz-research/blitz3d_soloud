// ==========================================================
// Simplified load functions
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

// ----------------------------------------------------------

FIBITMAP * DLL_CALLCONV
FreeImage_LoadBMP(const char *filename, int flags) {
	return FreeImage_Load(FreeImage_GetFIFFromFormat("BMP"), filename, flags);
}

FIBITMAP * DLL_CALLCONV
FreeImage_LoadBMPFromHandle(FreeImageIO *io, fi_handle handle, int flags) {
	return FreeImage_LoadFromHandle(FreeImage_GetFIFFromFormat("BMP"), io, handle, flags);
}

BOOL DLL_CALLCONV
FreeImage_SaveBMP(FIBITMAP *dib, const char *filename, int flags) {
	return FreeImage_Save(FreeImage_GetFIFFromFormat("BMP"), dib, filename, flags);
}

BOOL DLL_CALLCONV
FreeImage_SaveBMPToHandle(FIBITMAP *dib, FreeImageIO *io, fi_handle handle, int flags) {
	return FreeImage_SaveToHandle(FreeImage_GetFIFFromFormat("BMP"), dib, io, handle, flags);
}

// ----------------------------------------------------------

FIBITMAP * DLL_CALLCONV
FreeImage_LoadICO(const char *filename, int flags) {
	return FreeImage_Load(FreeImage_GetFIFFromFormat("ICO"), filename, flags);
}

FIBITMAP * DLL_CALLCONV
FreeImage_LoadICOFromHandle(FreeImageIO *io, fi_handle handle, int flags) {
	return FreeImage_LoadFromHandle(FreeImage_GetFIFFromFormat("ICO"), io, handle, flags);
}

// ----------------------------------------------------------

FIBITMAP * DLL_CALLCONV
FreeImage_LoadIFF(const char *filename, int flags) {
	return FreeImage_Load(FreeImage_GetFIFFromFormat("IFF"), filename, flags);
}

FIBITMAP * DLL_CALLCONV
FreeImage_LoadIFFFromHandle(FreeImageIO *io, fi_handle handle, int flags) {
	return FreeImage_LoadFromHandle(FreeImage_GetFIFFromFormat("IFF"), io, handle, flags);
}

// ----------------------------------------------------------

FIBITMAP * DLL_CALLCONV
FreeImage_LoadJPEG(const char *filename, int flags) {
	return FreeImage_Load(FreeImage_GetFIFFromFormat("JPEG"), filename, flags);
}

FIBITMAP * DLL_CALLCONV
FreeImage_LoadJPEGFromHandle(FreeImageIO *io, fi_handle handle, int flags) {
	return FreeImage_LoadFromHandle(FreeImage_GetFIFFromFormat("JPEG"), io, handle, flags);
}

BOOL DLL_CALLCONV
FreeImage_SaveJPEG(FIBITMAP *dib, const char *filename, int flags) {
	return FreeImage_Save(FreeImage_GetFIFFromFormat("JPEG"), dib, filename, flags);
}

BOOL DLL_CALLCONV
FreeImage_SaveJPEGToHandle(FIBITMAP *dib, FreeImageIO *io, fi_handle handle, int flags) {
	return FreeImage_SaveToHandle(FreeImage_GetFIFFromFormat("JPEG"), dib, io, handle, flags);
}

// ----------------------------------------------------------

FIBITMAP * DLL_CALLCONV
FreeImage_LoadKOALA(const char *filename, int flags) {
	return FreeImage_Load(FreeImage_GetFIFFromFormat("KOALA"), filename, flags);
}

FIBITMAP * DLL_CALLCONV
FreeImage_LoadKOALAFromHandle(FreeImageIO *io, fi_handle handle, int flags) {
	return FreeImage_LoadFromHandle(FreeImage_GetFIFFromFormat("KOALA"), io, handle, flags);
}

// ----------------------------------------------------------

FIBITMAP * DLL_CALLCONV
FreeImage_LoadLBM(const char *filename, int flags) {
	return FreeImage_Load(FreeImage_GetFIFFromFormat("IFF"), filename, flags);
}

FIBITMAP * DLL_CALLCONV
FreeImage_LoadLBMFromHandle(FreeImageIO *io, fi_handle handle, int flags) {
	return FreeImage_LoadFromHandle(FreeImage_GetFIFFromFormat("IFF"), io, handle, flags);
}

// ----------------------------------------------------------

FIBITMAP * DLL_CALLCONV
FreeImage_LoadMNG(const char *filename, int flags) {
	return FreeImage_Load(FreeImage_GetFIFFromFormat("MNG"), filename, flags);
}

FIBITMAP * DLL_CALLCONV
FreeImage_LoadMNGFromHandle(FreeImageIO *io, fi_handle handle, int flags) {
	return FreeImage_LoadFromHandle(FreeImage_GetFIFFromFormat("MNG"), io, handle, flags);
}

// ----------------------------------------------------------

FIBITMAP * DLL_CALLCONV
FreeImage_LoadPCD(const char *filename, int flags) {
	return FreeImage_Load(FreeImage_GetFIFFromFormat("PCD"), filename, flags);
}

FIBITMAP * DLL_CALLCONV
FreeImage_LoadPCDFromHandle(FreeImageIO *io, fi_handle handle, int flags) {
	return FreeImage_LoadFromHandle(FreeImage_GetFIFFromFormat("PCD"), io, handle, flags);
}

// ----------------------------------------------------------

FIBITMAP * DLL_CALLCONV
FreeImage_LoadPCX(const char *filename, int flags) {
	return FreeImage_Load(FreeImage_GetFIFFromFormat("PCX"), filename, flags);
}

FIBITMAP * DLL_CALLCONV
FreeImage_LoadPCXFromHandle(FreeImageIO *io, fi_handle handle, int flags) {
	return FreeImage_LoadFromHandle(FreeImage_GetFIFFromFormat("PCX"), io, handle, flags);
}

// ----------------------------------------------------------

FIBITMAP * DLL_CALLCONV
FreeImage_LoadPNG(const char *filename, int flags) {
	return FreeImage_Load(FreeImage_GetFIFFromFormat("PNG"), filename, flags);
}

FIBITMAP * DLL_CALLCONV
FreeImage_LoadPNGFromHandle(FreeImageIO *io, fi_handle handle, int flags) {
	return FreeImage_LoadFromHandle(FreeImage_GetFIFFromFormat("PNG"), io, handle, flags);
}

BOOL DLL_CALLCONV
FreeImage_SavePNG(FIBITMAP *dib, const char *filename, int flags) {
	return FreeImage_Save(FreeImage_GetFIFFromFormat("PNG"), dib, filename, flags);
}

BOOL DLL_CALLCONV
FreeImage_SavePNGToHandle(FIBITMAP *dib, FreeImageIO *io, fi_handle handle, int flags) {
	return FreeImage_SaveToHandle(FreeImage_GetFIFFromFormat("PNG"), dib, io, handle, flags);
}

// ----------------------------------------------------------

FIBITMAP * DLL_CALLCONV
FreeImage_LoadPNM(const char *filename, int flags) {
	return FreeImage_Load(FreeImage_GetFIFFromFormat("PGM"), filename, flags);
}

FIBITMAP * DLL_CALLCONV
FreeImage_LoadPNMFromHandle(FreeImageIO *io, fi_handle handle, int flags) {
	return FreeImage_LoadFromHandle(FreeImage_GetFIFFromFormat("PGM"), io, handle, flags);
}

BOOL DLL_CALLCONV
FreeImage_SavePNM(FIBITMAP *dib, const char *filename, int flags) {
	return FreeImage_Save(FreeImage_GetFIFFromFormat("PGM"), dib, filename, flags);
}

BOOL DLL_CALLCONV
FreeImage_SavePNMToHandle(FIBITMAP *dib, FreeImageIO *io, fi_handle handle, int flags) {
	return FreeImage_SaveToHandle(FreeImage_GetFIFFromFormat("PGM"), dib, io, handle, flags);
}

// ----------------------------------------------------------

FIBITMAP * DLL_CALLCONV
FreeImage_LoadPSD(const char *filename, int flags) {
	return FreeImage_Load(FreeImage_GetFIFFromFormat("PSD"), filename, flags);
}

FIBITMAP * DLL_CALLCONV
FreeImage_LoadPSDFromHandle(FreeImageIO *io, fi_handle handle, int flags) {
	return FreeImage_LoadFromHandle(FreeImage_GetFIFFromFormat("PSD"), io, handle, flags);
}

// ----------------------------------------------------------

FIBITMAP * DLL_CALLCONV
FreeImage_LoadRAS(const char *filename, int flags) {
	return FreeImage_Load(FreeImage_GetFIFFromFormat("RAS"), filename, flags);
}

FIBITMAP * DLL_CALLCONV
FreeImage_LoadRASFromHandle(FreeImageIO *io, fi_handle handle, int flags) {
	return FreeImage_LoadFromHandle(FreeImage_GetFIFFromFormat("RAS"), io, handle, flags);
}

// ----------------------------------------------------------

FIBITMAP * DLL_CALLCONV
FreeImage_LoadTARGA(const char *filename, int flags) {
	return FreeImage_Load(FreeImage_GetFIFFromFormat("TARGA"), filename, flags);
}

FIBITMAP * DLL_CALLCONV
FreeImage_LoadTARGAFromHandle(FreeImageIO *io, fi_handle handle, int flags) {
	return FreeImage_LoadFromHandle(FreeImage_GetFIFFromFormat("TARGA"), io, handle, flags);
}

// ----------------------------------------------------------

FIBITMAP * DLL_CALLCONV
FreeImage_LoadTIFF(const char *filename, int flags) {
	return FreeImage_Load(FreeImage_GetFIFFromFormat("TIFF"), filename, flags);
}

FIBITMAP * DLL_CALLCONV
FreeImage_LoadTIFFFromHandle(FreeImageIO *io, fi_handle handle, int flags) {
	return FreeImage_LoadFromHandle(FreeImage_GetFIFFromFormat("TIFF"), io, handle, flags);
}

BOOL DLL_CALLCONV
FreeImage_SaveTIFF(FIBITMAP *dib, const char *filename, int flags) {
	return FreeImage_Save(FreeImage_GetFIFFromFormat("TIFF"), dib, filename, flags);
}

BOOL DLL_CALLCONV
FreeImage_SaveTIFFToHandle(FIBITMAP *dib, FreeImageIO *io, fi_handle handle, int flags) {
	return FreeImage_SaveToHandle(FreeImage_GetFIFFromFormat("TIFF"), dib, io, handle, flags);
}

// ----------------------------------------------------------

FIBITMAP * DLL_CALLCONV
FreeImage_LoadWBMP(const char *filename, int flags) {
	return FreeImage_Load(FreeImage_GetFIFFromFormat("WBMP"), filename, flags);
}

FIBITMAP * DLL_CALLCONV
FreeImage_LoadWBMPFromHandle(FreeImageIO *io, fi_handle handle, int flags) {
	return FreeImage_LoadFromHandle(FreeImage_GetFIFFromFormat("WBMP"), io, handle, flags);
}

BOOL DLL_CALLCONV
FreeImage_SaveWBMP(FIBITMAP *dib, const char *filename, int flags) {
	return FreeImage_Save(FreeImage_GetFIFFromFormat("WBMP"), dib, filename, flags);
}

BOOL DLL_CALLCONV
FreeImage_SaveWBMPToHandle(FIBITMAP *dib, FreeImageIO *io, fi_handle handle, int flags) {
	return FreeImage_SaveToHandle(FreeImage_GetFIFFromFormat("WBMP"), dib, io, handle, flags);
}
