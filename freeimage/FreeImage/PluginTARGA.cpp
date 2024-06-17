// ==========================================================
// TARGA Loader
//
// Design and implementation by
// - Floris van den Berg (flvdberg@wxs.nl)
// - Jani Kajala (janik@remedy.fi)
// - Martin Weber (martweb@gmx.net)
// - Machiel ten Brinke (brinkem@uni-one.nl)
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

#ifdef WIN32
#pragma pack(push, 1)
#else
#pragma pack(1)
#endif

typedef struct tagRGBTRIPLE { 
  BYTE rgbtBlue; 
  BYTE rgbtGreen;
  BYTE rgbtRed;
} RGBTRIPLE; 

typedef struct tagBGRAQUAD { 
  BYTE bgraBlue; 
  BYTE bgraGreen; 
  BYTE bgraRed;
  BYTE bgraAlpha;
} BGRAQUAD; 

struct tagTGAHEADER {
	BYTE id_length;
	BYTE color_map_type;
	BYTE image_type;

	WORD cm_first_entry;
	WORD cm_length;
	BYTE cm_size;

	WORD is_xorigin;
	WORD is_yorigin;
	WORD is_width;
	WORD is_height;
	BYTE is_pixel_depth;
	BYTE is_image_descriptor;
};

#ifdef WIN32
#pragma pack(pop)
#else
#pragma pack(4)
#endif

// ==========================================================
// Internal functions
// ==========================================================

static BYTE *
Internal_GetScanLine(FreeImage &freeimage, FIBITMAP *dib, int scanline, int flipvert) {
	//assert ((scanline >= 0) && (scanline < (int)freeimage.get_height_proc(dib)));

	if (flipvert) {
		return freeimage.get_scanline_proc(dib, scanline);
	} else {
		return freeimage.get_scanline_proc(dib, freeimage.get_height_proc(dib) - scanline - 1);
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
	return "TARGA";
}

static const char * DLL_CALLCONV
Description() {
	return "Truevision Targa";
}

static const char * DLL_CALLCONV
Extension() {
	return "tga,targa";
}

static const char * DLL_CALLCONV
RegExpr() {
	return NULL;
}

// ----------------------------------------------------------

static FIBITMAP * DLL_CALLCONV
Load(FreeImage &freeimage, FreeImageIO &io, fi_handle handle, int page, int flags, void *data) {
	if (handle) {
		try {
			// remember the start offset

			long start_offset = io.tell_proc(handle);

			// read and process the bitmap's header

			FIBITMAP *dib = NULL;
			tagTGAHEADER header;

			io.read_proc(&header, sizeof(tagTGAHEADER), 1, handle);

			int line = CalculateLine(header.is_width, header.is_pixel_depth);
			int pitch = CalculatePitch(line);
			int alphabits = header.is_image_descriptor & 0x0f;
			int fliphoriz = (header.is_image_descriptor & 0x10) ? 0 : 1;
			int flipvert = (header.is_image_descriptor & 0x20) ? 1 : 0;

			io.seek_proc(handle, header.id_length, SEEK_CUR);
			switch (header.is_pixel_depth) {
				case 8 :
				{
					dib = freeimage.allocate_proc(header.is_width, header.is_height, 8, 0, 0, 0);

					if (dib == NULL) {
						throw "DIB allocation failed";
					}

					// read the palette

					RGBQUAD *palette = freeimage.get_palette_proc(dib);

					if (header.color_map_type == 0)
						for (unsigned i = 0; i < 256; i++) {
							palette[i].rgbRed	= i;
							palette[i].rgbGreen = i;
							palette[i].rgbBlue	= i;
						}
					else if (alphabits) {
						for (unsigned count = header.cm_first_entry; count < header.cm_length; count++) {
							BGRAQUAD quad;

							io.read_proc(&quad, sizeof(RGBTRIPLE), 1, handle);
						
							palette[count].rgbBlue = quad.bgraBlue;
							palette[count].rgbRed = quad.bgraRed;
							palette[count].rgbGreen = quad.bgraGreen;
							palette[count].rgbReserved = quad.bgraAlpha;
						}
					}
					else {
						for (unsigned count = header.cm_first_entry; count < header.cm_length; count++) {
							RGBTRIPLE triple;

							io.read_proc(&triple, sizeof(RGBTRIPLE), 1, handle);
						
							palette[count].rgbRed = triple.rgbtRed;
							palette[count].rgbGreen = triple.rgbtGreen;
							palette[count].rgbBlue = triple.rgbtBlue;
						}
					}
					
					// read in the bitmap bits
					switch (header.image_type) {
						case 1 :
						case 3 :
						{
							if (fliphoriz) {
								for (unsigned count = header.is_height; count > 0; count--)
									io.read_proc(Internal_GetScanLine(freeimage, dib, count - 1, flipvert), line, 1, handle);

							}
							else {
								for (unsigned count = 0; count < header.is_height; count++)
									io.read_proc(Internal_GetScanLine(freeimage, dib, count, flipvert), line, 1, handle);

							}
							
							break;
						}

						case 9 :
						case 11:
						{
							int x = 0;
							int y = 0;
							BYTE *bits;

							if (fliphoriz)
								bits = Internal_GetScanLine(freeimage, dib, header.is_height - y - 1, flipvert);
							else
								bits = Internal_GetScanLine(freeimage, dib, y, flipvert);

							BYTE rle;
							
							while(1) {
								io.read_proc(&rle,1, 1, handle);
								
								if (rle>127) {
									rle -= 127;

									BYTE triple;

									io.read_proc(&triple, 1, 1, handle);

									for (int ix = 0; ix < rle; ix++) {
										bits[x++] = triple;

										if (x >= line) {
											x = 0;

											y++;

											if (y >= header.is_height)
												goto done89;
											
											if(fliphoriz)
												bits = Internal_GetScanLine(freeimage, dib, header.is_height-y-1, flipvert);
											else
												bits = Internal_GetScanLine(freeimage, dib, y, flipvert);
										}
									}
								} else {
									rle++;

									for (int ix = 0; ix < rle; ix++) {
										BYTE triple;		

										io.read_proc(&triple, 1, 1, handle);

										bits[x++] = triple;
										
										if (x >= line) {
											x = 0;

											y++;

											if (y >= header.is_height)
												goto done89;											

											if(fliphoriz)
												bits = Internal_GetScanLine(freeimage, dib, header.is_height-y-1, flipvert);
											else
												bits = Internal_GetScanLine(freeimage, dib, y, flipvert);
										}
									}
								}
							}
					done89 :
							break;
						}
						
						default :
							freeimage.free_proc(dib);
							return NULL;
					}

					break;
				}

				case 15 :
				case 16 :
				{
					int pixel_bits;

					// allocate the dib

					if (TARGA_LOAD_RGB888 & flags) {
						pixel_bits = 24;

						dib = freeimage.allocate_proc(header.is_width, header.is_height, pixel_bits, 0xFF, 0xFF00, 0xFF0000);
					} else {			
						pixel_bits = 16;

						dib = freeimage.allocate_proc(header.is_width, header.is_height, pixel_bits, 0x1F, 0x3E0, 0x7C00);
					}

					if (dib == NULL)
						throw "DIB allocation failed";

					int line = CalculateLine(header.is_width, pixel_bits);
					int pitch = CalculatePitch(line);

					const unsigned pixel_size = unsigned(pixel_bits) / 8;

					// note header.cm_size is a misleading name, it should be seen as header.cm_bits 
					// ignore current position in file and set filepointer explicitly from the beginning of the file

					int garblen = 0;

					if (header.color_map_type != 0)
						garblen = (int)((header.cm_size + 7) / 8) * header.cm_length; /* should byte align */
					else
						garblen = 0;

					io.seek_proc(handle, start_offset, SEEK_SET);
					io.seek_proc(handle, sizeof(tagTGAHEADER) + header.id_length + garblen, SEEK_SET);

					// read in the bitmap bits

					WORD pixel;
							
					switch (header.image_type) {
						case 2 :
						{
							for (int y = 0; y < header.is_height; y++) {
								BYTE *bits;
								if(fliphoriz)
									bits = Internal_GetScanLine(freeimage, dib, header.is_height-y-1, flipvert);
								else
									bits = Internal_GetScanLine(freeimage, dib, y, flipvert);

								for (int x = 0; x < line; ) {
									io.read_proc(&pixel, sizeof(WORD), 1, handle);
								
									if (TARGA_LOAD_RGB888 & flags) {
										bits[x + 0] = ((pixel & 0x1F) * 0xFF) / 0x1F;
										bits[x + 1] = (((pixel & 0x3E0) >> 5) * 0xFF) / 0x1F;
										bits[x + 2] = (((pixel & 0x7C00) >> 10) * 0xFF) / 0x1F;
									} else {
										*reinterpret_cast<WORD*>(bits + x) = 0x7FFF & pixel;
									}

									x += pixel_size;
								}
							}

							break;
						}

						case 10 :
						{
							int x = 0;
							int y = 0;
							BYTE rle;
							WORD pixel;

							while(1) {
								BYTE *bits;

								if(fliphoriz)
									bits = Internal_GetScanLine(freeimage, dib, header.is_height-y-1, flipvert);
								else
									bits = Internal_GetScanLine(freeimage, dib, y, flipvert);
		
								io.read_proc(&rle,1, 1, handle);
								
								// compressed block
								
								if (rle > 127) {
									rle -= 127;

									io.read_proc(&pixel, sizeof(WORD), 1, handle);
								
									for (int ix = 0; ix < rle; ix++) {
										if (TARGA_LOAD_RGB888 & flags) {
											bits[x + 0] = ((pixel & 0x1F) * 0xFF) / 0x1F;
											bits[x + 1] = (((pixel & 0x3E0) >> 5) * 0xFF) / 0x1F;
											bits[x + 2] = (((pixel & 0x7C00) >> 10) * 0xFF) / 0x1F;
										} else {
											*reinterpret_cast<WORD *>(bits + x) = 0x7FFF & pixel;
										}

										x += pixel_size;
										
										if (x >= line) {
											x = 0;
											y++;

											if (y >= header.is_height)
												goto done2;																
										}
									}
								} else {
									rle++;

									for (int ix = 0; ix < rle; ix++) {
										io.read_proc(&pixel, sizeof(WORD), 1, handle);

										if (TARGA_LOAD_RGB888 & flags) {
											bits[x + 0] = ((pixel & 0x1F) * 0xFF) / 0x1F;
											bits[x + 1] = (((pixel & 0x3E0) >> 5) * 0xFF) / 0x1F;
											bits[x + 2] = (((pixel & 0x7C00) >> 10) * 0xFF) / 0x1F;
										} else {
											*reinterpret_cast<WORD*>(bits + x) = 0x7FFF & pixel;
										}

										x += pixel_size;

										if (x >= line) {
											x = 0;
											y++;

											if (y >= header.is_height)
												goto done2;																
										}
									}
								}
							}

					done2 :
							break;
						}

						default :
							freeimage.free_proc(dib);
							return NULL;
					}

					break;
				}

				case 24 :
				{
					dib = freeimage.allocate_proc(header.is_width, header.is_height, 24, 0xFF, 0xFF00, 0xFF0000);

					if (dib == 0)
						throw "DIB allocation failed";					

					// read in the bitmap bits

					switch (header.image_type) {
						case 2 :
						{
							if (fliphoriz)
								for (unsigned count = header.is_height; count > 0; count--) {
									BYTE *bits = bits = Internal_GetScanLine(freeimage, dib, count-1, flipvert);

									io.read_proc(bits, line, 1, handle);

									bits += pitch;
								}
							else
								for (unsigned count = 0; count < header.is_height; count++) {
									BYTE *bits = bits = Internal_GetScanLine(freeimage, dib, count, flipvert);

									io.read_proc(bits, line, 1, handle);

									bits += pitch;
								}

							break;
						}

						case 10 :
						{
							int x = 0;
							int y = 0;
							BYTE rle;
							BYTE *bits;
							
							if(fliphoriz)
								bits = Internal_GetScanLine(freeimage, dib, header.is_height-y-1, flipvert);
							else
								bits = Internal_GetScanLine(freeimage, dib, y, flipvert);
							
							if (alphabits) {
								while(1) {
									io.read_proc(&rle,1, 1, handle);
									
									if (rle>127) {
										rle -= 127;

										BGRAQUAD quad;

										io.read_proc(&quad, sizeof(BGRAQUAD), 1, handle);

										for (int ix = 0; ix < rle; ix++) {
											bits[x++] = quad.bgraBlue;
											bits[x++] = quad.bgraGreen;
											bits[x++] = quad.bgraRed;
											bits[x++] = quad.bgraAlpha;

											if (x >= line) {
												x = 0;
												y++;

												if (y >= header.is_height)
													goto done243;

												if(fliphoriz)
													bits = Internal_GetScanLine(freeimage, dib, header.is_height-y-1, flipvert);
												else
													bits = Internal_GetScanLine(freeimage, dib, y, flipvert);
											}
										}
									} else {
										rle++;

										for (int ix = 0; ix < rle; ix++) {
											BGRAQUAD quad;

											io.read_proc(&quad, sizeof(BGRAQUAD), 1, handle);

											bits[x++] = quad.bgraBlue;
											bits[x++] = quad.bgraGreen;
											bits[x++] = quad.bgraRed;
											bits[x++] = quad.bgraAlpha;
											
											if (x >= line) {
												x = 0;
												y++;

												if (y >= header.is_height)
													goto done243;											

												if(fliphoriz)
													bits = Internal_GetScanLine(freeimage, dib, header.is_height-y-1, flipvert);
												else
													bits = Internal_GetScanLine(freeimage, dib, y, flipvert);
											}
										}
									}
								}
							} else {
								while (1) {
									io.read_proc(&rle,1, 1, handle);
									
									if (rle>127) {
										rle -= 127;

										RGBTRIPLE triple;

										io.read_proc(&triple, sizeof(RGBTRIPLE), 1, handle);

										for (int ix = 0; ix < rle; ix++) {
											bits[x++] = triple.rgbtBlue;
											bits[x++] = triple.rgbtGreen;
											bits[x++] = triple.rgbtRed;

											if (x >= line) {
												x = 0;
												y++;

												if (y >= header.is_height)
													goto done243;											
												
												if(fliphoriz)
													bits = Internal_GetScanLine(freeimage, dib, header.is_height-y-1, flipvert);
												else
													bits = Internal_GetScanLine(freeimage, dib, y, flipvert);
											}
										}
									} else {
										rle++;

										for (int ix = 0; ix < rle; ix++) {
											RGBTRIPLE triple;		

											io.read_proc(&triple, sizeof(RGBTRIPLE), 1, handle);

											bits[x++] = triple.rgbtBlue;
											bits[x++] = triple.rgbtGreen;
											bits[x++] = triple.rgbtRed;
											
											if (x >= line) {
												x = 0;
												y++;

												if (y >= header.is_height)
													goto done243;											

												if(fliphoriz)
													bits = Internal_GetScanLine(freeimage, dib, header.is_height-y-1, flipvert);
												else
													bits = Internal_GetScanLine(freeimage, dib, y, flipvert);
											}
										}
									}
								}
							}
					done243 :
							break;
						}

						default :
							freeimage.free_proc(dib);
							return NULL;
					}

					break;
				}
				
				case 32 :
				{
					int pixel_bits;

					if (TARGA_LOAD_RGB888 & flags) {
						pixel_bits = 24;

						line = CalculateLine(header.is_width, pixel_bits);
						pitch = CalculatePitch(line);
					} else {
						pixel_bits = 32;
					}

					const unsigned pixel_size = unsigned (pixel_bits) / 8;

					// Allocate the DIB

					dib = freeimage.allocate_proc(header.is_width, header.is_height, pixel_bits, 0xFF, 0xFF00, 0xFF0000);
					
					if (dib == 0)
						throw "DIB allocation failed";					

					// read in the bitmap bits

					switch (header.image_type) {
						case 2 :
						{
							// uncompressed

							if (alphabits) {
								if (fliphoriz)
									for (unsigned count = header.is_height; count > 0; count--) {
										BYTE *bits = bits = Internal_GetScanLine(freeimage, dib, count-1, flipvert);

										for (unsigned cols = 0; cols < header.is_width; cols++) {
											RGBQUAD rgb;

											io.read_proc(&rgb, sizeof(RGBQUAD), 1, handle);

											bits[0] = rgb.rgbBlue;
											bits[1] = rgb.rgbGreen;
											bits[2] = rgb.rgbRed;

											if ((TARGA_LOAD_RGB888 & flags) != TARGA_LOAD_RGB888)
												bits[3] = rgb.rgbReserved;											

											bits += pixel_size;
										}
									}
								else
									for (unsigned count = 0; count < header.is_height; count++) {
										BYTE *bits = Internal_GetScanLine(freeimage, dib, count, flipvert);

										for (unsigned cols = 0; cols < header.is_width; cols++) {
											RGBQUAD rgb;

											io.read_proc(&rgb, sizeof(RGBQUAD), 1, handle);

											bits[0] = rgb.rgbBlue;
											bits[1] = rgb.rgbGreen;
											bits[2] = rgb.rgbRed;

											if ((TARGA_LOAD_RGB888 & flags) != TARGA_LOAD_RGB888)
												bits[3] = rgb.rgbReserved;											

											bits += pixel_size;
										}
									}

							} else {
								if (fliphoriz)
									for (unsigned count = header.is_height; count > 0; count--) {
										BYTE *bits;

										if (fliphoriz)
											bits = Internal_GetScanLine(freeimage, dib, header.is_height-count, flipvert);
										else
											bits = Internal_GetScanLine(freeimage, dib, count-1, flipvert);

										for (unsigned cols = 0; cols < header.is_width; cols++) {
											RGBQUAD rgb;

											io.read_proc(&rgb, sizeof(RGBQUAD), 1, handle);

											bits[0] = rgb.rgbBlue;
											bits[1] = rgb.rgbGreen;
											bits[2] = rgb.rgbRed;

											if ((TARGA_LOAD_RGB888 & flags) != TARGA_LOAD_RGB888)
												bits[3] = rgb.rgbReserved;											

											bits += pixel_size;
										}
									}
								else
									for (unsigned count = 0; count < header.is_height; count++) {
										BYTE *bits;

										if(fliphoriz)
											bits = Internal_GetScanLine(freeimage, dib, header.is_height-count-1, flipvert);
										else
											bits = Internal_GetScanLine(freeimage, dib, count, flipvert);

										for (unsigned cols = 0; cols < header.is_width; cols++) {
											RGBQUAD rgb;

											io.read_proc(&rgb, sizeof(RGBQUAD), 1, handle);

											bits[0] = rgb.rgbBlue;
											bits[1] = rgb.rgbGreen;
											bits[2] = rgb.rgbRed;

											if ((TARGA_LOAD_RGB888 & flags) != TARGA_LOAD_RGB888)
												bits[3] = rgb.rgbReserved;											

											bits += pixel_size;
										}
									}
							}

							break;
						}
						case 10:
						{
							int x = 0;
							int y = 0;
							BYTE rle;
							BYTE *bits;
							
							if(fliphoriz)
								bits = Internal_GetScanLine(freeimage, dib, header.is_height-y-1, flipvert);
							else
								bits = Internal_GetScanLine(freeimage, dib, y, flipvert);
							
							while(1) {
								io.read_proc(&rle,1, 1, handle);
									
								if (rle>127) {
									rle -= 127;

									BGRAQUAD quad;

									io.read_proc(&quad, sizeof(BGRAQUAD), 1, handle);

									for (int ix = 0; ix < rle; ix++) {
										bits[x++] = quad.bgraBlue;
										bits[x++] = quad.bgraGreen;
										bits[x++] = quad.bgraRed;
										bits[x++] = quad.bgraAlpha;

										if (x >= line) {
											x = 0;
											y++;

											if (y >= header.is_height)
												goto done3210;

											if(fliphoriz)
												bits = Internal_GetScanLine(freeimage, dib, header.is_height-y-1, flipvert);
											else
												bits = Internal_GetScanLine(freeimage, dib, y, flipvert);
										}
									}
								} else {
										rle++;

									for (int ix = 0; ix < rle; ix++) {
										BGRAQUAD quad;

										io.read_proc(&quad, sizeof(BGRAQUAD), 1, handle);

										bits[x++] = quad.bgraBlue;
										bits[x++] = quad.bgraGreen;
										bits[x++] = quad.bgraRed;
										bits[x++] = quad.bgraAlpha;
											
										if (x >= line) {
											x = 0;
											y++;

											if (y >= header.is_height)
												goto done3210;											

											if(fliphoriz)
												bits = Internal_GetScanLine(freeimage, dib, header.is_height-y-1, flipvert);
											else
												bits = Internal_GetScanLine(freeimage, dib, y, flipvert);
										}
									}
								}
							}

					done3210 :
							break;
						}

						default :
							freeimage.free_proc(dib);
							return NULL;
					}

					break;
				}
			}

			return (FIBITMAP *)dib;

		} catch(char *message) {
			freeimage.output_message_proc(s_format_id, message);

			return NULL;
		}
	}

	return NULL;
}

// ==========================================================
//   Init
// ==========================================================

void DLL_CALLCONV
InitTARGA(Plugin &plugin, int format_id) {
	s_format_id = format_id;

	plugin.format_proc = Format;
	plugin.description_proc = Description;
	plugin.extension_proc = Extension;
	plugin.regexpr_proc = RegExpr;
	plugin.load_proc = Load;
}
