//*****************************************************************************
// Libvidgfx: A graphics library for video compositing
//
// Copyright (C) 2014 Lucas Murray <lmurray@undefinedfire.com>
// All rights reserved.
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//*****************************************************************************

#ifndef LIBVIDGFX_H
#define LIBVIDGFX_H

#include <QtCore/QRect>
#include <QtCore/QString>

// Export symbols from the DLL while allowing header file reuse by users
#ifdef LIBVIDGFX_LIB
#define LVG_EXPORT Q_DECL_EXPORT
#else
#define LVG_EXPORT Q_DECL_IMPORT
#endif

//=============================================================================
// Global application constants

// Library version. NOTE: Don't forget to update the values in the resource
// files as well ("Libvidgfx.rc")
#define LIBVIDGFX_VER_STR "v0.5.0"
#define LIBVIDGFX_VER_MAJOR 0
#define LIBVIDGFX_VER_MINOR 5
#define LIBVIDGFX_VER_BUILD 0

//=============================================================================
// Enumerations

// Do not use line topologies as they are unreliable and prone to bugs
enum GfxTopology {
	GfxTriangleListTopology,
	GfxTriangleStripTopology
};

enum GfxRenderTarget {
	GfxScreenTarget = 0,
	GfxCanvas1Target,
	GfxCanvas2Target,
	GfxScratch1Target,
	GfxScratch2Target,
	GfxUserTarget // A dynamic target (E.g. scaler)
};

// WARNING: If this enum is modified `DShowRenderer::compareFormats()` and
// `D3DContext::convertToBgrx()` also need to be updated.
enum GfxPixelFormat {
	GfxNoFormat = 0,

	// Uncompressed RGB with a single packed plane
	GfxRGB24Format, // Convert to RGB32 on CPU
	GfxRGB32Format, // DXGI_FORMAT_B8G8R8X8_UNORM
	GfxARGB32Format, // DXGI_FORMAT_B8G8R8A8_UNORM

	// YUV 4:2:0 formats with 3 separate planes
	GfxYV12Format, // NxM Y, (N/2)x(M/2) V, (N/2)x(M/2) U
	GfxIYUVFormat, // NxM Y, (N/2)x(M/2) U, (N/2)x(M/2) V (A.k.a. I420) (Logitech C920)

	// YUV 4:2:0 formats with 2 separate planes
	GfxNV12Format, // NxM Y, Nx(M/2) interleaved UV

	// YUV 4:2:2 formats with a single packed plane
	GfxUYVYFormat, // UYVY, "Most popular after YV12"
	GfxHDYCFormat, // UYVY with BT.709, Used by Blackmagic Design
	GfxYUY2Format, // YUYV (Microsoft HD-3000, MacBook Pro FaceTime HD)

	NUM_PIXEL_FORMAT_TYPES // Must be last
};
static const char * const GfxPixelFormatStrings[] = {
	"Unknown",

	// Uncompressed RGB with a single packed plane
	"RGB24",
	"RGB32",
	"ARGB32",

	// YUV 4:2:0 formats with 3 separate planes
	"YV12",
	"IYUV",

	// YUV 4:2:0 formats with 2 separate planes
	"NV12",

	// YUV 4:2:2 formats with a single packed plane
	"UYVY",
	"HDYC",
	"YUY2"
};

enum GfxShader {
	GfxNoShader = 0,
	GfxSolidShader,
	GfxTexDecalShader,
	GfxTexDecalGbcsShader,
	GfxTexDecalRgbShader,
	GfxResizeLayerShader,
	GfxRgbNv16Shader,
	GfxYv12RgbShader,
	GfxUyvyRgbShader,
	GfxHdycRgbShader,
	GfxYuy2RgbShader
};

enum GfxFilter {
	// Standard filters shown to the user
	GfxPointFilter = 0,
	GfxBilinearFilter,
	//GfxBicubicFilter,

	NUM_STANDARD_TEXTURE_FILTERS, // Must be after all standard filters

	// Special internal filters
	GfxResizeLayerFilter = NUM_STANDARD_TEXTURE_FILTERS
};
static const char * const GfxFilterStrings[] = {
	"Nearest neighbour",
	"Bilinear",
	//"Bicubic",
};
static const char * const GfxFilterQualityStrings[] = {
	"Low (Nearest neighbour)",
	"Medium (Bilinear)",
	//"High (Bicubic)",
};

enum GfxBlending {
	GfxNoBlending = 0,
	GfxAlphaBlending,
	GfxPremultipliedBlending
};

typedef int GfxTextureFlags;
enum GfxTextureFlags_ {
	GfxWritableFlag = (1 << 0),
	GfxTargetableFlag = (1 << 1),
	GfxStagingFlag = (1 << 2),
	GfxGDIFlag = (1 << 3) // Used by `D3DContext` only
};

enum GfxOrientation {
	GfxUnchangedOrient = 0,
	GfxFlippedOrient,
	GfxMirroredOrient,
	GfxFlippedMirroredOrient
};

//=============================================================================
// Library initialization

LVG_EXPORT bool	initLibvidgfx_internal(
	int libVerMajor, int libVerMinor, int libVerPatch);

/// <summary>
/// Initializes Libvidgfx. Must be called as the very first thing in `main()`.
/// </summary>
#define INIT_LIBVIDGFX() \
	if(!initLibvidgfx_internal( \
	LIBVIDGFX_VER_MAJOR, LIBVIDGFX_VER_MINOR, LIBVIDGFX_VER_BUILD)) \
	return 1

#endif // LIBVIDGFX_H
