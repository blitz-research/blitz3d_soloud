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

#pragma warning(disable : 4541)

#include <assert.h>

#include <qcolor.h>
#include <qimage.h>
#include <qfile.h>
#include <qstring.h>
#include <windows.h>

//#include "FreeImageQt.h"

// ==========================================================

#define DLL_CALLCONV __stdcall

// For C compatility ----------------------------------------

#ifdef __cplusplus
#define FI_DEFAULT(x)	= x
#define FI_ENUM(x)		enum x
#define FI_STRUCT(x)	struct x
#else
#define FI_DEFAULT(x)
#define FI_ENUM(x)		typedef int x; enum x
#define FI_STRUCT(x)	typedef struct x x; struct x
#endif

// File IO routines -----------------------------------------

#ifndef FREEIMAGE_IO
#define FREEIMAGE_IO

#define fi_handle void*

typedef unsigned (*FI_ReadProc) (void *buffer, unsigned size, unsigned count, fi_handle handle);
typedef unsigned (*FI_WriteProc) (void *buffer, unsigned size, unsigned count, fi_handle handle);
typedef int (*FI_SeekProc) (fi_handle handle, long offset, int origin);
typedef long (*FI_TellProc) (fi_handle handle);

#ifdef WIN32
#pragma pack(push, 1)
#else
#pragma pack(1)
#endif

struct FreeImageIO {
	FI_ReadProc  read_proc;     //! pointer to the function used to read data
	FI_WriteProc write_proc;    //! pointer to the function used to write data
	FI_SeekProc  seek_proc;		//! pointer to the function used to seek
	FI_TellProc  tell_proc;		//! pointer to the function used to aquire the current position
};

#ifdef WIN32
#pragma pack(pop)
#else
#pragma pack(4)
#endif

#endif //!FREEIMAGE_IO

// ----------------------------------------------------------
//   WINDOWS STUFF FOR DLL HANDLING
// ----------------------------------------------------------

static HMODULE s_library_handle;
static bool s_library_loaded = false;

// ----------------------------------------------------------
//   DLL FUNCTION DECLARATIONS
// ----------------------------------------------------------

typedef void (DLL_CALLCONV *FreeImage_SetOutputMessageProc)(FreeImage_OutputMessageFunction omf);
typedef FIBITMAP *(DLL_CALLCONV *FreeImage_AllocateProc)(int width, int height, int bpp, int red_mask = 0, int green_mask = 0, int blue_mask = 0);
typedef void (DLL_CALLCONV *FreeImage_UnloadProc)(FIBITMAP *dib);
typedef int (DLL_CALLCONV *FreeImage_GetFIFCountProc)();
typedef FREE_IMAGE_FORMAT (DLL_CALLCONV *FreeImage_GetFIFFromFormatProc)(const char *format);
typedef const char * (DLL_CALLCONV *FreeImage_GetFormatFromFIFProc)(int id);
typedef const char * (DLL_CALLCONV *FreeImage_GetFIFDescriptionProc)(int id);
typedef const char * (DLL_CALLCONV *FreeImage_GetFIFRegExprProc)(int id);
typedef const char * (DLL_CALLCONV *FreeImage_GetFIFExtensionListProc)(int id);
typedef FREE_IMAGE_FORMAT (DLL_CALLCONV *FreeImage_GetFIFFromFilenameProc)(const char *filename);
typedef BOOL (DLL_CALLCONV *FreeImage_FIFSupportsReadingProc)(FREE_IMAGE_FORMAT id);
typedef BOOL (DLL_CALLCONV *FreeImage_FIFSupportsWritingProc)(FREE_IMAGE_FORMAT id);
typedef FIBITMAP * (DLL_CALLCONV *FreeImage_LoadFromHandleProc)(FREE_IMAGE_FORMAT fif, FreeImageIO *io, fi_handle handle, int flags = 0);
typedef BOOL (DLL_CALLCONV *FreeImage_SaveToHandleProc)(FREE_IMAGE_FORMAT fif, FIBITMAP *dib, FreeImageIO *io, fi_handle handle, int flags = 0);
typedef unsigned (DLL_CALLCONV *FreeImage_GetHeightProc) (FIBITMAP *dib);
typedef unsigned (DLL_CALLCONV *FreeImage_GetWidthProc) (FIBITMAP *dib);
typedef unsigned (DLL_CALLCONV *FreeImage_GetBPPProc) (FIBITMAP *dib);
typedef unsigned (DLL_CALLCONV *FreeImage_GetDotsPerMeterXProc) (FIBITMAP *dib);
typedef unsigned (DLL_CALLCONV *FreeImage_GetDotsPerMeterYProc) (FIBITMAP *dib);
typedef unsigned (DLL_CALLCONV *FreeImage_GetTransparencyCountProc) (FIBITMAP *dib);
typedef unsigned (DLL_CALLCONV *FreeImage_GetColorsUsedProc)(FIBITMAP *dib);
typedef unsigned (DLL_CALLCONV *FreeImage_GetPitchProc)(FIBITMAP *dib);
typedef unsigned (DLL_CALLCONV *FreeImage_GetRedMaskProc)(FIBITMAP *dib);
typedef unsigned (DLL_CALLCONV *FreeImage_GetGreenMaskProc)(FIBITMAP *dib);
typedef unsigned (DLL_CALLCONV *FreeImage_GetBlueMaskProc)(FIBITMAP *dib);
typedef const char *(DLL_CALLCONV *FreeImage_GetVersionProc)();
typedef const char *(DLL_CALLCONV *FreeImage_GetCopyrightMessageProc)();
typedef FREE_IMAGE_FORMAT (DLL_CALLCONV *FreeImage_GetFileTypeFromHandleProc)(FreeImageIO *io, fi_handle handle, int size);
typedef void (DLL_CALLCONV *FreeImage_SetTransparentProc) (FIBITMAP *dib, BOOL enable);
typedef BOOL (DLL_CALLCONV *FreeImage_IsTransparentProc) (FIBITMAP *dib);
typedef BYTE *(DLL_CALLCONV *FreeImage_GetScanLineProc) (FIBITMAP *dib, int scanline);
typedef BYTE *(DLL_CALLCONV *FreeImage_GetTransparencyTableProc) (FIBITMAP *dib);
typedef RGBQUAD *(DLL_CALLCONV *FreeImage_GetPaletteProc)(FIBITMAP *dib);
typedef BITMAPINFOHEADER *(DLL_CALLCONV *FreeImage_GetInfoHeaderProc) (FIBITMAP *dib);
typedef void (DLL_CALLCONV *FreeImage_ConvertLine16To32_555Proc)(BYTE *target, BYTE *source, int width_in_pixels);
typedef void (DLL_CALLCONV *FreeImage_ConvertLine16To32_565Proc)(BYTE *target, BYTE *source, int width_in_pixels);
typedef void (DLL_CALLCONV *FreeImage_ConvertLine4To8Proc) (BYTE *target, BYTE *source, int width_in_pixels);
typedef void (DLL_CALLCONV *FreeImage_ConvertLine24To32Proc) (BYTE *target, BYTE *source, int width_in_pixels);
typedef void (DLL_CALLCONV *FreeImage_ConvertLine32To24Proc) (BYTE *target, BYTE *source, int width_in_pixels);

// ----------------------------------------------------------
//   DLL FUNCTION ÍNSTANTIATONS
// ----------------------------------------------------------

FreeImage_SetOutputMessageProc FI_SetOutputMessageProc;
FreeImage_AllocateProc FI_AllocateProc;
FreeImage_UnloadProc FI_UnloadProc;
FreeImage_GetFIFCountProc FI_GetFIFCountProc;
FreeImage_GetFIFFromFormatProc FI_GetFIFFromFormatProc;
FreeImage_GetFormatFromFIFProc FI_GetFormatFromFIFProc;
FreeImage_GetFIFDescriptionProc FI_GetFIFDescriptionProc;
FreeImage_GetFIFRegExprProc FI_GetFIFRegExprProc;
FreeImage_GetFIFExtensionListProc FI_GetFIFExtensionListProc;
FreeImage_GetFIFFromFilenameProc FI_GetFIFFromFilenameProc;
FreeImage_FIFSupportsReadingProc FI_FIFSupportsReadingProc;
FreeImage_FIFSupportsWritingProc FI_FIFSupportsWritingProc;
FreeImage_LoadFromHandleProc FI_LoadFromHandleProc;
FreeImage_SaveToHandleProc FI_SaveToHandleProc;
FreeImage_GetHeightProc FI_GetHeightProc;
FreeImage_GetWidthProc FI_GetWidthProc;
FreeImage_GetBPPProc FI_GetBPPProc;
FreeImage_GetDotsPerMeterXProc FI_GetDotsPerMeterXProc;
FreeImage_GetDotsPerMeterYProc FI_GetDotsPerMeterYProc;
FreeImage_SetTransparentProc FI_SetTransparentProc;
FreeImage_IsTransparentProc FI_IsTransparentProc;
FreeImage_GetTransparencyCountProc FI_GetTransparencyCountProc;
FreeImage_GetColorsUsedProc FI_GetColorsUsedProc;
FreeImage_GetPitchProc FI_GetPitchProc;
FreeImage_GetRedMaskProc FI_GetRedMaskProc;
FreeImage_GetGreenMaskProc FI_GetGreenMaskProc;
FreeImage_GetBlueMaskProc FI_GetBlueMaskProc;
FreeImage_GetVersionProc FI_GetVersionProc;
FreeImage_GetCopyrightMessageProc FI_GetCopyrightMessageProc;
FreeImage_GetFileTypeFromHandleProc FI_GetFileTypeFromHandleProc;
FreeImage_GetScanLineProc FI_GetScanLineProc;
FreeImage_GetTransparencyTableProc FI_GetTransparencyTableProc;
FreeImage_GetPaletteProc FI_GetPaletteProc;
FreeImage_GetInfoHeaderProc FI_GetInfoHeaderProc;
FreeImage_ConvertLine4To8Proc FI_ConvertLine4To8Proc;
FreeImage_ConvertLine16To32_555Proc FI_ConvertLine16To32_555Proc;
FreeImage_ConvertLine16To32_565Proc FI_ConvertLine16To32_565Proc;
FreeImage_ConvertLine24To32Proc FI_ConvertLine24To32Proc;
FreeImage_ConvertLine32To24Proc FI_ConvertLine32To24Proc;

// ----------------------------------------------------------
//   INTERNAL QT BITMAP HANDLING FUNCTIONS
// ----------------------------------------------------------

BOOL
FormatSupportedByQt(QStrList &list, const char *format) {
	for (const char *qtformat = list.first(); qtformat != 0; qtformat = list.next()) {
		if (stricmp(format, qtformat) == 0)
			return TRUE;
	}

	return FALSE;
}

unsigned 
GetDotsPerMeterX(FIBITMAP *dib) {
	return (FI_GetDotsPerMeterXProc != NULL) ? FI_GetDotsPerMeterXProc(dib) : 0;
}

unsigned 
GetDotsPerMeterY(FIBITMAP *dib) {
	return (FI_GetDotsPerMeterYProc != NULL) ? FI_GetDotsPerMeterYProc(dib) : 0;
}

// ----------------------------------------------------------
//  FREEIMAGEIO FUNCTIONS SUITABLE FOR USE WITH QIODEVICE
// ----------------------------------------------------------

inline unsigned 
_ReadProc(void *buffer, unsigned size, unsigned count, fi_handle handle) {
	char *b = static_cast<char *>(buffer);

	for (unsigned c = 0; c < count; c++) {
		QIODevice *iod = static_cast<QIODevice *>(handle);

		iod->readBlock(b, size);

		b += size;
	}

	return count;
}

inline unsigned
_WriteProc(void *buffer, unsigned size, unsigned count, fi_handle handle) {
	char *b = static_cast<char *>(buffer);

	for (unsigned c = 0; c < count; c++) {
		QIODevice *iod = static_cast<QIODevice *>(handle);

		iod->writeBlock(b, size);

		b += size;
	}

	return count;
}

inline int
_SeekProc(fi_handle handle, long offset, int origin) {
	QIODevice *iod = static_cast<QIODevice *>(handle);

	switch(origin) {
		case SEEK_SET :
			iod->at(offset);
			break;

		case SEEK_CUR :
			iod->at(iod->at() + offset);
			break;

		default :
			return 1;
	}
  
	return 0;
}

inline long
_TellProc(fi_handle handle) {
	QIODevice *iod = static_cast<QIODevice *>(handle);

	return iod->at();
}

// ----------------------------------------------------------
//   PLUGIN INTERFACE
// ----------------------------------------------------------

int
FIQT_GetFIFCount() {
	return (FI_GetFIFCountProc != NULL) ? FI_GetFIFCountProc() : 0;
}

FREE_IMAGE_FORMAT
FIQT_GetFIFFromFormat(const char *format) {
	return (FI_GetFIFFromFormatProc != NULL) ? FI_GetFIFFromFormatProc(format) : FIF_UNKNOWN;
}

const char *
FIQT_GetFormatFromFIF(FREE_IMAGE_FORMAT id) {
	return (FI_GetFormatFromFIFProc != NULL) ? FI_GetFormatFromFIFProc(id) : NULL;
}

const char *
FIQT_GetFIFDescription(FREE_IMAGE_FORMAT id) {
	return (FI_GetFIFDescriptionProc != NULL) ? FI_GetFIFDescriptionProc(id) : NULL;
}

const char *
FIQT_GetFIFExtensionList(FREE_IMAGE_FORMAT id) {
	return (FI_GetFIFExtensionListProc != NULL) ? FI_GetFIFExtensionListProc(id) : NULL;
}

bool
FIQT_FIFSupportsReading(FREE_IMAGE_FORMAT id) {
	return (FI_FIFSupportsReadingProc != NULL) ? FI_FIFSupportsReadingProc(id) : false;
}

bool
FIQT_FIFSupportsWriting(FREE_IMAGE_FORMAT id) {
	return (FI_FIFSupportsWritingProc != NULL) ? FI_FIFSupportsWritingProc(id) : false;
}

FREE_IMAGE_FORMAT
FIQT_GetFIFFromFilename(const char *filename) {
	return (FI_GetFIFFromFilenameProc != NULL) ? FI_GetFIFFromFilenameProc(filename) : FIF_UNKNOWN;
}

const char *
FIQT_GetFileTypeFromFormat(FREE_IMAGE_FORMAT fif) {
	return FIQT_GetFormatFromFIF(fif);
}

const char *
FIQT_GetFileTypeFromExtension(const char *format) {
	FREE_IMAGE_FORMAT fif = FIQT_GetFIFFromFilename(format);

	return (fif != FIF_UNKNOWN) ? FIQT_GetFormatFromFIF(fif) : NULL;
}

FREE_IMAGE_FORMAT
FIQT_GetFileTypeFromHandle(FreeImageIO *io, fi_handle handle, int size) {
	return (FI_GetFileTypeFromHandleProc != NULL) ? FI_GetFileTypeFromHandleProc(io, handle, size) : FIF_UNKNOWN;
}

// ----------------------------------------------------------
//   LOAD/SAVE FUNCTIONS
// ----------------------------------------------------------

static void
FIQT_Load(QImageIO *iio) {
    FreeImageIO io;

    io.read_proc  = _ReadProc;
	io.seek_proc  = _SeekProc;
	io.tell_proc  = _TellProc;
	io.write_proc = _WriteProc;
	
	// because we don't always receive the format string here, we need to do some of our own investigation...
	// first try to identify the image by looking at the filename/extension

	FREE_IMAGE_FORMAT fif = FIQT_GetFIFFromFilename(iio->format());

	if (fif == FIF_UNKNOWN) {
		// format id based image lookup failed... try to identify the bitmap by looking at the header

		fif = FIQT_GetFileTypeFromHandle(&io, (fi_handle)iio->ioDevice(), 16);

		if (fif == FIF_UNKNOWN) {
			// failed again... look at the complete filename as a last resort

			if (!iio->fileName().isEmpty()) {
				fif = FIQT_GetFIFFromFilename(iio->fileName());

				if (fif == FIF_UNKNOWN) {
					iio->setStatus(-1);
					return;
				}
			} else {
				iio->setStatus(-1);
				return;
			}
		}
	}

	assert(fif != FIF_UNKNOWN);

	// if we come here, we have a valid FIF id

	if (FI_FIFSupportsReadingProc(fif)) {
		FIBITMAP *bitmap = FI_LoadFromHandleProc(fif, &io, (fi_handle)iio->ioDevice(), 0);
		
		if (bitmap != NULL) {
			QImage image;

			switch(FI_GetBPPProc(bitmap)) {
				case 1 :
					image.create(FI_GetWidthProc(bitmap), FI_GetHeightProc(bitmap), 1, FI_GetColorsUsedProc(bitmap), QImage::BigEndian);
					break;

				case 4 :
					image.create(FI_GetWidthProc(bitmap), FI_GetHeightProc(bitmap), 8, 256);
					break;

				case 16 :
				case 24 :
					image.create(FI_GetWidthProc(bitmap), FI_GetHeightProc(bitmap), 32, 0);
					break;

				default :
					image.create(FI_GetWidthProc(bitmap), FI_GetHeightProc(bitmap), FI_GetBPPProc(bitmap), FI_GetColorsUsedProc(bitmap));
					break;
			}
			
			// set metrics data

			image.setDotsPerMeterX(GetDotsPerMeterX(bitmap));
			image.setDotsPerMeterY(GetDotsPerMeterY(bitmap));

			// if there is a palette, copy it

			if (FI_IsTransparentProc(bitmap)) {
				image.setAlphaBuffer(TRUE);

				if (FI_GetBPPProc(bitmap) < 16) {
					RGBQUAD *palette = FI_GetPaletteProc(bitmap);	
					BYTE *trans_table = FI_GetTransparencyTableProc(bitmap);

					unsigned i;

					for (i = 0; i < FI_GetTransparencyCountProc(bitmap); ++i) {
						image.setColor(i, qRgba(palette[i].rgbRed, palette[i].rgbGreen, palette[i].rgbBlue, trans_table[i]));
					}

					for (; i < FI_GetColorsUsedProc(bitmap); ++i) {
						image.setColor(i, qRgba(palette[i].rgbRed, palette[i].rgbGreen, palette[i].rgbBlue, 0xff));				
					}
				}
			} else {
				image.setAlphaBuffer(FALSE);

				if (FI_GetBPPProc(bitmap) < 16) {
					RGBQUAD *palette = FI_GetPaletteProc(bitmap);

					for (unsigned i = 0; i < FI_GetColorsUsedProc(bitmap); ++i) {
						image.setColor(i, qRgb(palette[i].rgbRed, palette[i].rgbGreen, palette[i].rgbBlue));
					}
				}
			}

			// copy the bitmap data and convert if neccesary

			for (unsigned y = 0; y < FI_GetHeightProc(bitmap); y++) {
				if (FI_GetBPPProc(bitmap) == 4) {
					FI_ConvertLine4To8Proc(image.scanLine(FI_GetHeightProc(bitmap) - 1 - y), FI_GetScanLineProc(bitmap, y), FI_GetWidthProc(bitmap));
				} else if (FI_GetBPPProc(bitmap) == 16) {
					if ((FI_GetRedMaskProc(bitmap) == 0x1F) && (FI_GetGreenMaskProc(bitmap) == 0x3E0) && (FI_GetBlueMaskProc(bitmap) == 0x7C00)) {
						FI_ConvertLine16To32_555Proc(image.scanLine(FI_GetHeightProc(bitmap) - 1 - y), FI_GetScanLineProc(bitmap, y), FI_GetWidthProc(bitmap));
					} else {
						FI_ConvertLine16To32_565Proc(image.scanLine(FI_GetHeightProc(bitmap) - 1 - y), FI_GetScanLineProc(bitmap, y), FI_GetWidthProc(bitmap));
					}
				} else if (FI_GetBPPProc(bitmap) == 24) {
					FI_ConvertLine24To32Proc(image.scanLine(FI_GetHeightProc(bitmap) - 1 - y), FI_GetScanLineProc(bitmap, y), FI_GetWidthProc(bitmap));
				} else {
					memcpy(image.scanLine(FI_GetHeightProc(bitmap) - y - 1), FI_GetScanLineProc(bitmap, y), FI_GetPitchProc(bitmap));
				}
			}		

			iio->setImage(image);
			iio->setStatus(0);
			
			FI_UnloadProc(bitmap);

			return;
		}
	}

	iio->setStatus(-1);
}

static void
FIQT_Save(QImageIO *iio) {
    FreeImageIO io;

    io.read_proc  = _ReadProc;
	io.seek_proc  = _SeekProc;
	io.tell_proc  = _TellProc;
	io.write_proc = _WriteProc;
	
	// get the save pointer for the needed bitmap format

	FREE_IMAGE_FORMAT fif = FIQT_GetFIFFromFilename(iio->format());

	if (fif != FIF_UNKNOWN) {
		if (FIQT_FIFSupportsWriting(fif)) {
			FIBITMAP *bitmap;
			
			switch(fif) {
				case FIF_BMP :
					if ((!iio->image().hasAlphaBuffer()) && (iio->image().depth() == 32)) {
						bitmap = FI_AllocateProc(iio->image().width(), iio->image().height(), 24);
					} else {
						bitmap = FI_AllocateProc(iio->image().width(), iio->image().height(), iio->image().depth());
					}

					break;

				case FIF_JPEG :
					if (iio->image().depth() == 32) {
						bitmap = FI_AllocateProc(iio->image().width(), iio->image().height(), 24);
					} else {
						bitmap = FI_AllocateProc(iio->image().width(), iio->image().height(), iio->image().depth());
					}
	
					break;
					
				default :
					bitmap = FI_AllocateProc(iio->image().width(), iio->image().height(), iio->image().depth());
					break;
			};

			// enable alpha channels in the DIB when we must

			if (iio->image().hasAlphaBuffer()) {
				FI_SetTransparentProc(bitmap, TRUE);
			} else {
				FI_SetTransparentProc(bitmap, FALSE);
			}

			// copy the palette

			RGBQUAD *palette = FI_GetPaletteProc(bitmap);
			QRgb *colorTable = iio->image().colorTable();

			for (int i = 0; i < iio->image().numColors(); ++i) {
				palette[i].rgbBlue = qBlue(colorTable[i]);
				palette[i].rgbGreen = qGreen(colorTable[i]);
				palette[i].rgbRed = qRed(colorTable[i]);
			}

			// copy the metrics data

			BITMAPINFOHEADER *header = FI_GetInfoHeaderProc(bitmap);
			header->biXPelsPerMeter = iio->image().dotsPerMeterX();
			header->biYPelsPerMeter = iio->image().dotsPerMeterY();

			// copy the bits

			for (unsigned y = 0; y < FI_GetHeightProc(bitmap); y++) {
				switch(fif) {
					case FIF_BMP :
						if ((!iio->image().hasAlphaBuffer()) && (iio->image().depth() == 32)) {
							FI_ConvertLine32To24Proc(FI_GetScanLineProc(bitmap, y), iio->image().scanLine(FI_GetHeightProc(bitmap) - 1 - y), iio->image().width());
						} else {
							memcpy(FI_GetScanLineProc(bitmap, y), iio->image().scanLine(FI_GetHeightProc(bitmap) - 1 - y), FI_GetPitchProc(bitmap));
						}

						break;

					case FIF_JPEG :
						if (iio->image().depth() == 32) {
							FI_ConvertLine32To24Proc(FI_GetScanLineProc(bitmap, y), iio->image().scanLine(FI_GetHeightProc(bitmap) - 1 - y), iio->image().width());
						} else {
							memcpy(FI_GetScanLineProc(bitmap, y), iio->image().scanLine(FI_GetHeightProc(bitmap) - 1 - y), FI_GetPitchProc(bitmap));
						}

						break;
					
					default :
						memcpy(FI_GetScanLineProc(bitmap, y), iio->image().scanLine(FI_GetHeightProc(bitmap) - 1 - y), FI_GetPitchProc(bitmap));
						break;
				};
			}

			// save the bitmap

			if (FI_SaveToHandleProc(fif, bitmap, &io, (fi_handle)iio->ioDevice())) {
				iio->setStatus(0); // status 0 == SUCCESS
			} else {
				iio->setStatus(-1); // status -1 == FAILURE
			}
					
			FI_UnloadProc(bitmap);
					
			return;
		}
	}

	iio->setStatus(-1);
}

// ----------------------------------------------------------
//   OUTPUT MESSAGE FUNCTIONS
// ----------------------------------------------------------

void 
FIQT_SetOutputMessage(FreeImage_OutputMessageFunction omf) {
	if (FI_SetOutputMessageProc != NULL) {
		FI_SetOutputMessageProc(omf);
	}
}

// ----------------------------------------------------------
//   VERSION HANDLING / COPYRIGHT
// ----------------------------------------------------------

const char * 
FIQT_GetVersion() {
	return (FI_GetVersionProc != NULL) ? FI_GetVersionProc() : NULL;
}

const char * 
FIQT_GetCopyrightMessage() {
	return (FI_GetCopyrightMessageProc != NULL) ? FI_GetCopyrightMessageProc() : NULL;
}

// ----------------------------------------------------------
//   REGISTRATION OF FREEIMAGE IN THE QT MECHANISM
// ----------------------------------------------------------

bool
FIQT_Register(bool new_formats_only) {
	if (!s_library_loaded) {
#ifdef _DEBUG
		if ((s_library_handle = LoadLibrary("freeimaged.dll")) != NULL) {
#else
		if ((s_library_handle = LoadLibrary("freeimage.dll")) != NULL) {
#endif
			// extract the functions from the FreeImage library

			FI_SetOutputMessageProc = reinterpret_cast<FreeImage_SetOutputMessageProc>(GetProcAddress(s_library_handle, "_FreeImage_SetOutputMessage@4"));
			FI_AllocateProc = reinterpret_cast<FreeImage_AllocateProc>(GetProcAddress(s_library_handle, "_FreeImage_Allocate@24"));
			FI_UnloadProc = reinterpret_cast<FreeImage_UnloadProc>(GetProcAddress(s_library_handle, "_FreeImage_Unload@4"));
			FI_GetFIFCountProc = reinterpret_cast<FreeImage_GetFIFCountProc>(GetProcAddress(s_library_handle, "_FreeImage_GetFIFCount@0"));
			FI_GetFIFFromFormatProc = reinterpret_cast<FreeImage_GetFIFFromFormatProc>(GetProcAddress(s_library_handle, "_FreeImage_GetFIFFromFormat@4"));
			FI_GetFormatFromFIFProc = reinterpret_cast<FreeImage_GetFormatFromFIFProc>(GetProcAddress(s_library_handle, "_FreeImage_GetFormatFromFIF@4"));
			FI_GetFIFDescriptionProc = reinterpret_cast<FreeImage_GetFIFDescriptionProc>(GetProcAddress(s_library_handle, "_FreeImage_GetFIFDescription@4"));
			FI_GetFIFRegExprProc = reinterpret_cast<FreeImage_GetFIFRegExprProc>(GetProcAddress(s_library_handle, "_FreeImage_GetFIFRegExpr@4"));
			FI_GetFIFExtensionListProc = reinterpret_cast<FreeImage_GetFIFExtensionListProc>(GetProcAddress(s_library_handle, "_FreeImage_GetFIFExtensionList@4"));
			FI_GetFIFFromFilenameProc = reinterpret_cast<FreeImage_GetFIFFromFilenameProc>(GetProcAddress(s_library_handle, "_FreeImage_GetFIFFromFilename@4"));
			FI_FIFSupportsReadingProc = reinterpret_cast<FreeImage_FIFSupportsReadingProc>(GetProcAddress(s_library_handle, "_FreeImage_FIFSupportsReading@4"));
			FI_FIFSupportsWritingProc = reinterpret_cast<FreeImage_FIFSupportsWritingProc>(GetProcAddress(s_library_handle, "_FreeImage_FIFSupportsWriting@4"));
			FI_LoadFromHandleProc = reinterpret_cast<FreeImage_LoadFromHandleProc>(GetProcAddress(s_library_handle, "_FreeImage_LoadFromHandle@16"));
			FI_SaveToHandleProc = reinterpret_cast<FreeImage_SaveToHandleProc>(GetProcAddress(s_library_handle, "_FreeImage_SaveToHandle@20"));
			FI_GetHeightProc = reinterpret_cast<FreeImage_GetHeightProc>(GetProcAddress(s_library_handle, "_FreeImage_GetHeight@4"));
			FI_GetWidthProc = reinterpret_cast<FreeImage_GetWidthProc>(GetProcAddress(s_library_handle, "_FreeImage_GetWidth@4"));
			FI_GetBPPProc = reinterpret_cast<FreeImage_GetBPPProc>(GetProcAddress(s_library_handle, "_FreeImage_GetBPP@4"));
			FI_GetDotsPerMeterXProc = reinterpret_cast<FreeImage_GetDotsPerMeterXProc>(GetProcAddress(s_library_handle, "_FreeImage_GetDotsPerMeterX@4"));
			FI_GetDotsPerMeterYProc = reinterpret_cast<FreeImage_GetDotsPerMeterYProc>(GetProcAddress(s_library_handle, "_FreeImage_GetDotsPerMeterY@4"));
			FI_SetTransparentProc = reinterpret_cast<FreeImage_SetTransparentProc>(GetProcAddress(s_library_handle, "_FreeImage_SetTransparent@8"));
			FI_IsTransparentProc = reinterpret_cast<FreeImage_IsTransparentProc>(GetProcAddress(s_library_handle, "_FreeImage_IsTransparent@4"));
			FI_GetTransparencyCountProc = reinterpret_cast<FreeImage_GetTransparencyCountProc>(GetProcAddress(s_library_handle, "_FreeImage_GetTransparencyCount@4"));
			FI_GetTransparencyTableProc = reinterpret_cast<FreeImage_GetTransparencyTableProc>(GetProcAddress(s_library_handle, "_FreeImage_GetTransparencyTable@4"));
			FI_GetColorsUsedProc = reinterpret_cast<FreeImage_GetColorsUsedProc>(GetProcAddress(s_library_handle, "_FreeImage_GetColorsUsed@4"));
			FI_GetPaletteProc = reinterpret_cast<FreeImage_GetPaletteProc>(GetProcAddress(s_library_handle, "_FreeImage_GetPalette@4"));
			FI_GetPitchProc = reinterpret_cast<FreeImage_GetPitchProc>(GetProcAddress(s_library_handle, "_FreeImage_GetPitch@4"));
			FI_GetVersionProc = reinterpret_cast<FreeImage_GetVersionProc>(GetProcAddress(s_library_handle, "_FreeImage_GetVersion@0"));
			FI_GetRedMaskProc = reinterpret_cast<FreeImage_GetRedMaskProc>(GetProcAddress(s_library_handle, "_FreeImage_GetRedMask@4"));
			FI_GetGreenMaskProc = reinterpret_cast<FreeImage_GetGreenMaskProc>(GetProcAddress(s_library_handle, "_FreeImage_GetGreenMask@4"));
			FI_GetBlueMaskProc = reinterpret_cast<FreeImage_GetBlueMaskProc>(GetProcAddress(s_library_handle, "_FreeImage_GetBlueMask@4"));
			FI_GetCopyrightMessageProc = reinterpret_cast<FreeImage_GetCopyrightMessageProc>(GetProcAddress(s_library_handle, "_FreeImage_GetCopyrightMessage@4"));
			FI_GetFileTypeFromHandleProc = reinterpret_cast<FreeImage_GetFileTypeFromHandleProc>(GetProcAddress(s_library_handle, "_FreeImage_GetFileTypeFromHandle@12"));
			FI_GetScanLineProc = reinterpret_cast<FreeImage_GetScanLineProc>(GetProcAddress(s_library_handle, "_FreeImage_GetScanLine@8"));
			FI_GetInfoHeaderProc = reinterpret_cast<FreeImage_GetInfoHeaderProc>(GetProcAddress(s_library_handle, "_FreeImage_GetInfoHeader@4"));
			FI_ConvertLine4To8Proc = reinterpret_cast<FreeImage_ConvertLine4To8Proc>(GetProcAddress(s_library_handle, "_FreeImage_ConvertLine4To8@12"));
			FI_ConvertLine16To32_555Proc = reinterpret_cast<FreeImage_ConvertLine16To32_555Proc>(GetProcAddress(s_library_handle, "_FreeImage_ConvertLine16To32_555@12"));
			FI_ConvertLine16To32_565Proc = reinterpret_cast<FreeImage_ConvertLine16To32_565Proc>(GetProcAddress(s_library_handle, "_FreeImage_ConvertLine16To32_565@12"));
			FI_ConvertLine24To32Proc = reinterpret_cast<FreeImage_ConvertLine24To32Proc>(GetProcAddress(s_library_handle, "_FreeImage_ConvertLine24To32@12"));
			FI_ConvertLine32To24Proc = reinterpret_cast<FreeImage_ConvertLine32To24Proc>(GetProcAddress(s_library_handle, "_FreeImage_ConvertLine32To24@12"));
			
			if ((FI_GetVersionProc != 0) &&
				(FI_AllocateProc != 0) &&
				(FI_UnloadProc != 0) &&
				(FI_LoadFromHandleProc != 0) &&
				(FI_SaveToHandleProc != 0) &&
				(FI_GetHeightProc != 0) &&
				(FI_GetWidthProc != 0) &&
				(FI_GetBPPProc != 0) &&
				(FI_GetScanLineProc != 0) &&
				(FI_GetTransparencyCountProc != 0) &&
				(FI_GetTransparencyTableProc != 0) &&
				(FI_IsTransparentProc != 0) &&
				(FI_GetColorsUsedProc != 0) &&
				(FI_GetPaletteProc != 0) &&
				(FI_GetPitchProc != 0) &&
				(FI_GetRedMaskProc != 0) &&
				(FI_GetGreenMaskProc != 0) &&
				(FI_GetBlueMaskProc != 0) &&
				(FI_GetInfoHeaderProc != 0) &&
				(FI_ConvertLine4To8Proc != 0) &&
				(FI_ConvertLine16To32_555Proc != 0) &&
				(FI_ConvertLine16To32_565Proc != 0) &&
				(FI_ConvertLine24To32Proc != 0),
				(FI_ConvertLine32To24Proc != 0),
				(FI_GetFIFRegExprProc != 0)) {
				
				QStrList format_list;
				format_list = QImageIO::inputFormats();

				for (int i = 0; i < FIQT_GetFIFCount(); ++i) {
					if (!FormatSupportedByQt(format_list, FIQT_GetFormatFromFIF((FREE_IMAGE_FORMAT)i)) || !new_formats_only) {
						QImageIO::defineIOHandler(FIQT_GetFormatFromFIF((FREE_IMAGE_FORMAT)i), FI_GetFIFRegExprProc(i), 0, FIQT_FIFSupportsReading((FREE_IMAGE_FORMAT)i) ? FIQT_Load : NULL, FIQT_FIFSupportsWriting((FREE_IMAGE_FORMAT)i) ? FIQT_Save : NULL);
					}
				}

				s_library_loaded = true;

				atexit(FIQT_Unregister); // when the application stops, automatically unregister

				return s_library_loaded;
			}
		}

		s_library_loaded = false;

		return s_library_loaded;
	}

	return s_library_loaded;
}

bool 
FIQT_IsLoaded() {
	return s_library_loaded;
}

void 
FIQT_Unregister() {
	if (s_library_loaded) {
		FreeLibrary(s_library_handle);

		s_library_loaded = FALSE;
	}		
}
