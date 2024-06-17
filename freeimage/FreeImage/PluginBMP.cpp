	// ==========================================================
// BMP Loader and Writer
//
// Design and implementation by
// - Floris van den Berg (flvdberg@wxs.nl)
// - Markus Loibl (markus.loibl@epost.de)
// - Martin Weber (martweb@gmx.net)
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

#include <assert.h>

#include "FreeImage.h"
#include "Utilities.h"

// ----------------------------------------------------------
//   Constants + headers
// ----------------------------------------------------------

const int RLE_COMMAND     = 0;
const int RLE_ENDOFLINE   = 0;
const int RLE_ENDOFBITMAP = 1;
const int RLE_DELTA       = 2;

#define BI_RGB        0L
#define BI_RLE8       1L
#define BI_RLE4       2L
#define BI_BITFIELDS  3L

// ----------------------------------------------------------

#ifdef WIN32
#pragma pack(push, 1)
#else
#pragma pack(1)
#endif

typedef struct tagBITMAPCOREHEADER {
  DWORD   bcSize;
  WORD    bcWidth;
  WORD    bcHeight;
  WORD    bcPlanes;
  WORD    bcBitCnt;
} BITMAPCOREHEADER, *PBITMAPCOREHEADER; 

typedef struct tagBITMAPINFOOS2_1X_HEADER {
  DWORD  biSize;
  WORD   biWidth;
  WORD   biHeight; 
  WORD   biPlanes; 
  WORD   biBitCount;
} BITMAPINFOOS2_1X_HEADER, *PBITMAPINFOOS2_1X_HEADER; 

typedef struct tagBITMAPFILEHEADER {
  WORD    bfType; 
  DWORD   bfSize;
  WORD    bfReserved1; 
  WORD    bfReserved2;
  DWORD   bfOffBits; 
} BITMAPFILEHEADER, *PBITMAPFILEHEADER;

typedef struct tagRGBTRIPLE { 
  BYTE rgbtBlue; 
  BYTE rgbtGreen; 
  BYTE rgbtRed; 
} RGBTRIPLE; 

#ifdef WIN32
#pragma pack(pop)
#else
#pragma pack(4)
#endif

// ==========================================================
// Plugin Interface
// ==========================================================

static int s_format_id;

// ==========================================================
// Internal functions
// ==========================================================

static FIBITMAP *
LoadWindowsBMP(FreeImage &freeimage, FreeImageIO &io, fi_handle handle, int flags, unsigned bitmap_bits_offset) {
	FIBITMAP *dib;

	try {
		// load the info header

		BITMAPINFOHEADER bih;

		io.read_proc(&bih, sizeof(BITMAPINFOHEADER), 1, handle);

		// keep some general information about the bitmap

		int used_colors = bih.biClrUsed;
		int width       = bih.biWidth;
		int height      = bih.biHeight;
		int bit_count   = bih.biBitCount;
		int compression = bih.biCompression;
		int pitch       = CalculatePitch(CalculateLine(width, bit_count));

		switch (bit_count) {
			case 1 :
			case 4 :
			case 8 :
			{
				if ((used_colors <= 0) || (used_colors > CalculateUsedColors(bit_count)))
					used_colors = CalculateUsedColors(bit_count);					
				
				// allocate enough memory to hold the bitmap (header, palette, pixels) and read the palette

				dib = freeimage.allocate_proc(width, height, bit_count, 0, 0, 0);

				if (dib == NULL)
					throw "DIB allocation failed";						

				BITMAPINFOHEADER *pInfoHeader = freeimage.get_info_header_proc(dib);
				pInfoHeader->biXPelsPerMeter = bih.biXPelsPerMeter;
				pInfoHeader->biYPelsPerMeter = bih.biYPelsPerMeter;
				
				// load the palette

				io.read_proc(freeimage.get_palette_proc(dib), used_colors * sizeof(RGBQUAD), 1, handle);

				// seek to the actual pixel data.
				// this is needed because sometimes the palette is larger than the entries it contains predicts

				if (bitmap_bits_offset > (sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (used_colors * sizeof(RGBQUAD))))
					io.seek_proc(handle, bitmap_bits_offset, SEEK_SET);
				
				// read the pixel data

				switch (compression) {
					case BI_RGB :
						if (height > 0) {
							io.read_proc((void *)freeimage.get_bits_proc(dib), height * pitch, 1, handle);
						} else {
							for (int c = 0; c < abs(height); ++c) {
								io.read_proc((void *)freeimage.get_scanline_proc(dib, height - c - 1), pitch, 1, handle);								
							}
						}
						
						return dib;

					case BI_RLE4 :
					{
						BYTE status_byte = 0;
						BYTE second_byte = 0;
						int scanline = 0;
						int bits = 0;
						BOOL low_nibble = FALSE;

						for (;;) {
							io.read_proc(&status_byte, sizeof(BYTE), 1, handle);

							switch (status_byte) {
								case RLE_COMMAND :
									io.read_proc(&status_byte, sizeof(BYTE), 1, handle);

									switch (status_byte) {
										case RLE_ENDOFLINE :
											bits = 0;
											scanline++;
											low_nibble = FALSE;
											break;

										case RLE_ENDOFBITMAP :
											return (FIBITMAP *)dib;

										case RLE_DELTA :
										{
											// read the delta values

											BYTE delta_x;
											BYTE delta_y;

											io.read_proc(&delta_x, sizeof(BYTE), 1, handle);
											io.read_proc(&delta_y, sizeof(BYTE), 1, handle);

											// apply them

											bits       += delta_x / 2;
											scanline   += delta_y;
											break;
										}

										default :
											io.read_proc(&second_byte, sizeof(BYTE), 1, handle);

											BYTE *sline = freeimage.get_scanline_proc(dib, scanline);

											for (int i = 0; i < status_byte; i++) {
												if (low_nibble) {
													*(sline + bits) |= LOWNIBBLE(second_byte);

													if (i != status_byte - 1)
														io.read_proc(&second_byte, sizeof(BYTE), 1, handle);

													bits++;
												} else {
													*(sline + bits) |= HINIBBLE(second_byte);
												}
												
												low_nibble = !low_nibble;
											}

											if (((status_byte / 2) & 1 )== 1)
												io.read_proc(&second_byte, sizeof(BYTE), 1, handle);												

											break;
									};

									break;

								default :
								{
									BYTE *sline = freeimage.get_scanline_proc(dib, scanline);

									io.read_proc(&second_byte, sizeof(BYTE), 1, handle);

									for (unsigned i = 0; i < status_byte; i++) {
										if (low_nibble) {
											*(sline + bits) |= LOWNIBBLE(second_byte);

											bits++;
										} else {
											*(sline + bits) |= HINIBBLE(second_byte);
										}				
										
										low_nibble = !low_nibble;
									}
								}

								break;
							};
						}

						break;
					}

					case BI_RLE8 :
					{
						BYTE status_byte = 0;
						BYTE second_byte = 0;
						int scanline = 0;
						int bits = 0;

						for (;;) {
							io.read_proc(&status_byte, sizeof(BYTE), 1, handle);

							switch (status_byte) {
								case RLE_COMMAND :
									io.read_proc(&status_byte, sizeof(BYTE), 1, handle);

									switch (status_byte) {
										case RLE_ENDOFLINE :
											bits = 0;
											scanline++;
											break;

										case RLE_ENDOFBITMAP :
											return (FIBITMAP *)dib;

										case RLE_DELTA :
										{
											// read the delta values

											BYTE delta_x;
											BYTE delta_y;

											io.read_proc(&delta_x, sizeof(BYTE), 1, handle);
											io.read_proc(&delta_y, sizeof(BYTE), 1, handle);

											// apply them

											bits     += delta_x;
											scanline += delta_y;
											break;
										}

										default :
											io.read_proc((void *)(freeimage.get_scanline_proc(dib, scanline) + bits), sizeof(BYTE) * status_byte, 1, handle);
											
											// align run length to even number of bytes 

											if ((status_byte & 1) == 1)
												io.read_proc(&second_byte, sizeof(BYTE), 1, handle);												

											bits += status_byte;													

											break;								
									};

									break;

								default :
									BYTE *sline = freeimage.get_scanline_proc(dib, scanline);

									io.read_proc(&second_byte, sizeof(BYTE), 1, handle);

									for (unsigned i = 0; i < status_byte; i++) {
										*(sline + bits) = second_byte;

										bits++;					
									}

									break;
							};
						}

						break;
					}

					default :								
						throw "compression type not supported";
				}

				break;
			}

			case 16 :
			{
				if (bih.biCompression == BI_BITFIELDS) {
					DWORD bitfields[3];

					io.read_proc(bitfields, 3 * sizeof(DWORD), 1, handle);

					dib = freeimage.allocate_proc(width, height, bit_count, bitfields[2], bitfields[1], bitfields[0]);
				} else {
					dib = freeimage.allocate_proc(width, height, bit_count, 0x1F, 0x3E0, 0x7C00);
				}

				if (dib == NULL)
					throw "DIB allocation failed";						

				BITMAPINFOHEADER *pInfoHeader = freeimage.get_info_header_proc(dib);
				pInfoHeader->biXPelsPerMeter = bih.biXPelsPerMeter;
				pInfoHeader->biYPelsPerMeter = bih.biYPelsPerMeter;

				io.read_proc(freeimage.get_bits_proc(dib), height * pitch, 1, handle);

				return dib;
			}

			case 24 :
			case 32 :
			{
				if (bih.biCompression == BI_BITFIELDS) {
					throw "bitfields in 32-bit BMPs are currently unsupported";
				} else {
					dib = freeimage.allocate_proc(width, height, bit_count, 0xFF, 0xFF00, 0xFF0000);

					if (dib == NULL)
						throw "DIB allocation failed";						

					BITMAPINFOHEADER *pInfoHeader = freeimage.get_info_header_proc(dib);
					pInfoHeader->biXPelsPerMeter = bih.biXPelsPerMeter;
					pInfoHeader->biYPelsPerMeter = bih.biYPelsPerMeter;

					// Skip over the optional palette 
					// A 24 or 32 bit DIB may contain a palette for faster color reduction

					if (pInfoHeader->biClrUsed > 0)
						io.seek_proc(handle, pInfoHeader->biClrUsed * sizeof(RGBQUAD), SEEK_CUR);
					
					// read in the bitmap bits

					io.read_proc(freeimage.get_bits_proc(dib), height * pitch, 1, handle);

					// check if the bitmap contains transparency, if so enable it in the header

					freeimage.set_transparent_proc(dib, (freeimage.get_color_type_proc(dib) == FIC_RGBALPHA));

					return dib;
				}
			}
		}
	} catch(const char *message) {
		freeimage.output_message_proc(s_format_id, message);
	}

	return NULL;
}

static FIBITMAP *
LoadOS22XBMP(FreeImage &freeimage, FreeImageIO &io, fi_handle handle, int flags, unsigned bitmap_bits_offset) {
	FIBITMAP *dib = NULL;

	try {
		// load the info header

		BITMAPINFOHEADER bih;

		io.read_proc(&bih, sizeof(BITMAPINFOHEADER), 1, handle);

		// keep some general information about the bitmap

		int used_colors = bih.biClrUsed;
		int width       = bih.biWidth;
		int height      = bih.biHeight;
		int bit_count   = bih.biBitCount;
		int compression = bih.biCompression;
		int pitch       = CalculatePitch(CalculateLine(width, bit_count));
		
		switch (bit_count) {
			case 1 :
			case 4 :
			case 8 :
			{
				if ((used_colors <= 0) || (used_colors > CalculateUsedColors(bit_count)))
					used_colors = CalculateUsedColors(bit_count);
					
				// allocate enough memory to hold the bitmap (header, palette, pixels) and read the palette

				dib = freeimage.allocate_proc(width, height, bit_count, 0, 0, 0);

				if (dib == NULL)
					throw "DIB allocation failed";

				BITMAPINFOHEADER *pInfoHeader = freeimage.get_info_header_proc(dib);
				pInfoHeader->biXPelsPerMeter = bih.biXPelsPerMeter;
				pInfoHeader->biYPelsPerMeter = bih.biYPelsPerMeter;
				
				// load the palette

				io.seek_proc(handle, sizeof(BITMAPFILEHEADER) + bih.biSize, SEEK_SET);

				RGBQUAD *pal = freeimage.get_palette_proc(dib);

				for (int count = 0; count < used_colors; count++) {
					RGBTRIPLE triple;

					io.read_proc(&triple, sizeof(RGBTRIPLE), 1, handle);
					
					pal[count].rgbRed = triple.rgbtRed;
					pal[count].rgbGreen = triple.rgbtGreen;
					pal[count].rgbBlue = triple.rgbtBlue;
				}

				// seek to the actual pixel data.
				// this is needed because sometimes the palette is larger than the entries it contains predicts

				if (bitmap_bits_offset > (sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (used_colors * 3)))
					io.seek_proc(handle, bitmap_bits_offset, SEEK_SET);

				// read the pixel data

				switch (compression) {
					case BI_RGB :
						if (height > 0) {
							io.read_proc((void *)freeimage.get_bits_proc(dib), height * pitch, 1, handle);
						} else {
							for (int c = 0; c < abs(height); ++c) {
								io.read_proc((void *)freeimage.get_scanline_proc(dib, height - c - 1), pitch, 1, handle);								
							}
						}
						
						return dib;

					case BI_RLE4 :
					{
						BYTE status_byte = 0;
						BYTE second_byte = 0;
						int scanline = 0;
						int bits = 0;
						BOOL low_nibble = FALSE;

						for (;;) {
							io.read_proc(&status_byte, sizeof(BYTE), 1, handle);

							switch (status_byte) {
								case RLE_COMMAND :
									io.read_proc(&status_byte, sizeof(BYTE), 1, handle);

									switch (status_byte) {
										case RLE_ENDOFLINE :
											bits = 0;
											scanline++;
											low_nibble = FALSE;
											break;

										case RLE_ENDOFBITMAP :
											return (FIBITMAP *)dib;

										case RLE_DELTA :
										{
											// read the delta values

											BYTE delta_x;
											BYTE delta_y;

											io.read_proc(&delta_x, sizeof(BYTE), 1, handle);
											io.read_proc(&delta_y, sizeof(BYTE), 1, handle);

											// apply them

											bits       += delta_x / 2;
											scanline   += delta_y;
											break;
										}

										default :
											io.read_proc(&second_byte, sizeof(BYTE), 1, handle);

											BYTE *sline = freeimage.get_scanline_proc(dib, scanline);

											for (int i = 0; i < status_byte; i++) {
												if (low_nibble) {
													*(sline + bits) |= LOWNIBBLE(second_byte);

													if (i != status_byte - 1)
														io.read_proc(&second_byte, sizeof(BYTE), 1, handle);

													bits++;
												} else {
													*(sline + bits) |= HINIBBLE(second_byte);
												}
												
												low_nibble = !low_nibble;
											}

											if (((status_byte / 2) & 1 ) == 1)
												io.read_proc(&second_byte, sizeof(BYTE), 1, handle);												

											break;
									};

									break;

								default :
								{
									BYTE *sline = freeimage.get_scanline_proc(dib, scanline);

									io.read_proc(&second_byte, sizeof(BYTE), 1, handle);

									for (unsigned i = 0; i < status_byte; i++) {
										if (low_nibble) {
											*(sline + bits) |= LOWNIBBLE(second_byte);

											bits++;
										} else {
											*(sline + bits) |= HINIBBLE(second_byte);
										}				
										
										low_nibble = !low_nibble;
									}
								}

								break;
							};
						}

						break;
					}

					case BI_RLE8 :
					{
						BYTE status_byte = 0;
						BYTE second_byte = 0;
						int scanline = 0;
						int bits = 0;

						for (;;) {
							io.read_proc(&status_byte, sizeof(BYTE), 1, handle);

							switch (status_byte) {
								case RLE_COMMAND :
									io.read_proc(&status_byte, sizeof(BYTE), 1, handle);

									switch (status_byte) {
										case RLE_ENDOFLINE :
											bits = 0;
											scanline++;
											break;

										case RLE_ENDOFBITMAP :
											return (FIBITMAP *)dib;

										case RLE_DELTA :
										{
											// read the delta values

											BYTE delta_x;
											BYTE delta_y;

											io.read_proc(&delta_x, sizeof(BYTE), 1, handle);
											io.read_proc(&delta_y, sizeof(BYTE), 1, handle);

											// apply them

											bits     += delta_x;
											scanline += delta_y;
											break;
										}

										default :
											io.read_proc((void *)(freeimage.get_scanline_proc(dib, scanline) + bits), sizeof(BYTE) * status_byte, 1, handle);
											
											// align run length to even number of bytes 

											if (status_byte & 1 == 1)
												io.read_proc(&second_byte, sizeof(BYTE), 1, handle);												

											bits += status_byte;													

											break;								
									};

									break;

								default :
									BYTE *sline = freeimage.get_scanline_proc(dib, scanline);

									io.read_proc(&second_byte, sizeof(BYTE), 1, handle);

									for (unsigned i = 0; i < status_byte; i++) {
										*(sline + bits) = second_byte;

										bits++;					
									}

									break;
							};
						}

						break;
					}

					default :		
						throw "compression type not supported";
				}						

				break;
			}

			case 16 :
			{
				if (bih.biCompression == 3) {
					DWORD bitfields[3];

					io.read_proc(bitfields, 3 * sizeof(DWORD), 1, handle);

					dib = freeimage.allocate_proc(width, height, bit_count, bitfields[2], bitfields[1], bitfields[0]);
				} else {
					dib = freeimage.allocate_proc(width, height, bit_count, 0x1F, 0x3E0, 0x7C00);
				}

				if (dib == NULL)
					throw "DIB allocation failed";						

				BITMAPINFOHEADER *pInfoHeader = freeimage.get_info_header_proc(dib);
				pInfoHeader->biXPelsPerMeter = bih.biXPelsPerMeter;
				pInfoHeader->biYPelsPerMeter = bih.biYPelsPerMeter;

				if (bitmap_bits_offset > (sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (used_colors * 3)))
					io.seek_proc(handle, bitmap_bits_offset, SEEK_SET);

				io.read_proc(freeimage.get_bits_proc(dib), height * pitch, 1, handle);

				return dib;
			}

			case 24 :
			case 32 :
			{
				dib = freeimage.allocate_proc(width, height, bit_count, 0xFF, 0xFF00, 0xFF0000);

				if (dib == NULL)
					throw "DIB allocation failed";
				
				BITMAPINFOHEADER *pInfoHeader = freeimage.get_info_header_proc(dib);
				pInfoHeader->biXPelsPerMeter = bih.biXPelsPerMeter;
				pInfoHeader->biYPelsPerMeter = bih.biYPelsPerMeter;

				// Skip over the optional palette 
				// A 24 or 32 bit DIB may contain a palette for faster color reduction

				if (bitmap_bits_offset > (sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (used_colors * 3)))
					io.seek_proc(handle, bitmap_bits_offset, SEEK_SET);
				
				// read in the bitmap bits

				io.read_proc(freeimage.get_bits_proc(dib), height * pitch, 1, handle);

				// check if the bitmap contains transparency, if so enable it in the header

				if (freeimage.get_color_type_proc(dib) == FIC_RGBALPHA)
					freeimage.set_transparent_proc(dib, TRUE);
				else
					freeimage.set_transparent_proc(dib, FALSE);

				return dib;
			}
		}
	} catch(const char *message) {
		freeimage.output_message_proc(s_format_id, message);
	}

	return NULL;
}

static FIBITMAP *
LoadOS21XBMP(FreeImage &freeimage, FreeImageIO &io, fi_handle handle, int flags, unsigned bitmap_bits_offset) {
	FIBITMAP *dib = NULL;

	try {
		BITMAPINFOOS2_1X_HEADER bios2_1x;

		io.read_proc(&bios2_1x, sizeof(BITMAPINFOOS2_1X_HEADER), 1, handle);

		// keep some general information about the bitmap

		int used_colors = 0;
		int width       = bios2_1x.biWidth;
		int height      = bios2_1x.biHeight;
		int bit_count   = bios2_1x.biBitCount;
		int compression = 0;
		int pitch       = CalculatePitch(CalculateLine(width, bit_count));
		
		switch (bit_count) {
			case 1 :
			case 4 :
			case 8 :
			{
				used_colors = CalculateUsedColors(bit_count);					
				
				// allocate enough memory to hold the bitmap (header, palette, pixels) and read the palette

				dib = freeimage.allocate_proc(width, height, bit_count, 0, 0, 0);

				if (dib == NULL)
					throw "DIB allocation failed";						

				BITMAPINFOHEADER *pInfoHeader = freeimage.get_info_header_proc(dib);
				pInfoHeader->biXPelsPerMeter = 0;
				pInfoHeader->biYPelsPerMeter = 0;
				
				// load the palette

				RGBQUAD *pal = freeimage.get_palette_proc(dib);

				for (int count = 0; count < used_colors; count++) {
					RGBTRIPLE triple;

					io.read_proc(&triple, sizeof(RGBTRIPLE), 1, handle);
					
					pal[count].rgbRed = triple.rgbtRed;
					pal[count].rgbGreen = triple.rgbtGreen;
					pal[count].rgbBlue = triple.rgbtBlue;
				}

				// Skip over the optional palette 
				// A 24 or 32 bit DIB may contain a palette for faster color reduction

				io.seek_proc(handle, bitmap_bits_offset, SEEK_SET);
				
				// read the pixel data

				if (height > 0) {
					io.read_proc((void *)freeimage.get_bits_proc(dib), height * pitch, 1, handle);
				} else {
					for (int c = 0; c < abs(height); ++c) {
						io.read_proc((void *)freeimage.get_scanline_proc(dib, height - c - 1), pitch, 1, handle);								
					}
				}
						
				return dib;
			}

			case 16 :
			{
				dib = freeimage.allocate_proc(width, height, bit_count, 0x1F, 0x3E0, 0x7C00);

				if (dib == NULL)
					throw "DIB allocation failed";						

				BITMAPINFOHEADER *pInfoHeader = freeimage.get_info_header_proc(dib);
				pInfoHeader->biXPelsPerMeter = 0;
				pInfoHeader->biYPelsPerMeter = 0;

				io.read_proc(freeimage.get_bits_proc(dib), height * pitch, 1, handle);

				return dib;
			}

			case 24 :
			case 32 :
			{
				dib = freeimage.allocate_proc(width, height, bit_count, 0xFF, 0xFF00, 0xFF0000);

				if (dib == NULL)
					throw "DIB allocation failed";						

				BITMAPINFOHEADER *pInfoHeader = freeimage.get_info_header_proc(dib);
				pInfoHeader->biXPelsPerMeter = 0;
				pInfoHeader->biYPelsPerMeter = 0;

				// Skip over the optional palette 
				// A 24 or 32 bit DIB may contain a palette for faster color reduction

				io.read_proc(freeimage.get_bits_proc(dib), height * pitch, 1, handle);

				// check if the bitmap contains transparency, if so enable it in the header

				freeimage.set_transparent_proc(dib, (freeimage.get_color_type_proc(dib) == FIC_RGBALPHA));

				return dib;
			}
		}
	} catch(const char *message) {	
		freeimage.output_message_proc(s_format_id, message);
	}

	return NULL;
}

// ==========================================================
// Plugin Implementation
// ==========================================================

static const char * DLL_CALLCONV
Format() {
	return "BMP";
}

static const char * DLL_CALLCONV
Description() {
	return "Windows or OS/2 Bitmap";
}

static const char * DLL_CALLCONV
Extension() {
	return "bmp";
}

static const char * DLL_CALLCONV
RegExpr() {
	return "^BM";
}

static BOOL DLL_CALLCONV
Validate(FreeImageIO &io, fi_handle handle) {
	BYTE bmp_signature1[] = { 0x42, 0x4D };
	BYTE bmp_signature2[] = { 0x42, 0x41 };
	BYTE signature[2];

	int items_read = io.read_proc(signature, 1, sizeof(bmp_signature1), handle);

	if (memcmp(bmp_signature1, signature, sizeof(bmp_signature1)) == 0)
		return TRUE;

	if (memcmp(bmp_signature2, signature, sizeof(bmp_signature2)) == 0)
		return TRUE;

	return FALSE;
}

// ----------------------------------------------------------

static FIBITMAP * DLL_CALLCONV
Load(FreeImage &freeimage, FreeImageIO &io, fi_handle handle, int page, int flags, void *data) {
	if (handle != NULL) {
		BITMAPFILEHEADER bitmapfileheader;
		DWORD type = 0;
		WORD magic;

		// we use this offset value to make seemingly absolute seeks relative in the file
		
		long offset_in_file = io.tell_proc(handle);

		// read the magic

		io.read_proc(&magic, sizeof(WORD), 1, handle);

		// compare the magic with the number we know

		while (memcmp(&magic, "BA", 2) == 0) {
			io.read_proc(&bitmapfileheader.bfSize, sizeof(DWORD), 1, handle);
			io.read_proc(&bitmapfileheader.bfReserved1, sizeof(WORD), 1, handle);
			io.read_proc(&bitmapfileheader.bfReserved2, sizeof(WORD), 1, handle);
			io.read_proc(&bitmapfileheader.bfOffBits, sizeof(DWORD), 1, handle);
			io.read_proc(&magic, sizeof(WORD), 1, handle);
		}

		// read the fileheader

		io.seek_proc(handle, 0 - sizeof(WORD), SEEK_CUR);
		io.read_proc(&bitmapfileheader, sizeof(BITMAPFILEHEADER), 1, handle);

		// read the first byte of the infoheader

		io.read_proc(&type, sizeof(DWORD), 1, handle);
		io.seek_proc(handle, 0 - sizeof(DWORD), SEEK_CUR);

		// call the appropriate load function for the found bitmap type
		
		if (type == 40)
			return LoadWindowsBMP(freeimage, io, handle, flags, offset_in_file + bitmapfileheader.bfOffBits);
		
		if (type == 12)
			return LoadOS21XBMP(freeimage, io, handle, flags, offset_in_file + bitmapfileheader.bfOffBits);

		if (type <= 64)
			return LoadOS22XBMP(freeimage, io, handle, flags, offset_in_file + bitmapfileheader.bfOffBits);		
	}

	return NULL;
}

// ----------------------------------------------------------

static BOOL DLL_CALLCONV
Save(FreeImage &freeimage, FreeImageIO &io, FIBITMAP *dib, fi_handle handle, int page, int flags, void *data) {
	if ((dib != NULL) && (handle != NULL)) {
		bool bit_fields = (freeimage.get_bpp_proc(dib) == 16);

		// write the file header

		BITMAPFILEHEADER bitmapfileheader;
		bitmapfileheader.bfType = 0x4D42;
		bitmapfileheader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + freeimage.get_height_proc(dib) * freeimage.get_pitch_proc(dib);
		bitmapfileheader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + freeimage.get_colors_used_proc(dib) * sizeof(RGBQUAD);
		bitmapfileheader.bfReserved1 = 0;
		bitmapfileheader.bfReserved2 = 0;

		if (bit_fields) {
			bitmapfileheader.bfSize += 3 * sizeof(DWORD);
			bitmapfileheader.bfOffBits += 3 * sizeof(DWORD);
		}

		if (io.write_proc(&bitmapfileheader, sizeof(BITMAPFILEHEADER), 1, handle) != 1)
			return FALSE;		

		// write the info header

		BITMAPINFOHEADER bih = *freeimage.get_info_header_proc(dib);

		bih.biCompression = (bit_fields) ? 3 : 0;
		bih.biBitCount = ((freeimage.get_bpp_proc(dib) == 32) && (!freeimage.is_transparent_proc(dib))) ? 24 : bih.biBitCount;

		if (io.write_proc(&bih, sizeof(BITMAPINFOHEADER), 1, handle) != 1) {
			return FALSE;
		}

		// write the bit fields when we are dealing with a 16 bit BMP

		if (bit_fields) {
			DWORD d;

			d = freeimage.get_blue_mask_proc(dib);

			if (io.write_proc(&d, sizeof(DWORD), 1, handle) != 1)
				return FALSE;

			d = freeimage.get_green_mask_proc(dib);

			if (io.write_proc(&d, sizeof(DWORD), 1, handle) != 1)
				return FALSE;

			d = freeimage.get_red_mask_proc(dib);

			if (io.write_proc(&d, sizeof(DWORD), 1, handle) != 1)
				return FALSE;
		}

		// write the palette

		if (freeimage.get_palette_proc(dib) != NULL)
			if (io.write_proc(freeimage.get_palette_proc(dib), sizeof(RGBQUAD) * freeimage.get_colors_used_proc(dib), 1, handle) != 1)
				return FALSE;

		// write the bitmap data

		if ((freeimage.get_bpp_proc(dib) == 32) && (!freeimage.is_transparent_proc(dib))) {
			int pitch = CalculatePitch(CalculateLine(freeimage.get_width_proc(dib), 24));

			BYTE *buffer = (BYTE *)malloc(pitch);

			for (unsigned i = 0; i < freeimage.get_height_proc(dib); ++i) {
				freeimage.convert_line_32to24_proc(buffer, freeimage.get_scanline_proc(dib, i), freeimage.get_width_proc(dib));

				if (io.write_proc(buffer, pitch, 1, handle) != 1) {
					free(buffer);
					return FALSE;
				}
			}

			free(buffer);
		} else {
			if (io.write_proc(freeimage.get_bits_proc(dib), freeimage.get_height_proc(dib) * freeimage.get_pitch_proc(dib), 1, handle) != 1) {
				return FALSE;
			}
		}

		return TRUE;
	} else {
		return FALSE;
	}
}

// ==========================================================
//   Init
// ==========================================================

void DLL_CALLCONV
InitBMP(Plugin &plugin, int format_id) {
	s_format_id = format_id;

	plugin.format_proc = Format;
	plugin.description_proc = Description;
	plugin.extension_proc = Extension;
	plugin.regexpr_proc = RegExpr;
	plugin.load_proc = Load;
	plugin.save_proc = 0;	//Save;
	plugin.validate_proc = Validate;
}
