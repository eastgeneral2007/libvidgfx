//*****************************************************************************
// Libvidgfx: A graphics library for video compositing
//
// Copyright (C) 2014 Lucas Murray <lucas@polyflare.com>
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
#include <QtGui/QColor>
#include <QtGui/QMatrix4x4>

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
#define LIBVIDGFX_VER_STR "v0.6.0"
#define LIBVIDGFX_VER_MAJOR 0
#define LIBVIDGFX_VER_MINOR 6
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

//=============================================================================
// C interface datatypes

class GraphicsContext;
class Texture;
class VertexBuffer;
class TexDecalVertBuf;

typedef GraphicsContext		VidgfxContext;
typedef Texture				VidgfxTex;
typedef VertexBuffer		VidgfxVertBuf;
typedef TexDecalVertBuf		VidgfxTexDecalBuf;

typedef GfxRenderTarget		VidgfxRendTarget;
typedef GfxFilter			VidgfxFilter;
typedef GfxPixelFormat		VidgfxPixFormat;
typedef GfxShader			VidgfxShader;
typedef GfxTopology			VidgfxTopology;
typedef GfxBlending			VidgfxBlending;

//=============================================================================
// VertexBuffer C interface

//-----------------------------------------------------------------------------
// Methods

LVG_EXPORT float *vidgfx_vertbuf_get_data_ptr(
	VidgfxVertBuf *buf);
LVG_EXPORT int vidgfx_vertbuf_get_num_floats(
	VidgfxVertBuf *buf);

LVG_EXPORT void vidgfx_vertbuf_set_num_verts(
	VidgfxVertBuf *buf,
	int num_verts);
LVG_EXPORT int vidgfx_vertbuf_get_num_verts(
	VidgfxVertBuf *buf);
LVG_EXPORT void vidgfx_vertbuf_set_vert_size(
	VidgfxVertBuf *buf,
	int vert_size);
LVG_EXPORT int vidgfx_vertbuf_get_vert_size(
	VidgfxVertBuf *buf);

LVG_EXPORT void vidgfx_vertbuf_set_dirty(
	VidgfxVertBuf *buf,
	bool dirty = true);
LVG_EXPORT bool vidgfx_vertbuf_is_dirty(
	VidgfxVertBuf *buf);

//=============================================================================
// TexDecalVertBuf C interface

//-----------------------------------------------------------------------------
// Constructor/destructor

LVG_EXPORT VidgfxTexDecalBuf *vidgfx_texdecalbuf_new(
	VidgfxContext *context = NULL);
LVG_EXPORT void vidgfx_texdecalbuf_destroy(
	VidgfxTexDecalBuf *buf);

//-----------------------------------------------------------------------------
// Methods

LVG_EXPORT void vidgfx_texdecalbuf_set_context(
	VidgfxTexDecalBuf *buf,
	VidgfxContext *context);
LVG_EXPORT VertexBuffer *vidgfx_texdecalbuf_get_vert_buf(
	VidgfxTexDecalBuf *buf); // Applies settings
LVG_EXPORT GfxTopology vidgfx_texdecalbuf_get_topology(
	VidgfxTexDecalBuf *buf);
LVG_EXPORT void vidgfx_texdecalbuf_destroy_vert_buf(
	VidgfxTexDecalBuf *buf);

// Position
LVG_EXPORT void vidgfx_texdecalbuf_set_rect(
	VidgfxTexDecalBuf *buf,
	const QRectF &rect);
LVG_EXPORT QRectF vidgfx_texdecalbuf_get_rect(
	VidgfxTexDecalBuf *buf);

// Scrolling
LVG_EXPORT void vidgfx_texdecalbuf_scroll_by(
	VidgfxTexDecalBuf *buf,
	const QPointF &delta);
LVG_EXPORT void vidgfx_texdecalbuf_scroll_by(
	VidgfxTexDecalBuf *buf,
	float x_delta,
	float y_delta);
LVG_EXPORT void vidgfx_texdecalbuf_reset_scrolling(
	VidgfxTexDecalBuf *buf);
LVG_EXPORT void vidgfx_texdecalbuf_set_round_offset(
	VidgfxTexDecalBuf *buf,
	bool round);
LVG_EXPORT bool vidgfx_texdecalbuf_get_round_offset(
	VidgfxTexDecalBuf *buf);

// Texture UV
LVG_EXPORT void vidgfx_texdecalbuf_set_tex_uv(
	VidgfxTexDecalBuf *buf,
	const QPointF &top_left,
	const QPointF &top_right,
	const QPointF &bot_left,
	const QPointF &bot_right);
LVG_EXPORT void vidgfx_texdecalbuf_set_tex_uv(
	VidgfxTexDecalBuf *buf,
	const QRectF &norm_rect,
	GfxOrientation orient = GfxUnchangedOrient);
LVG_EXPORT void vidgfx_texdecalbuf_set_tex_uv(
	VidgfxTexDecalBuf *buf,
	const QPointF &top_left,
	const QPointF &bot_right,
	GfxOrientation orient = GfxUnchangedOrient);
LVG_EXPORT void vidgfx_texdecalbuf_get_tex_uv(
	VidgfxTexDecalBuf *buf,
	QPointF *top_left,
	QPointF *top_right,
	QPointF *bot_left,
	QPointF *bot_right);

//=============================================================================
// Texture C interface

//-----------------------------------------------------------------------------
// Methods

LVG_EXPORT bool vidgfx_tex_is_valid(
	VidgfxTex *tex);
LVG_EXPORT bool vidgfx_tex_is_mapped(
	VidgfxTex *tex);
LVG_EXPORT void *vidgfx_tex_get_data_ptr(
	VidgfxTex *tex);
LVG_EXPORT int vidgfx_tex_get_stride(
	VidgfxTex *tex);

LVG_EXPORT bool vidgfx_tex_is_writable(
	VidgfxTex *tex);
LVG_EXPORT bool vidgfx_tex_is_targetable(
	VidgfxTex *tex);
LVG_EXPORT bool vidgfx_tex_is_staging(
	VidgfxTex *tex);
LVG_EXPORT QSize vidgfx_tex_get_size(
	VidgfxTex *tex);
LVG_EXPORT int vidgfx_tex_get_width(
	VidgfxTex *tex);
LVG_EXPORT int vidgfx_tex_get_height(
	VidgfxTex *tex);

LVG_EXPORT void vidgfx_tex_update_data(
	VidgfxTex *tex,
	const QImage &img);

//-----------------------------------------------------------------------------
// Interface

LVG_EXPORT void *vidgfx_tex_map(
	VidgfxTex *tex);
LVG_EXPORT void vidgfx_tex_unmap(
	VidgfxTex *tex);

LVG_EXPORT bool vidgfx_tex_is_srgb_hack(
	VidgfxTex *tex);

//=============================================================================
// GraphicsContext C interface

//-----------------------------------------------------------------------------
// Static methods

LVG_EXPORT bool vidgfx_create_solid_rect(
	VidgfxVertBuf *out_buf,
	const QRectF &rect,
	const QColor &col);
LVG_EXPORT bool vidgfx_create_solid_rect(
	VidgfxVertBuf *out_buf,
	const QRectF &rect,
	const QColor &tl_col,
	const QColor &tr_col,
	const QColor &bl_col,
	const QColor &br_col);

LVG_EXPORT bool vidgfx_create_solid_rect_outline(
	VidgfxVertBuf *out_buf,
	const QRectF &rect,
	const QColor &col,
	const QPointF &half_width = QPointF(0.5f, 0.5f));
LVG_EXPORT bool vidgfx_create_solid_rect_outline(
	VidgfxVertBuf *out_buf,
	const QRectF &rect,
	const QColor &tl_col,
	const QColor &tr_col,
	const QColor &bl_col,
	const QColor &br_col,
	const QPointF &half_width = QPointF(0.5f, 0.5f));

LVG_EXPORT bool vidgfx_create_tex_decal_rect(
	VidgfxVertBuf *out_buf,
	const QRectF &rect);
LVG_EXPORT bool vidgfx_create_tex_decal_rect(
	VidgfxVertBuf *out_buf,
	const QRectF &rect,
	const QPointF &br_uv);
LVG_EXPORT bool vidgfx_create_tex_decal_rect(
	VidgfxVertBuf *out_buf,
	const QRectF &rect,
	const QPointF &tl_uv,
	const QPointF &tr_uv,
	const QPointF &bl_uv,
	const QPointF &br_uv);

LVG_EXPORT bool vidgfx_create_resize_rect(
	VidgfxVertBuf *out_buf,
	const QRectF &rect,
	float handle_size,
	const QPointF &half_width = QPointF(0.5f, 0.5f));

// Helpers
LVG_EXPORT quint32 vidgfx_next_pow_two(
	quint32 n);

//-----------------------------------------------------------------------------
// Methods

LVG_EXPORT void vidgfx_context_set_view_mat(
	VidgfxContext *context,
	const QMatrix4x4 &matrix);
LVG_EXPORT QMatrix4x4 vidgfx_context_get_view_mat(
	VidgfxContext *context);
LVG_EXPORT void vidgfx_context_set_proj_mat(
	VidgfxContext *context,
	const QMatrix4x4 &matrix);
LVG_EXPORT QMatrix4x4 vidgfx_context_get_proj_mat(
	VidgfxContext *context);

LVG_EXPORT void vidgfx_context_set_screen_view_mat(
	VidgfxContext *context,
	const QMatrix4x4 &matrix);
LVG_EXPORT QMatrix4x4 vidgfx_context_get_screen_view_mat(
	VidgfxContext *context);
LVG_EXPORT void vidgfx_context_set_screen_proj_mat(
	VidgfxContext *context,
	const QMatrix4x4 &matrix);
LVG_EXPORT QMatrix4x4 vidgfx_context_get_screen_proj_mat(
	VidgfxContext *context);

LVG_EXPORT void vidgfx_context_set_user_render_target(
	VidgfxContext *context,
	VidgfxTex *tex_a,
	VidgfxTex *tex_b = NULL);
LVG_EXPORT VidgfxTex *vidgfx_context_get_user_render_target(
	VidgfxContext *context,
	int index);
LVG_EXPORT void vidgfx_context_set_user_render_target_viewport(
	VidgfxContext *context,
	const QRect &rect);
LVG_EXPORT void vidgfx_context_set_user_render_target_viewport(
	VidgfxContext *context,
	const QSize &size);
LVG_EXPORT QRect vidgfx_context_get_user_render_target_viewport(
	VidgfxContext *context);

LVG_EXPORT void vidgfx_context_set_resize_layer_rect(
	VidgfxContext *context,
	const QRectF &rect);
LVG_EXPORT QRectF vidgfx_context_get_resize_layer_rect(
	VidgfxContext *context);

LVG_EXPORT void vidgfx_context_set_rgb_nv16_px_size(
	VidgfxContext *context,
	const QPointF &size);
LVG_EXPORT QPointF vidgfx_context_get_rgb_nv16_px_size(
	VidgfxContext *context);

LVG_EXPORT void vidgfx_context_set_tex_decal_mod_color(
	VidgfxContext *context,
	const QColor &color);
LVG_EXPORT QColor vidgfx_context_get_tex_decal_mod_color(
	VidgfxContext *context);

LVG_EXPORT void vidgfx_context_set_tex_decal_effects(
	VidgfxContext *context,
	float gamma,
	float brightness,
	float contrast,
	float saturation);
LVG_EXPORT bool vidgfx_context_set_tex_decal_effects_helper(
	VidgfxContext *context,
	float gamma,
	int brightness,
	int contrast,
	int saturation);
LVG_EXPORT const float *vidgfx_context_get_tex_decal_effects(
	VidgfxContext *context);

LVG_EXPORT bool vidgfx_context_dilute_img(
	VidgfxContext *context,
	QImage &img);

//-----------------------------------------------------------------------------
// Interface

LVG_EXPORT bool vidgfx_context_is_valid(
	VidgfxContext *context);
LVG_EXPORT void vidgfx_context_flush(
	VidgfxContext *context);

// Buffers
LVG_EXPORT VidgfxVertBuf *vidgfx_context_new_vertbuf(
	VidgfxContext *context,
	int size);
LVG_EXPORT void vidgfx_context_destroy_vertbuf(
	VidgfxContext *context,
	VidgfxVertBuf *buf);
LVG_EXPORT VidgfxTex *vidgfx_context_new_tex(
	VidgfxContext *context,
	QImage img,
	bool writable = false,
	bool targetable = false);
LVG_EXPORT VidgfxTex *vidgfx_context_new_tex(
	VidgfxContext *context,
	const QSize &size,
	bool writable = false,
	bool targetable = false,
	bool use_bgra = false);
LVG_EXPORT VidgfxTex *vidgfx_context_new_tex(
	VidgfxContext *context,
	const QSize &size,
	VidgfxTex *same_format,
	bool writable = false,
	bool targetable = false);
LVG_EXPORT VidgfxTex *vidgfx_context_new_staging_tex(
	VidgfxContext *context,
	const QSize &size);
LVG_EXPORT void vidgfx_context_destroy_tex(
	VidgfxContext *context,
	VidgfxTex *tex);
LVG_EXPORT bool vidgfx_context_copy_tex_data(
	VidgfxContext *context,
	VidgfxTex *dst,
	VidgfxTex *src,
	const QPoint &dst_pos,
	const QRect &src_rect);

// Render targets
LVG_EXPORT void vidgfx_context_resize_screen_target(
	VidgfxContext *context,
	const QSize &new_size);
LVG_EXPORT void vidgfx_context_resize_canvas_target(
	VidgfxContext *context,
	const QSize &new_size);
LVG_EXPORT void vidgfx_context_resize_scratch_target(
	VidgfxContext *context,
	const QSize &new_size);
LVG_EXPORT void vidgfx_context_swap_screen_bufs(
	VidgfxContext *context);
LVG_EXPORT VidgfxTex *vidgfx_context_get_target_tex(
	VidgfxContext *context,
	VidgfxRendTarget target);
LVG_EXPORT GfxRenderTarget vidgfx_context_get_next_scratch_target(
	VidgfxContext *context);
LVG_EXPORT QPointF vidgfx_context_get_scratch_target_to_tex_ratio(
	VidgfxContext *context);

// Advanced rendering
LVG_EXPORT VidgfxTex *vidgfx_context_prepare_tex(
	VidgfxContext *context,
	VidgfxTex *tex,
	const QSize &size,
	VidgfxFilter filter,
	bool set_filter,
	QPointF &px_size_out,
	QPointF &bot_right_out);
LVG_EXPORT VidgfxTex *vidgfx_context_prepare_tex(
	VidgfxContext *context,
	VidgfxTex *tex,
	const QRect &crop_rect,
	const QSize &size,
	VidgfxFilter filter,
	bool set_filter,
	QPointF &px_size_out,
	QPointF &top_left_out,
	QPointF &bot_right_out);
LVG_EXPORT VidgfxTex *vidgfx_context_convert_to_bgrx(
	VidgfxContext *context,
	VidgfxPixFormat format,
	VidgfxTex *plane_a,
	VidgfxTex *plane_b,
	VidgfxTex *plane_c);

// Drawing
LVG_EXPORT void vidgfx_context_set_render_target(
	VidgfxContext *context,
	VidgfxRendTarget target);
LVG_EXPORT void vidgfx_context_set_shader(
	VidgfxContext *context,
	VidgfxShader shader);
LVG_EXPORT void vidgfx_context_set_topology(
	VidgfxContext *context,
	VidgfxTopology topology);
LVG_EXPORT void vidgfx_context_set_blending(
	VidgfxContext *context,
	VidgfxBlending blending);
LVG_EXPORT void vidgfx_context_set_tex(
	VidgfxContext *context,
	VidgfxTex *tex_a,
	VidgfxTex *tex_b = NULL,
	VidgfxTex *tex_c = NULL);
LVG_EXPORT void vidgfx_context_set_tex_filter(
	VidgfxContext *context,
	VidgfxFilter filter);
LVG_EXPORT void vidgfx_context_clear(
	VidgfxContext *context,
	const QColor &color);
LVG_EXPORT void vidgfx_context_draw_buf(
	VidgfxContext *context,
	VidgfxVertBuf *buf,
	int num_vertices = -1,
	int start_vertex = 0);

//-----------------------------------------------------------------------------
// Signals

typedef void VidgfxContextInitializedCallback(
	void *opaque, VidgfxContext *context);
typedef void VidgfxContextDestroyingCallback(
	void *opaque, VidgfxContext *context);

LVG_EXPORT void vidgfx_context_add_initialized_callback(
	VidgfxContext *context,
	VidgfxContextInitializedCallback *initialized,
	void *opaque);
LVG_EXPORT void vidgfx_context_remove_initialized_callback(
	VidgfxContext *context,
	VidgfxContextInitializedCallback *initialized,
	void *opaque);
LVG_EXPORT void vidgfx_context_add_destroying_callback(
	VidgfxContext *context,
	VidgfxContextDestroyingCallback *destroying,
	void *opaque);
LVG_EXPORT void vidgfx_context_remove_destroying_callback(
	VidgfxContext *context,
	VidgfxContextDestroyingCallback *destroying,
	void *opaque);

#endif // LIBVIDGFX_H
