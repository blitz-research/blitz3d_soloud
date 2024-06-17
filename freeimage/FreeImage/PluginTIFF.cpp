// ==========================================================
// TIFF Loader and Writer
//
// Design and implementation by 
// - Floris van den Berg (flvdberg@wxs.nl)
// - Hervé Drolon (drolon@iut.univ-lehavre.fr)
// - Markus Loibl (markus.loibl@epost.de)
// - Luca Piergentili (l.pierge@terra.es)
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

#ifdef unix
#undef unix
#endif
#ifdef __unix
#undef __unix
#endif

#include "../LibTIFF/tiffiop.h"

#include "FreeImage.h"
#include "Utilities.h"

// ----------------------------------------------------------

static FreeImageIO *g_io = NULL;

// ----------------------------------------------------------
//   libtiff interface 
// ----------------------------------------------------------

static tsize_t 
_tiffReadProc(thandle_t file, tdata_t buf, tsize_t size) {
	return g_io->read_proc(buf, size, 1, (fi_handle)file) * size;
}

static tsize_t
_tiffWriteProc(thandle_t file, tdata_t buf, tsize_t size) {
	return g_io->write_proc(buf, size, 1, (fi_handle)file) * size;
}

static toff_t
_tiffSeekProc(thandle_t file, toff_t off, int whence) {
	g_io->seek_proc((fi_handle)file, off, whence);
	return g_io->tell_proc((fi_handle)file);
}

static int
_tiffCloseProc(thandle_t fd) {
	return 0;
}

#include <sys/stat.h>

static toff_t
_tiffSizeProc(thandle_t file) {
	struct stat sb;
	return (fstat((int) file, &sb) < 0 ? 0 : sb.st_size);
}

static int
_tiffMapProc(thandle_t fd, tdata_t* pbase, toff_t* psize) {
	return 0;
}

static void
_tiffUnmapProc(thandle_t fd, tdata_t base, toff_t size) {
}

// ----------------------------------------------------------
//   Open a TIFF file descriptor for read/writing.
// ----------------------------------------------------------

TIFF *
TIFFFdOpen(thandle_t handle, const char *name, const char *mode) {
	TIFF *tif;

	tif = TIFFClientOpen(name, mode, handle,
	    _tiffReadProc, _tiffWriteProc, _tiffSeekProc, _tiffCloseProc,
	    _tiffSizeProc, _tiffMapProc, _tiffUnmapProc);

	if (tif)
		tif->tif_fd = (int)handle;

	return tif;
}

// ----------------------------------------------------------
//   Open a TIFF file for read/writing.
// ----------------------------------------------------------

TIFF*
TIFFOpen(const char* name, const char* mode) {
	return 0;
}

tdata_t
_TIFFmalloc(tsize_t s) {
	return malloc(s);
}

void
_TIFFfree(tdata_t p) {
	free(p);
}

tdata_t
_TIFFrealloc(tdata_t p, tsize_t s) {
	return realloc(p, s);
}

void
_TIFFmemset(tdata_t p, int v, tsize_t c) {
	memset(p, v, (size_t) c);
}

void
_TIFFmemcpy(tdata_t d, const tdata_t s, tsize_t c) {
	memcpy(d, s, (size_t) c);
}

int
_TIFFmemcmp(const tdata_t p1, const tdata_t p2, tsize_t c) {
	return (memcmp(p1, p2, (size_t) c));
}

// ----------------------------------------------------------
//   in FreeImage warnings and errors are disabled
// ----------------------------------------------------------

static void
msdosWarningHandler(const char* module, const char* fmt, va_list ap) {
}

TIFFErrorHandler _TIFFwarningHandler = msdosWarningHandler;

static void
msdosErrorHandler(const char* module, const char* fmt, va_list ap) {
}

TIFFErrorHandler _TIFFerrorHandler = msdosErrorHandler;

// ----------------------------------------------------------

#define CVT(x)      (((x) * 255L) / ((1L<<16)-1))
#define	SCALE(x)	(((x)*((1L<<16)-1))/255)

// ==========================================================
// Internal functions
// ==========================================================

static uint16
CheckColormap(int n, uint16* r, uint16* g, uint16* b) {
    while (n-- > 0) {
        if (*r++ >= 256 || *g++ >= 256 || *b++ >= 256) {
			return 16;
		}
	}

    return 8;
}

static uint16
CheckPhotometric(FreeImage &freeimage, FIBITMAP *dib, uint16 bitspersample) {
	RGBQUAD *rgb;
	uint16 i;

	switch(bitspersample) {
		case 1:
		{
			rgb = freeimage.get_palette_proc(dib);

			if ((rgb->rgbRed == 0) && (rgb->rgbGreen == 0) && (rgb->rgbBlue == 0)) {
				rgb++;

				if ((rgb->rgbRed == 255) && (rgb->rgbGreen == 255) && (rgb->rgbBlue == 255))
					return PHOTOMETRIC_MINISBLACK;				
			}

			if ((rgb->rgbRed == 255) && (rgb->rgbGreen == 255) && (rgb->rgbBlue == 255)) {
				rgb++;

				if ((rgb->rgbRed == 0) && (rgb->rgbGreen == 0) && (rgb->rgbBlue == 0))
					return PHOTOMETRIC_MINISWHITE;				
			}

			return PHOTOMETRIC_PALETTE;
		}

		case 4:	// Check if the DIB has a color or a greyscale palette
		case 8:
			rgb = freeimage.get_palette_proc(dib);

			for (i = 0; i < freeimage.get_colors_used_proc(dib); i++) {
				if ((rgb->rgbRed != rgb->rgbGreen) || (rgb->rgbRed != rgb->rgbBlue))
					return PHOTOMETRIC_PALETTE;
				
				// The DIB has a color palette if the greyscale isn't a linear ramp

				if (rgb->rgbRed != i)
					return PHOTOMETRIC_PALETTE;				

				rgb++;
			}

			return PHOTOMETRIC_MINISBLACK;
			
		case 24:
			return PHOTOMETRIC_RGB;			
	}

	return PHOTOMETRIC_MINISBLACK;
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
	return "TIFF";
}

static const char * DLL_CALLCONV
Description() {
	return "Tagged Image File Format";
}

static const char * DLL_CALLCONV
Extension() {
	return "tif,tiff";
}

static const char * DLL_CALLCONV
RegExpr() {
	return "^[MI][MI][\\x01*][\\x01*]";
}

static const char * DLL_CALLCONV
MimeType() {
	return "image/tiff";
}

static BOOL DLL_CALLCONV
Validate(FreeImageIO &io, fi_handle handle) {	
	BYTE tiff_id1[] = { 0x49, 0x49 };
	BYTE tiff_id2[] = { 0x4D, 0x4D };
	BYTE signature[2] = { 0, 0 };

	io.read_proc(signature, 1, sizeof(tiff_id1), handle);

	if (memcmp(tiff_id1, signature, sizeof(tiff_id1)) == 0)
		return TRUE;

	if (memcmp(tiff_id2, signature, sizeof(tiff_id1)) == 0)
		return TRUE;

	return FALSE;
}

// ----------------------------------------------------------

static void * DLL_CALLCONV
Open(FreeImageIO &io, fi_handle handle, BOOL read) {
	g_io = &io;

	if (read)
		return TIFFFdOpen((thandle_t)handle, "", "r");
	else
		return TIFFFdOpen((thandle_t)handle, "", "w");
}

static void DLL_CALLCONV
Close(FreeImageIO &io, fi_handle handle, void *data) {
	TIFFClose((TIFF *)data);

	g_io = NULL;
}

// ----------------------------------------------------------

static int DLL_CALLCONV
PageCount(FreeImageIO &io, fi_handle handle, void *data) {
	int nr_ifd = 0;

	TIFF *tif = (TIFF *)data;

	do {
		nr_ifd++;
	} while (TIFFReadDirectory(tif));
			
	return nr_ifd;
}

// ----------------------------------------------------------

static FIBITMAP * DLL_CALLCONV
Load(FreeImage &freeimage, FreeImageIO &io, fi_handle handle, int page, int flags, void *data) {
	if ((handle != NULL) && (data != NULL)) {
		TIFF   *tif;
		uint32 height; 
		uint32 width; 
		uint16 bitspersample;
		uint16 samplesperpixel;
		uint32 rowsperstrip;  
		uint16 photometric;
		uint16 compression;
		uint32 x, y;
		BOOL isRGB;

		int32 nrow;
		BYTE *buf;          
		FIBITMAP *dib;
		BYTE *bits;		// pointer to dib data
		RGBQUAD *pal;	// pointer to dib palette
    
		if (handle) {
			try {			
				tif = (TIFF *)data;
    
				if (page != -1)
					if (!tif || !TIFFSetDirectory(tif, page))
						throw "Error encountered while opening TIFF file";			

				TIFFGetField(tif, TIFFTAG_COMPRESSION, &compression);
				TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
				TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
				TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel);
				TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitspersample);
				TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP, &rowsperstrip);   
				TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &photometric);

				if (compression == COMPRESSION_LZW)
					throw "LZW compression is no longer supported due to Unisys patent enforcement";			

				// Convert to 24 or 32 bits RGB if the image is full color

				isRGB = (bitspersample >= 8) &&
					(photometric == PHOTOMETRIC_RGB) ||
					(photometric == PHOTOMETRIC_YCBCR) ||
					(photometric == PHOTOMETRIC_SEPARATED) ||
					(photometric == PHOTOMETRIC_LOGLUV);

				if (isRGB) {
					// Read the whole image into one big RGBA buffer and then 
					// convert it to a DIB. This is using the traditional
					// TIFFReadRGBAImage() API that we trust.

					uint32* raster;		// retrieve RGBA image
					uint32 *row;

					raster = (uint32*)_TIFFmalloc(width * height * sizeof (uint32));

					if (raster == NULL) {
						throw "No space for raster buffer";
					}
					
					// Read the image in one chunk into an RGBA array

					if(!TIFFReadRGBAImage(tif, width, height, raster, 0)) {
						_TIFFfree(raster);
						return NULL;
					}

					// create a new DIB

					if (samplesperpixel == 4)
						dib = freeimage.allocate_proc(width, height, 32, 0xFF, 0xFF00, 0xFF0000);
					else
						dib = freeimage.allocate_proc(width, height, 24, 0xFF, 0xFF00, 0xFF0000);

					if (dib == NULL) {
						_TIFFfree(raster);

						throw "DIB allocation failed";
					}
					
					// fill in the metrics (english or universal)

					float	fResX, fResY;
					uint16	resUnit;
					TIFFGetField(tif, TIFFTAG_RESOLUTIONUNIT, &resUnit);
					TIFFGetField(tif, TIFFTAG_XRESOLUTION, &fResX);
					TIFFGetField(tif, TIFFTAG_YRESOLUTION, &fResY);

					BITMAPINFOHEADER *pInfoHeader = freeimage.get_info_header_proc(dib);

					if (resUnit == RESUNIT_INCH) {
						pInfoHeader->biXPelsPerMeter = (int) (fResX/0.0254000 + 0.5);
						pInfoHeader->biYPelsPerMeter = (int) (fResY/0.0254000 + 0.5);
					} else if(resUnit == RESUNIT_CENTIMETER) {
						pInfoHeader->biXPelsPerMeter = (int) (fResX*100.0 + 0.5);
						pInfoHeader->biYPelsPerMeter = (int) (fResY*100.0 + 0.5);
					}

					// read the raster lines and save them in the DIB
					// with RGB mode, we have to change the order of the 3 samples RGB
					// We use macros for extracting components from the packed ABGR 
					// form returned by TIFFReadRGBAImage.

					BOOL has_alpha = FALSE;
					row = &raster[0];

					for (y = 0; y < height; y++) {
						bits = freeimage.get_scanline_proc(dib, y);

						for (x = 0; x < width; x++) {
							bits[0] = (BYTE)TIFFGetB(row[x]);
							bits[1] = (BYTE)TIFFGetG(row[x]);
							bits[2] = (BYTE)TIFFGetR(row[x]);

							if (samplesperpixel == 4) {
								bits[3] = (BYTE)TIFFGetA(row[x]);

								if (bits[3] != 0) {
									has_alpha = TRUE;
								}

								bits += 4;
							} else {
								bits += 3;
							}							
						}

						row += width;
					}

					freeimage.set_transparent_proc(dib, has_alpha);

					_TIFFfree(raster);

					return (FIBITMAP *)dib;
				} else {
					// calculate the line + pitch

					int line = CalculateLine(width, bitspersample * samplesperpixel);
					int pitch = CalculatePitch(line);

					// create a new DIB

					dib = freeimage.allocate_proc(width, height, bitspersample * samplesperpixel, 0, 0, 0);

					if (dib == NULL) {
						throw "No space for DIB image";
					}

					float fResX, fResY;
					uint16 resUnit;
					TIFFGetField(tif, TIFFTAG_RESOLUTIONUNIT, &resUnit);
					TIFFGetField(tif, TIFFTAG_XRESOLUTION, &fResX);
					TIFFGetField(tif, TIFFTAG_YRESOLUTION, &fResY);

					BITMAPINFOHEADER *pInfoHeader = freeimage.get_info_header_proc(dib);

					if (resUnit == RESUNIT_INCH) {
						/* english */

						pInfoHeader->biXPelsPerMeter = (int) (fResX/0.0254000 + 0.5);
						pInfoHeader->biYPelsPerMeter = (int) (fResY/0.0254000 + 0.5);
					} else if (resUnit== RESUNIT_CENTIMETER) {
						/* metric */

						pInfoHeader->biXPelsPerMeter = (int) (fResX*100.0 + 0.5);
						pInfoHeader->biYPelsPerMeter = (int) (fResY*100.0 + 0.5);
					}

					// In the tiff file the lines are save from up to down 
					// In a DIB the lines must be save from down to up

					bits = freeimage.get_bits_proc(dib) + height * pitch;

					// now lpBits pointe on the bottom line

					// set up the colormap based on photometric	

					pal = freeimage.get_palette_proc(dib);

					switch(photometric) {
						case PHOTOMETRIC_MINISBLACK:	// bitmap and greyscale image types
						case PHOTOMETRIC_MINISWHITE:
							// Monochrome image

							if (bitspersample == 1) {
								if (photometric == PHOTOMETRIC_MINISBLACK) {
									pal[0].rgbRed = 0;
									pal[0].rgbGreen = 0;
									pal[0].rgbBlue = 0;
									pal[1].rgbRed = 255;
									pal[1].rgbGreen = 255;
									pal[1].rgbBlue = 255;
								} else {
									pal[0].rgbRed = 255;
									pal[0].rgbGreen = 255;
									pal[0].rgbBlue = 255;
									pal[1].rgbRed = 0;
									pal[1].rgbGreen = 0;
									pal[1].rgbBlue = 0;
								}
							} else {
								// need to build the scale for greyscale images

								if (photometric == PHOTOMETRIC_MINISBLACK) {
									for (int i = 0; i < 256; i++) {
										pal[i].rgbRed	=
										pal[i].rgbGreen =
										pal[i].rgbBlue	= i;
									}
								} else {
									for (int i = 0; i < 256; i++) {
										pal[i].rgbRed	=
										pal[i].rgbGreen =
										pal[i].rgbBlue	= 255 - i;
									}
								}
							}

							break;

						case PHOTOMETRIC_PALETTE:	// color map indexed
							uint16 *red;
							uint16 *green;
							uint16 *blue;
							BOOL Palette16Bits;
							
							TIFFGetField(tif, TIFFTAG_COLORMAP, &red, &green, &blue); 

							// Is the palette 16 or 8 bits ?

							if (CheckColormap(1<<bitspersample, red, green, blue) == 16)  {
								Palette16Bits = TRUE;
							} else {
								Palette16Bits = FALSE;
							}

							// load the palette in the DIB

							for (int i = (1 << bitspersample) - 1; i >= 0; i--) {
								if (Palette16Bits) {
									pal[i].rgbRed =(BYTE) CVT(red[i]);
									pal[i].rgbGreen = (BYTE) CVT(green[i]);
									pal[i].rgbBlue = (BYTE) CVT(blue[i]);           
								} else {
									pal[i].rgbRed = (BYTE) red[i];
									pal[i].rgbGreen = (BYTE) green[i];
									pal[i].rgbBlue = (BYTE) blue[i];        
								}
							}

							break;
						
					}

					// read the tiff lines and save them in the DIB

					buf = new BYTE[TIFFStripSize(tif)];

					for (y = 0; y < height; y += rowsperstrip) {
						nrow = (y + rowsperstrip > height ? height - y : rowsperstrip);

						if (TIFFReadEncodedStrip(tif, TIFFComputeStrip(tif, y, 0), buf, nrow * line) == -1) {
							delete [] buf;

							return NULL;
						} else {
							for (int l = 0; l < nrow; l++) {
								bits -= pitch;

								memcpy(bits, buf + l * line, line);
							}
						}
					}
					
					delete [] buf;

					return (FIBITMAP *)dib;

				}
			} catch (char *message) {
				freeimage.output_message_proc(s_format_id, message);

				return NULL;
			}
		}
	}

	return NULL;	   
}

static BOOL DLL_CALLCONV
Save(FreeImage &freeimage, FreeImageIO &io, FIBITMAP *dib, fi_handle handle, int page, int flags, void *data) {
	if (dib != NULL) {
		TIFF *out = (TIFF *)data;

		uint32 height;
		uint32 width;
		uint32 rowsperstrip = (uint32) -1;
		uint16 bitspersample;
		uint16 samplesperpixel;
		uint16 photometric;
		uint16 compression;
		uint16 pitch;

		uint32 x, y;

		width = freeimage.get_width_proc(dib);
		height = freeimage.get_height_proc(dib);
		bitspersample = freeimage.get_bpp_proc(dib);
		samplesperpixel = ((bitspersample == 24) || (bitspersample == 32)) ? 3 : 1;
		photometric = CheckPhotometric(freeimage, dib, (bitspersample == 32) ? 24 : bitspersample);

		// don't accept ALPHA at this moment

		if ((bitspersample == 32) && (freeimage.is_transparent_proc(dib)))
			throw "alpha channels are currently unsupported";

		// handle standard width/height/bpp stuff

		TIFFSetField(out, TIFFTAG_IMAGEWIDTH, width);
		TIFFSetField(out, TIFFTAG_IMAGELENGTH, height);
		TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, samplesperpixel);
		TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, ((bitspersample == 32) ? 24 : bitspersample) / samplesperpixel);
		TIFFSetField(out, TIFFTAG_PHOTOMETRIC, photometric);
		TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);	// single image plane 
		TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
		TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(out, rowsperstrip));

		// handle metrics

		BITMAPINFOHEADER *pInfoHeader = freeimage.get_info_header_proc(dib);
		TIFFSetField(out, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
		TIFFSetField(out, TIFFTAG_XRESOLUTION, 0.0254*((float)pInfoHeader->biXPelsPerMeter));
		TIFFSetField(out, TIFFTAG_YRESOLUTION, 0.0254*((float)pInfoHeader->biYPelsPerMeter));

		// multi-paging

		if (page >= 0) {
			char page_number[20];
			sprintf(page_number, "Page %d", page);

			TIFFSetField(out, TIFFTAG_SUBFILETYPE, FILETYPE_PAGE);
			TIFFSetField(out, TIFFTAG_PAGENUMBER, page);
			TIFFSetField(out, TIFFTAG_PAGENAME, page_number);
		} else {
			TIFFSetField(out, TIFFTAG_SUBFILETYPE, 0);
		}

		// palettes (image colormaps are automatically scaled to 16-bits)

		if (photometric == PHOTOMETRIC_PALETTE) {
			uint16 *r, *g, *b;
			uint16 nColors = freeimage.get_colors_used_proc(dib);
			RGBQUAD *pal = freeimage.get_palette_proc(dib);

			r = (uint16 *) _TIFFmalloc(sizeof(uint16) * 3 * nColors);
			g = r + nColors;
			b = g + nColors;

			for (int i = nColors - 1; i >= 0; i--) {
				r[i] = SCALE((uint16)pal[i].rgbRed);
				g[i] = SCALE((uint16)pal[i].rgbGreen);
				b[i] = SCALE((uint16)pal[i].rgbBlue);
			}

			TIFFSetField(out, TIFFTAG_COLORMAP, r, g, b);

			_TIFFfree(r);
		}

		// compression

		switch(bitspersample) {
			case 1 :
				compression = COMPRESSION_CCITTFAX4;
				break;

			case 8 :
			case 24 :
			case 32 :
				compression = COMPRESSION_PACKBITS;
				break;

			default :
				compression = COMPRESSION_NONE;
				break;
		}

		TIFFSetField(out, TIFFTAG_COMPRESSION, compression);

		// read the DIB lines from bottom to top
		// and save them in the TIF
		// -------------------------------------
		
		pitch = freeimage.get_pitch_proc(dib);
		
		switch(bitspersample) {				
			case 1 :
			case 4 :
			case 8 :
			{
				for (y = 0; y < height; y++) {
					BYTE *bits = freeimage.get_bits_row_col_proc(dib, 0, y);

					TIFFWriteScanline(out,bits, y, 0);
				}

				break;
			}				

			case 24:
			case 32 :
			{
				BYTE *buffer = (BYTE *)malloc(CalculatePitch(CalculateLine(width, 24)));

				for (y = 0; y < height; y++) {
					// get a pointer to the (converted) scanline

					if (bitspersample == 32)
						freeimage.convert_line_32to24_proc(buffer, freeimage.get_scanline_proc(dib, height - y - 1), width);
					else
						memcpy(buffer, freeimage.get_scanline_proc(dib, height - y - 1), pitch);

					// TIFFs store color data RGB instead of BGR

					BYTE *pBuf = buffer;

					for (x = 0; x < width; x++) {
						BYTE tmp = pBuf[0];
						pBuf[0] = pBuf[2];
						pBuf[2] = tmp;

						pBuf += 3;
					}

					// write the scanline to disc

					TIFFWriteScanline(out, buffer, y, 0);
				}

				free(buffer);

				break;
			}				
		}

		return TRUE;
	}

	return FALSE;
}

// ==========================================================
//   Init
// ==========================================================

void DLL_CALLCONV
InitTIFF(Plugin &plugin, int format_id) {
	s_format_id = format_id;

	plugin.format_proc = Format;
	plugin.description_proc = Description;
	plugin.extension_proc = Extension;
	plugin.regexpr_proc = RegExpr;
	plugin.open_proc = Open;
	plugin.close_proc = Close;
	plugin.pagecount_proc = PageCount;
	plugin.load_proc = Load;
	plugin.save_proc = Save;
	plugin.validate_proc = Validate;
	plugin.mime_proc = MimeType;
}
