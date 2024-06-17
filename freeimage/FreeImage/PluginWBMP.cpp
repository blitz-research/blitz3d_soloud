// ==========================================================
// Wireless Bitmap Format Loader and Writer
//
// Design and implementation by
// - Hervé Drolon <drolon@infonie.fr>
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
// Wireless Bitmap Format
// ----------------------
// The WBMP format enables graphical information to be sent to a variety of handsets.
// The WBMP format is terminal independent and describes only graphical information.

// IMPLEMENTATION NOTES:
// ------------------------
// The WBMP format is configured according to a type field value (TypeField below),
// which maps to all relevant image encoding information, such as:
// · Pixel organisation and encoding
// · Palette organisation and encoding
// · Compression characteristics
// · Animation encoding
// For each TypeField value, all relevant image characteristics are 
// fully specified as part of the WAP documentation.
// Currently, a simple compact, monochrome image format is defined
// within the WBMP type space :
//
// Image Type Identifier, multi-byte integer	0
// Image Format description						0 B/W, no compression
// -------------------------------------------------------------------------------

// WBMP Header

#ifdef WIN32
#pragma pack(push, 1)
#else
#pragma pack(1)
#endif

typedef struct {
	WORD TypeField;			// Image type identifier of multi-byte length
	BYTE FixHeaderField;	// Octet of general header information
	BYTE ExtHeaderFields;	// Zero or more extension header fields
	WORD Width;				// Multi-byte width field
	WORD Height;			// Multi-byte height field
} WBMPHEADER;

#ifdef WIN32
#pragma pack(pop)
#else
#pragma pack(4)
#endif

// The extension headers may be of type binary 00 through binary 11, defined as follows.

// - Type 00 indicates a multi-byte bitfield used to specify additional header information.
// The first bit is set if a type 00, extension header is set if more data follows.
//  The other bits are reserved for future use.
// - Type 01 - reserved for future use.
// - Type 10 - reserved for future use.
// - Type 11 indicates a sequence of parameter/value pairs. These can be used for 
// optimisations and special purpose extensions, eg, animation image formats.
// The parameter size tells the length (1-8 bytes) of the following parameter name.
// The value size gives the length (1-16 bytes) of the following parameter value.
// The concatenation flag indicates whether another parameter/value pair will follow
// after reading the specified bytes of data.

// ==========================================================
// Internal functions
// ==========================================================

static DWORD
multiByteRead(FreeImageIO &io, fi_handle handle) {
	// Multi-byte encoding / decoding
	// -------------------------------
	// A multi-byte integer consists of a series of octets, where the most significant bit
	// is the continuation flag, and the remaining seven bits are a scalar value.
	// The continuation flag is used to indicate that an octet is not the end of the multi-byte
	// sequence.

	DWORD Out = 0;
	BYTE In = 0;

	while (io.read_proc(&In, 1, 1, handle)) {
		Out += (In & 0x7F);

		if ((In & 0x80) == 0x00) {
			// Last byte to read
		  
			break;
		}

		Out <<= 7;
	}

	return Out;
}

static void
multiByteWrite(FreeImageIO &io, fi_handle handle, DWORD In) {
  BYTE Out, k = 1;
  
  while(In & (0x7F << 7*k))
	  k++;
  
  while(k > 1) {
	  k--;
	  Out = (BYTE)(0x80 | (In >> 7*k) & 0xFF);
	  io.write_proc(&Out, 1, 1, handle);
  }

  // Last byte to write

  Out = (BYTE)(In & 0x7F);

  io.write_proc(&Out, 1, 1, handle);
}

static void
readExtHeader(FreeImageIO &io, fi_handle handle, BYTE b) {
    // Extension header fields
    // ------------------------
    // Read the extension header fields
    // (since we don't use them for the moment, we skip them).

	switch(b & 0x60) {
		// Type 00: read multi-byte bitfield

		case 0x00:
		{
			DWORD info = multiByteRead(io, handle);
		}
		break;

		// Type 11: read a sequence of parameter/value pairs.

		case 0x60:
		{
			BYTE sizeParamIdent = (b & 0x70) >> 4;	// Size of Parameter Identifier (in bytes)
			BYTE sizeParamValue = (b & 0x0F);		// Size of Parameter Value (in bytes)
			
			BYTE *Ident = (BYTE*)malloc(sizeParamIdent * sizeof(BYTE));
			BYTE *Value = (BYTE*)malloc(sizeParamValue * sizeof(BYTE));
		
			io.read_proc(Ident, sizeParamIdent, 1, handle);
			io.read_proc(Value, sizeParamValue, 1, handle);
			
			free(Ident);
			free(Value);
		}

		break;

		// reserved for future use
		case 0x20:	// Type 01
		case 0x40:	// Type 10
			break;
	}
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
	return "WBMP";
}

static const char * DLL_CALLCONV
Description() {
	return "Wireless Bitmap";
}

static const char * DLL_CALLCONV
Extension() {
	return "wbmp,wap";
}

static const char * DLL_CALLCONV
RegExpr() {
	return NULL;
}

static const char * DLL_CALLCONV
MimeType() {
	return "image/vnd.wap.wbmp";
}

// ----------------------------------------------------------

static FIBITMAP * DLL_CALLCONV
Load(FreeImage &freeimage, FreeImageIO &io, fi_handle handle, int page, int flags, void *dataa) {
	WORD x, y, width, height;
	FIBITMAP *dib;
    BYTE *bits;		// pointer to dib data
	RGBQUAD *pal;	// pointer to dib palette

	WBMPHEADER header;

	if(handle) {
		try {
			// Read header information
			// -----------------------

			// Type

			header.TypeField = (WORD)multiByteRead(io, handle);

			if (header.TypeField != 0)
				throw "Unsupported WBMP type";			

			// FixHeaderField

			io.read_proc(&header.FixHeaderField, 1, 1, handle);

			// ExtHeaderFields
			// 1 = More will follow, 0 = Last octet

			if (header.FixHeaderField & 0x80) {
				header.ExtHeaderFields = 0x80;

				while(header.ExtHeaderFields & 0x80) {
					io.read_proc(&header.ExtHeaderFields, 1, 1, handle);

					readExtHeader(io, handle, header.ExtHeaderFields);
				}
			}

			// Width & Height

			width  = (WORD)multiByteRead(io, handle);
			height = (WORD)multiByteRead(io, handle);

			// Allocate a new dib

			dib = freeimage.allocate_proc(width, height, 1, 0, 0, 0);

			if (!dib) {
				throw "DIB allocation failed";
			}

			// write the palette data

			pal = freeimage.get_palette_proc(dib);
			pal[0].rgbRed = pal[0].rgbGreen = pal[0].rgbBlue = 0;
			pal[1].rgbRed = pal[1].rgbGreen = pal[1].rgbBlue = 255;

			// read the bitmap data
			
			int pitch = freeimage.get_pitch_proc(dib);
			int line = freeimage.get_line_proc(dib);

			for (y = 0; y < height; y++) {
				bits = freeimage.get_scanline_proc(dib, height - 1 - y);

				for (x = 0; x < line; x++) {
					io.read_proc(&bits[x], 1, 1, handle);
				}
			}

			return dib;

		} catch(char *text)  {
			freeimage.output_message_proc(s_format_id, text);

			return NULL;
		}

	}

	return NULL;
}

static BOOL DLL_CALLCONV
Save(FreeImage &freeimage, FreeImageIO &io, FIBITMAP *dib, fi_handle handle, int page, int flags, void *data) {
    BYTE *bits;	// pointer to dib data

	if ((dib) && (handle)) {
		try {
			if (freeimage.get_bpp_proc(dib) != 1)
				throw "Only 1-bit depth bitmaps can be saved as WBMP";

			// Write the header

			WBMPHEADER header;
			header.TypeField = 0;						// Type 0: B/W, no compression
			header.FixHeaderField = 0;					// No ExtHeaderField
			header.Width = freeimage.get_width_proc(dib);		// Image width
			header.Height = freeimage.get_height_proc(dib);	// Image height

			multiByteWrite(io, handle, header.TypeField);
			
			io.write_proc(&header.FixHeaderField, 1, 1, handle);

			multiByteWrite(io, handle, header.Width);
			multiByteWrite(io, handle, header.Height);

			// write the bitmap data

			WORD linelength = freeimage.get_line_proc(dib);

			for (WORD y = 0; y < header.Height; y++) {
				bits = freeimage.get_scanline_proc(dib, header.Height - 1 - y);

				io.write_proc(&bits[0], linelength, 1, handle);
			}

			return TRUE;

		} catch (char* text) {
			freeimage.output_message_proc(s_format_id, text);
		}
	}

	return FALSE;
}

// ==========================================================
//   Init
// ==========================================================

void DLL_CALLCONV
InitWBMP(Plugin &plugin, int format_id) {
	s_format_id = format_id;

	plugin.format_proc = Format;
	plugin.description_proc = Description;
	plugin.extension_proc = Extension;
	plugin.regexpr_proc = RegExpr;
	plugin.load_proc = Load;
	plugin.save_proc = Save;
	plugin.mime_proc = MimeType;
}
