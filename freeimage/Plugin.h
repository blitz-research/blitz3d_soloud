// ==========================================================
// Internal plugins
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

#ifndef PLUGIN_H
#define PLUGIN_H

#ifndef FREEIMAGE_H
#include "FreeImage.h"
#endif

// ==========================================================

struct Plugin;

// ==========================================================
//   Plugin Initialisation Callback
// ==========================================================

void DLL_CALLCONV FreeImage_OutputMessage(int fif, const char *message);

// ==========================================================
//   Plugin validation
// ==========================================================

extern "C" { BOOL DLL_CALLCONV FreeImage_Validate(FREE_IMAGE_FORMAT fif, FreeImageIO &io, fi_handle handle); }

// ==========================================================
//   Internal plugins
// ==========================================================

void DLL_CALLCONV InitBMP(Plugin &plugin, int format_id);
void DLL_CALLCONV InitICO(Plugin &plugin, int format_id);
void DLL_CALLCONV InitIFF(Plugin &plugin, int format_id);
void DLL_CALLCONV InitJPEG(Plugin &plugin, int format_id);
void DLL_CALLCONV InitKOALA(Plugin &plugin, int format_id);
void DLL_CALLCONV InitLBM(Plugin &plugin, int format_id);
void DLL_CALLCONV InitMNG(Plugin &plugin, int format_id);
void DLL_CALLCONV InitPCD(Plugin &plugin, int format_id);
void DLL_CALLCONV InitPCX(Plugin &plugin, int format_id);
void DLL_CALLCONV InitPNG(Plugin &plugin, int format_id);
void DLL_CALLCONV InitPNM(Plugin &plugin, int format_id);
void DLL_CALLCONV InitPSD(Plugin &plugin, int format_id);
void DLL_CALLCONV InitRAS(Plugin &plugin, int format_id);
void DLL_CALLCONV InitTARGA(Plugin &plugin, int format_id);
void DLL_CALLCONV InitTIFF(Plugin &plugin, int format_id);
void DLL_CALLCONV InitWBMP(Plugin &plugin, int format_id);

#endif //!PLUGIN_H
