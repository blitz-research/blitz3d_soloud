// =====================================================================
// FreeImage Plugin Interface
//
// Design and implementation by
// - Floris van den Berg (flvdberg@wxs.nl)
// - Rui Lopes (ruiglopes@yahoo.com)
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
// ==================	===================================================

#ifdef WIN32
#pragma warning (disable : 4786)
#include <windows.h>
#else
#include <ctype.h>
#endif

#include <io.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <map>

#include "FreeImage.h"
#include "FreeImageIO.h"
#include "Plugin.h"

// =====================================================================

using namespace std;

// =====================================================================
// Plugin search list
// =====================================================================

const char *
s_search_list[] = {
	"",
	"plugins\\",
};

static int s_search_list_size = sizeof(s_search_list) / sizeof(char *);

// =====================================================================
// Reimplementation of stricmp (it is not supported on some systems)
// =====================================================================

int
FreeImage_stricmp(const char *s1, const char *s2) {
	int c1, c2;

	do {
		c1 = tolower(*s1++);
		c2 = tolower(*s2++);
	} while (c1 && c1 == c2);

	return c1 - c2;
}

// =====================================================================
//  Declaration of the FreeImage function pointer structure
// =====================================================================

static FreeImage s_freeimage;

// =====================================================================
//  Plugin Node
// =====================================================================

FI_STRUCT (PluginNode) {
	int m_id;
	void *m_instance;
	Plugin *m_plugin;
	PluginNode *m_next;
	BOOL m_enabled;

	const char *m_format;
	const char *m_description;
	const char *m_extension;
	const char *m_regexpr;
};

// =====================================================================
//  Internal Plugin List
// =====================================================================

class PluginList {
public :
	PluginList();
	~PluginList();

	FREE_IMAGE_FORMAT AddNode(FI_InitProc proc, void *instance = NULL, const char *format = 0, const char *description = 0, const char *extension = 0, const char *regexpr = 0);
	PluginNode *FindNodeFromFormat(const char *format);
	PluginNode *FindNodeFromMime(const char *mime);
	PluginNode *FindNodeFromFIF(int node_id);

	int Size() const;
	BOOL IsEmpty() const;

private :
	map<int, PluginNode *> m_plugin_map;
	int m_node_count;
};

// ---------------------------------------------------------------------

PluginList::PluginList() :
m_plugin_map() {
}

FREE_IMAGE_FORMAT
PluginList::AddNode(FI_InitProc init_proc, void *instance, const char *format, const char *description, const char *extension, const char *regexpr) {
	if (init_proc != NULL) {
		PluginNode *node = new PluginNode;
		Plugin *plugin = new Plugin;
		memset(plugin, 0, sizeof(Plugin));

		// fill-in the plugin structure

		init_proc(*plugin, m_plugin_map.size());

		// get the format string (two possible ways)

		const char *the_format = NULL;

		if (format != NULL) 
			the_format = format;
		else if (plugin->format_proc != NULL)
			the_format = plugin->format_proc();

		// add the node if it wasn't there already

		if (the_format != NULL) {
			if (FindNodeFromFormat(the_format) == NULL) {
				node->m_id = m_plugin_map.size();
				node->m_instance = instance;
				node->m_plugin = plugin;
				node->m_format = format;
				node->m_description = description;
				node->m_extension = extension;
				node->m_regexpr = regexpr;
				node->m_next = NULL;
				node->m_enabled = TRUE;

				m_plugin_map[m_plugin_map.size()] = node;

				return (FREE_IMAGE_FORMAT)node->m_id;
			}
		}

		// something went wrong while allocating the plugin... cleanup

		delete plugin;
		delete node;
	}

	return FIF_UNKNOWN;
}

PluginNode *
PluginList::FindNodeFromFormat(const char *format) {
	int count = 0;

	for (map<int, PluginNode *>::iterator i = m_plugin_map.begin(); i != m_plugin_map.end(); ++i) {
		const char *the_format = ((*i).second->m_format != NULL) ? (*i).second->m_format : (*i).second->m_plugin->format_proc();

		if (strcmp(the_format, format) == 0)
			return (*i).second;		

		count++;
	}

	return NULL;
}

PluginNode *
PluginList::FindNodeFromMime(const char *mime) {
	int count = 0;

	for (map<int, PluginNode *>::iterator i = m_plugin_map.begin(); i != m_plugin_map.end(); ++i) {
		const char *the_mime = ((*i).second->m_plugin->mime_proc != NULL) ? (*i).second->m_plugin->mime_proc() : "";

		if (strcmp(the_mime, mime) == 0)
			return (*i).second;		

		count++;
	}

	return NULL;
}

PluginNode *
PluginList::FindNodeFromFIF(int node_id) {
	map<int, PluginNode *>::iterator i = m_plugin_map.find(node_id);

	if (i != m_plugin_map.end())
		return (*i).second;

	return NULL;
}

int
PluginList::Size() const {
	return m_plugin_map.size();
}

BOOL
PluginList::IsEmpty() const {
	return m_plugin_map.empty();
}

PluginList::~PluginList() {
	for (map<int, PluginNode *>::iterator i = m_plugin_map.begin(); i != m_plugin_map.end(); ++i) {
#ifdef WIN32
		if ((*i).second->m_instance != NULL)
			FreeLibrary((HINSTANCE)(*i).second->m_instance);
#endif
		delete (*i).second->m_plugin; /*CORRECTED*/
		delete ((*i).second);
	}
}

// =====================================================================
// Plugin System Initialization
// =====================================================================

static PluginList *s_plugins = NULL;
static int s_plugin_reference_count = 0;

// ---------------------------------------------------------------------

void DLL_CALLCONV
FreeImage_Initialise(BOOL load_local_plugins_only) {
	if (s_plugin_reference_count++ == 0) {
		// initialize the freeimage function pointer table

		s_freeimage.allocate_proc = FreeImage_Allocate;
		s_freeimage.free_proc = FreeImage_Free;
		s_freeimage.get_bits_proc = FreeImage_GetBits;
		s_freeimage.get_bits_row_col_proc = FreeImage_GetBitsRowCol;
		s_freeimage.get_blue_mask_proc = FreeImage_GetBlueMask;
		s_freeimage.get_bpp_proc = FreeImage_GetBPP;
		s_freeimage.get_color_type_proc = FreeImage_GetColorType;
		s_freeimage.get_colors_used_proc = FreeImage_GetColorsUsed;
		s_freeimage.get_dib_size_proc = FreeImage_GetDIBSize;
		s_freeimage.get_dots_per_meter_x_proc = FreeImage_GetDotsPerMeterX;
		s_freeimage.get_dots_per_meter_y_proc = FreeImage_GetDotsPerMeterY;
		s_freeimage.get_green_mask_proc = FreeImage_GetGreenMask;
		s_freeimage.get_height_proc = FreeImage_GetHeight;
		s_freeimage.get_info_header_proc = FreeImage_GetInfoHeader;
		s_freeimage.get_info_proc = FreeImage_GetInfo;
		s_freeimage.get_line_proc = FreeImage_GetLine;
		s_freeimage.get_palette_proc = FreeImage_GetPalette;
		s_freeimage.get_pitch_proc = FreeImage_GetPitch;
		s_freeimage.get_red_mask_proc = FreeImage_GetRedMask;
		s_freeimage.get_scanline_proc = FreeImage_GetScanLine;
		s_freeimage.get_transparency_count_proc = FreeImage_GetTransparencyCount;
		s_freeimage.get_transparency_table_proc = FreeImage_GetTransparencyTable;
		s_freeimage.get_width_proc = FreeImage_GetWidth;
		s_freeimage.output_message_proc = FreeImage_OutputMessage;
		s_freeimage.set_transparency_table_proc = FreeImage_SetTransparencyTable;
		s_freeimage.is_transparent_proc = FreeImage_IsTransparent;
		s_freeimage.set_transparent_proc = FreeImage_SetTransparent;
		s_freeimage.unload_proc = FreeImage_Unload;
		s_freeimage.convert_line1to8_proc = FreeImage_ConvertLine1To8;
		s_freeimage.convert_line_4to8_proc = FreeImage_ConvertLine4To8;
		s_freeimage.convert_line_16to8_555_proc = FreeImage_ConvertLine16To8_555;
		s_freeimage.convert_line_16to8_565_proc = FreeImage_ConvertLine16To8_565;
		s_freeimage.convert_line_24to8_proc = FreeImage_ConvertLine24To8;
		s_freeimage.convert_line_32to8_proc = FreeImage_ConvertLine32To8;
		s_freeimage.convert_line_1to16_555_proc = FreeImage_ConvertLine1To16_555;
		s_freeimage.convert_line_4to16_555_proc = FreeImage_ConvertLine4To16_555;
		s_freeimage.convert_line_8to16_555_proc = FreeImage_ConvertLine8To16_555;
		s_freeimage.convert_line_16_565_to_16_555_proc = FreeImage_ConvertLine16_565_To16_555;
		s_freeimage.convert_line_24to16_555_proc = FreeImage_ConvertLine24To16_555;
		s_freeimage.convert_line_32to16_555_proc = FreeImage_ConvertLine32To16_555;
		s_freeimage.convert_line_1to16_565_proc = FreeImage_ConvertLine1To16_565;
		s_freeimage.convert_line_4to16_565_proc = FreeImage_ConvertLine4To16_565;
		s_freeimage.convert_line_8to16_565_proc = FreeImage_ConvertLine8To16_565;
		s_freeimage.convert_line_16_555_to_16_565_proc = FreeImage_ConvertLine16_555_To16_565;
		s_freeimage.convert_line_24to16_565_proc = FreeImage_ConvertLine24To16_565;
		s_freeimage.convert_line_32to16_565_proc = FreeImage_ConvertLine32To16_565;
		s_freeimage.convert_line_1to24_proc = FreeImage_ConvertLine1To24;
		s_freeimage.convert_line_4to24_proc = FreeImage_ConvertLine4To24;
		s_freeimage.convert_line_8to24_proc = FreeImage_ConvertLine8To24;
		s_freeimage.convert_line_16to24_555_proc = FreeImage_ConvertLine16To24_555;
		s_freeimage.convert_line_16to24_565_proc = FreeImage_ConvertLine16To24_565;
		s_freeimage.convert_line_32to24_proc = FreeImage_ConvertLine32To24;
		s_freeimage.convert_line_1to32_proc = FreeImage_ConvertLine1To32;
		s_freeimage.convert_line_4to32_proc = FreeImage_ConvertLine4To32;
		s_freeimage.convert_line_8to32_proc = FreeImage_ConvertLine8To32;
		s_freeimage.convert_line_16to32_555_proc = FreeImage_ConvertLine16To32_555;
		s_freeimage.convert_line_16to32_565_proc = FreeImage_ConvertLine16To32_565;
		s_freeimage.convert_line_24to32_proc = FreeImage_ConvertLine24To32;

		// internal plugin initialization

		s_plugins = new PluginList;

		s_plugins->AddNode(InitBMP);
//		s_plugins->AddNode(InitICO);
		s_plugins->AddNode(InitJPEG);
//		s_plugins->AddNode(InitMNG, NULL, "JNG", "JPEG Network Graphics", "jng", "");
//		s_plugins->AddNode(InitKOALA);
		s_plugins->AddNode(InitIFF);
//		s_plugins->AddNode(InitMNG);
//		s_plugins->AddNode(InitPNM, NULL, "PBM", "Portable Bitmap (ASCII)", "pbm", "^P1");
//		s_plugins->AddNode(InitPNM, NULL, "PBMRAW", "Portable Bitmap (RAW)", "pbm", "^P4");
//		s_plugins->AddNode(InitPCD);
		s_plugins->AddNode(InitPCX);
//		s_plugins->AddNode(InitPNM, NULL, "PGM", "Portable Greymap (ASCII)", "pgm", "^P2");
//		s_plugins->AddNode(InitPNM, NULL, "PGMRAW", "Portable Greymap (RAW)", "pgm", "^P5");
		s_plugins->AddNode(InitPNG);
//		s_plugins->AddNode(InitPNM, NULL, "PPM", "Portable Pixelmap (ASCII)", "ppm", "^P3");
//		s_plugins->AddNode(InitPNM, NULL, "PPMRAW", "Portable Pixelmap (RAW)", "ppm", "^P6");
//		s_plugins->AddNode(InitRAS);
		s_plugins->AddNode(InitTARGA);
//		s_plugins->AddNode(InitTIFF);
//		s_plugins->AddNode(InitWBMP);
//		s_plugins->AddNode(InitPSD);

		// external plugin initialization

#ifdef WIN32
		if (!load_local_plugins_only) {
			int count = 0;
			char buffer[MAX_PATH + 200];

			while (count < s_search_list_size) {
				_finddata_t find_data;
				long find_handle;

				strcpy(buffer, s_search_list[count]);
				strcat(buffer, "*.fip");

				if ((find_handle= _findfirst(buffer, &find_data)) != -1L) {
					do {
						strcpy(buffer, s_search_list[count]);
						strncat(buffer, find_data.name, MAX_PATH + 200);

						HINSTANCE instance = LoadLibrary(buffer);

						if (instance != NULL) {
							FARPROC proc_address = GetProcAddress(instance, "_Init@8");

							if (proc_address != NULL)
								s_plugins->AddNode((FI_InitProc)proc_address, (void *)instance);
							else
								FreeLibrary(instance);
						}
					} while (_findnext(find_handle, &find_data) != -1L);

					_findclose(find_handle);
				}

				count++;
			}
		}
#endif
	}
}

void DLL_CALLCONV
FreeImage_DeInitialise() {
	--s_plugin_reference_count;

	if (s_plugin_reference_count == 0)
		delete s_plugins;
}

// =====================================================================
// Plugin System Load/Save Functions
// =====================================================================

FIBITMAP * DLL_CALLCONV
FreeImage_LoadFromHandle(FREE_IMAGE_FORMAT fif, FreeImageIO *io, fi_handle handle, int flags) {
	if ((fif >= 0) && (fif < FreeImage_GetFIFCount())) {
		PluginNode *node = s_plugins->FindNodeFromFIF(fif);

		if (node != NULL) {
			if (node->m_enabled) {
				FIBITMAP *bitmap = NULL;

				if (node->m_plugin->open_proc != NULL) {
					void *data = node->m_plugin->open_proc(*io, handle, TRUE);

					bitmap = node->m_plugin->load_proc(s_freeimage, *io, handle, -1, flags, data);

					if (node->m_plugin->close_proc != NULL)
						node->m_plugin->close_proc(*io, handle, data);

					return bitmap;
				} else {
					return node->m_plugin->load_proc(s_freeimage, *io, handle, -1, flags, NULL);
				}
			}
		}
	}

	return NULL;
}

FIBITMAP * DLL_CALLCONV
FreeImage_Load(FREE_IMAGE_FORMAT fif, const char *filename, int flags) {
	FreeImageIO io;
	SetDefaultIO(&io);
	
	FILE *handle = fopen(filename, "rb");

	if (handle) {
		FIBITMAP *bitmap = FreeImage_LoadFromHandle(fif, &io, (fi_handle)handle, flags);

		fclose(handle);

		return bitmap;
	}

	return NULL;
}

BOOL DLL_CALLCONV
FreeImage_SaveToHandle(FREE_IMAGE_FORMAT fif, FIBITMAP *dib, FreeImageIO *io, fi_handle handle, int flags) {
	if ((fif >= 0) && (fif < FreeImage_GetFIFCount())) {
		PluginNode *node = s_plugins->FindNodeFromFIF(fif);

		if (node != NULL) {
			if (node->m_enabled) {
				BOOL result = FALSE;

				if (node->m_plugin->open_proc != NULL) {
					void *data = node->m_plugin->open_proc(*io, handle, FALSE);

					result = node->m_plugin->save_proc(s_freeimage, *io, dib, handle, -1, flags, data);

					if (node->m_plugin->close_proc != NULL)
						node->m_plugin->close_proc(*io, handle, data);				
				} else {
					result = node->m_plugin->save_proc(s_freeimage, *io, dib, handle, -1, flags, NULL);
				}

				return result;
			}
		}
	}

	return NULL;
}


BOOL DLL_CALLCONV
FreeImage_Save(FREE_IMAGE_FORMAT fif, FIBITMAP *dib, const char *filename, int flags) {
	FreeImageIO io;
	SetDefaultIO(&io);
	
	FILE *handle = fopen(filename, "wb");
	
	if (handle) {
		BOOL success = FreeImage_SaveToHandle(fif, dib, &io, (fi_handle)handle, flags);

		fclose(handle);

		return success;
	}

	return FALSE;
}

// =====================================================================
// Plugin Creation / Enabling Functions
// =====================================================================

FREE_IMAGE_FORMAT DLL_CALLCONV
FreeImage_RegisterLocalPlugin(FI_InitProc proc_address, const char *format, const char *description, const char *extension, const char *regexpr) {
	return s_plugins->AddNode(proc_address, NULL, format, description, extension, regexpr);
}

FREE_IMAGE_FORMAT DLL_CALLCONV
FreeImage_RegisterExternalPlugin(const char *path, const char *format, const char *description, const char *extension, const char *regexpr) {
	if (path != NULL) {
		HINSTANCE instance = LoadLibrary(path);

		if (instance != NULL) {
			FARPROC proc_address = GetProcAddress(instance, "_Init@8");

			FREE_IMAGE_FORMAT result = s_plugins->AddNode((FI_InitProc)proc_address, (void *)instance, format, description, extension, regexpr);

			if (result == FIF_UNKNOWN)
				FreeLibrary(instance);

			return result;
		}
	}

	return FIF_UNKNOWN;
}

int DLL_CALLCONV
FreeImage_SetPluginEnabled(FREE_IMAGE_FORMAT fif, BOOL enable) {
	if (s_plugins != NULL) {
		PluginNode *node = s_plugins->FindNodeFromFIF(fif);

		if (node != NULL) {
			BOOL previous_state = node->m_enabled;

			node->m_enabled = enable;

			return previous_state;
		}
	}

	return -1;
}

int DLL_CALLCONV
FreeImage_IsPluginEnabled(FREE_IMAGE_FORMAT fif) {
	if (s_plugins != NULL) {
		PluginNode *node = s_plugins->FindNodeFromFIF(fif);

		return (node != NULL) ? node->m_enabled : FALSE;
	}
	
	return -1;
}

// =====================================================================
// Plugin Access Functions
// =====================================================================

int DLL_CALLCONV
FreeImage_GetFIFCount() {
	return (s_plugins != NULL) ? s_plugins->Size() : 0;
}

FREE_IMAGE_FORMAT DLL_CALLCONV
FreeImage_GetFIFFromFormat(const char *format) {
	if (s_plugins != NULL) {
		PluginNode *node = s_plugins->FindNodeFromFormat(format);

		return (node != NULL) ? (node->m_enabled) ? (FREE_IMAGE_FORMAT)node->m_id : FIF_UNKNOWN : FIF_UNKNOWN;
	}

	return FIF_UNKNOWN;
}

FREE_IMAGE_FORMAT DLL_CALLCONV
FreeImage_GetFIFFromMime(const char *mime) {
	if (s_plugins != NULL) {
		PluginNode *node = s_plugins->FindNodeFromMime(mime);

		return (node != NULL) ? (node->m_enabled) ? (FREE_IMAGE_FORMAT)node->m_id : FIF_UNKNOWN : FIF_UNKNOWN;
	}

	return FIF_UNKNOWN;
}

const char * DLL_CALLCONV
FreeImage_GetFormatFromFIF(FREE_IMAGE_FORMAT fif) {
	if (s_plugins != NULL) {
		PluginNode *node = s_plugins->FindNodeFromFIF(fif);

		return (node != NULL) ? (node->m_format != NULL) ? node->m_format : node->m_plugin->format_proc() : NULL;
	}

	return NULL;
}

const char * DLL_CALLCONV
FreeImage_GetFIFExtensionList(FREE_IMAGE_FORMAT fif) {
	if (s_plugins != NULL) {
		PluginNode *node = s_plugins->FindNodeFromFIF(fif);

		return (node != NULL) ? (node->m_extension != NULL) ? node->m_extension : (node->m_plugin->extension_proc != NULL) ? node->m_plugin->extension_proc() : NULL : NULL;
	}

	return NULL;
}

const char * DLL_CALLCONV
FreeImage_GetFIFDescription(FREE_IMAGE_FORMAT fif) {
	if (s_plugins != NULL) {
		PluginNode *node = s_plugins->FindNodeFromFIF(fif);

		return (node != NULL) ? (node->m_description != NULL) ? node->m_description : (node->m_plugin->description_proc != NULL) ? node->m_plugin->description_proc() : NULL : NULL;
	}

	return NULL;
}

const char * DLL_CALLCONV
FreeImage_GetFIFRegExpr(FREE_IMAGE_FORMAT fif) {
	if (s_plugins != NULL) {
		PluginNode *node = s_plugins->FindNodeFromFIF(fif);

		return (node != NULL) ? (node->m_regexpr != NULL) ? node->m_regexpr : (node->m_plugin->regexpr_proc != NULL) ? node->m_plugin->regexpr_proc() : NULL : NULL;
	}

	return NULL;
}

BOOL DLL_CALLCONV
FreeImage_FIFSupportsReading(FREE_IMAGE_FORMAT fif) {
	if (s_plugins != NULL) {
		PluginNode *node = s_plugins->FindNodeFromFIF(fif);

		return (node != NULL) ? (node->m_enabled) ? node->m_plugin->load_proc != NULL : FALSE : FALSE;
	}

	return FALSE;
}

BOOL DLL_CALLCONV
FreeImage_FIFSupportsWriting(FREE_IMAGE_FORMAT fif) {
	if (s_plugins != NULL) {
		PluginNode *node = s_plugins->FindNodeFromFIF(fif);

		return (node != NULL) ? (node->m_enabled) ? node->m_plugin->save_proc != NULL : FALSE : FALSE;
	}

	return FALSE;
}

FREE_IMAGE_FORMAT DLL_CALLCONV
FreeImage_GetFIFFromFilename(const char *filename) {
	if (filename != NULL) {
		const char *extension;

		// get the proper extension if we received a filename

		char *place = strrchr((char *)filename, '.');

		if (place != NULL)
			extension = ++place;
		else
			extension = filename;

		// look for the extension in the plugin table

		for (int i = 0; i < FreeImage_GetFIFCount(); ++i) {
			// compare the format id with the extension

			if (FreeImage_stricmp(FreeImage_GetFormatFromFIF((FREE_IMAGE_FORMAT)i), extension) == 0) {
				if (s_plugins->FindNodeFromFIF(i)->m_enabled) {
					return (FREE_IMAGE_FORMAT)i;
				} else {
					return FIF_UNKNOWN;
				}
			} else {
				// make a copy of the extension list and split it

				char *copy = (char *)malloc(strlen(FreeImage_GetFIFExtensionList((FREE_IMAGE_FORMAT)i)) + 1);
				memset(copy, 0, strlen(FreeImage_GetFIFExtensionList((FREE_IMAGE_FORMAT)i)) + 1);
				memcpy(copy, FreeImage_GetFIFExtensionList((FREE_IMAGE_FORMAT)i), strlen(FreeImage_GetFIFExtensionList((FREE_IMAGE_FORMAT)i)));

				// get the first token

				char *token = strtok(copy, ",");

				while (token != NULL) {
					if (FreeImage_stricmp(token, extension) == 0) {
						free(copy);

						if (s_plugins->FindNodeFromFIF(i)->m_enabled) {							
							return (FREE_IMAGE_FORMAT)i;
						} else {
							return FIF_UNKNOWN;
						}
					}

					token = strtok(NULL, ",");
				}

				// free the copy of the extension list

				free(copy);
			}	
		}
	}

	return FIF_UNKNOWN;
}

BOOL DLL_CALLCONV
FreeImage_Validate(FREE_IMAGE_FORMAT fif, FreeImageIO &io, fi_handle handle) {
	if (s_plugins != NULL) {
		BOOL validated = FALSE;

		PluginNode *node = s_plugins->FindNodeFromFIF(fif);

		if (node) {
			long tell = io.tell_proc(handle);

			validated = (node != NULL) ? (node->m_enabled) ? (node->m_plugin->validate_proc != NULL) ? node->m_plugin->validate_proc(io, handle) : FALSE : FALSE : FALSE;

			io.seek_proc(handle, tell, SEEK_SET);
		}

		return validated;
	}

	return FALSE;
}
