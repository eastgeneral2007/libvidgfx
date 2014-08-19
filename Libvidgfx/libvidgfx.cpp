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

#include "include/libvidgfx.h"
#include "d3dcontext.h"
#include "gfxlog.h"
#include <iostream>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#include <QtGui/QImage>

//=============================================================================
// Helpers

#ifdef Q_OS_WIN
/// <summary>
/// Converts a `QString` into a `wchar_t` array. We cannot use
/// `QString::toStdWString()` as it's not enabled in our Qt build. The caller
/// is responsible for calling `delete[]` on the returned string.
/// </summary>
static wchar_t *QStringToWChar(const QString &str)
{
	// We must keep the QByteArray in memory as otherwise its `data()` gets
	// freed and we end up converting undefined data
	QByteArray data = str.toUtf8();
	const char *msg = data.data();
	int len = MultiByteToWideChar(CP_UTF8, 0, msg, -1, NULL, 0);
	wchar_t *wstr = new wchar_t[len];
	MultiByteToWideChar(CP_UTF8, 0, msg, -1, wstr, len);
	return wstr;
}
#endif

/// <summary>
/// Displays a very basic error dialog box. Used as a last resort.
/// </summary>
static void showBasicErrorMessageBox(
	const QString &msg, const QString &caption)
{
#ifdef Q_OS_WIN
	wchar_t *wMessage = QStringToWChar(msg);
	wchar_t *wCaption = QStringToWChar(caption);
	MessageBox(NULL, wMessage, wCaption, MB_OK | MB_ICONERROR);
	delete[] wMessage;
	delete[] wCaption;
#else
#error Unsupported platform
#endif
}

//=============================================================================
// Library initialization

bool initLibvidgfx_internal(int libVerMajor, int libVerMinor, int libVerPatch)
{
	static bool inited = false;
	if(inited)
		return false; // Already initialized
	inited = true;

	// Test Libvidgfx version. TODO: When the API is stable we should not test
	// to see if the patch version is the same
	if(libVerMajor != LIBVIDGFX_VER_MAJOR ||
		libVerMinor != LIBVIDGFX_VER_MINOR ||
		libVerPatch != LIBVIDGFX_VER_BUILD)
	{
		QString msg = QStringLiteral("Fatal: Mismatched Libvidgfx version!");

		// Output to the terminal
		QByteArray output = msg.toLocal8Bit();
		std::cout << output.constData() << std::endl;

		// Visual Studio does not display stdout in the debug console so we
		// need to use a special Windows API
#if defined(Q_OS_WIN) && defined(QT_DEBUG)
		// Output to the Visual Studio or system debugger in debug builds only
		OutputDebugStringA(output.constData());
		OutputDebugStringA("\r\n");
#endif

		// Display a message box so the user knows something went wrong
		showBasicErrorMessageBox(msg, QStringLiteral("Libvidgfx"));

		return false;
	}

	// Initialize resources in our QRC file so we can access them
	Q_INIT_RESOURCE(Libvidgfx);

	return true;
}

//=============================================================================
// GfxLog C interface

void vidgfx_set_log_callback(
	VidgfxLogCallback *callback)
{
	GfxLog::setCallback(callback);
}

//=============================================================================
// VertexBuffer C interface

//-----------------------------------------------------------------------------
// Methods

float *vidgfx_vertbuf_get_data_ptr(
	VidgfxVertBuf *buf)
{
	VertexBuffer *ptr = reinterpret_cast<VertexBuffer *>(buf);
	return ptr->getDataPtr();
}

int vidgfx_vertbuf_get_num_floats(
	VidgfxVertBuf *buf)
{
	VertexBuffer *ptr = reinterpret_cast<VertexBuffer *>(buf);
	return ptr->getNumFloats();
}

void vidgfx_vertbuf_set_num_verts(
	VidgfxVertBuf *buf,
	int num_verts)
{
	VertexBuffer *ptr = reinterpret_cast<VertexBuffer *>(buf);
	ptr->setNumVerts(num_verts);
}

int vidgfx_vertbuf_get_num_verts(
	VidgfxVertBuf *buf)
{
	VertexBuffer *ptr = reinterpret_cast<VertexBuffer *>(buf);
	return ptr->getNumVerts();
}

void vidgfx_vertbuf_set_vert_size(
	VidgfxVertBuf *buf,
	int vert_size)
{
	VertexBuffer *ptr = reinterpret_cast<VertexBuffer *>(buf);
	ptr->setVertSize(vert_size);
}

int vidgfx_vertbuf_get_vert_size(
	VidgfxVertBuf *buf)
{
	VertexBuffer *ptr = reinterpret_cast<VertexBuffer *>(buf);
	return ptr->getVertSize();
}

void vidgfx_vertbuf_set_dirty(
	VidgfxVertBuf *buf,
	bool dirty)
{
	VertexBuffer *ptr = reinterpret_cast<VertexBuffer *>(buf);
	ptr->setDirty(dirty);
}

bool vidgfx_vertbuf_is_dirty(
	VidgfxVertBuf *buf)
{
	VertexBuffer *ptr = reinterpret_cast<VertexBuffer *>(buf);
	return ptr->isDirty();
}

//=============================================================================
// TexDecalVertBuf C interface

//-----------------------------------------------------------------------------
// Constructor/destructor

VidgfxTexDecalBuf *vidgfx_texdecalbuf_new(
	VidgfxContext *context)
{
	GraphicsContext *con = reinterpret_cast<GraphicsContext *>(context);
	TexDecalVertBuf *vertBuf = new TexDecalVertBuf(con);
	return reinterpret_cast<VidgfxTexDecalBuf *>(vertBuf);
}

void vidgfx_texdecalbuf_destroy(
	VidgfxTexDecalBuf *buf)
{
	TexDecalVertBuf *vertBuf = reinterpret_cast<TexDecalVertBuf *>(buf);
	if(vertBuf != NULL)
		delete vertBuf;
}

//-----------------------------------------------------------------------------
// Methods

void vidgfx_texdecalbuf_set_context(
	VidgfxTexDecalBuf *buf,
	VidgfxContext *context)
{
	TexDecalVertBuf *ptr = reinterpret_cast<TexDecalVertBuf *>(buf);
	GraphicsContext *con = reinterpret_cast<GraphicsContext *>(context);
	ptr->setContext(con);
}

VidgfxVertBuf *vidgfx_texdecalbuf_get_vert_buf(
	VidgfxTexDecalBuf *buf)
{
	TexDecalVertBuf *ptr = reinterpret_cast<TexDecalVertBuf *>(buf);
	VertexBuffer *ret = ptr->getVertBuf();
	return reinterpret_cast<VidgfxVertBuf *>(ret);
}

GfxTopology vidgfx_texdecalbuf_get_topology(
	VidgfxTexDecalBuf *buf)
{
	TexDecalVertBuf *ptr = reinterpret_cast<TexDecalVertBuf *>(buf);
	return ptr->getTopology();
}

void vidgfx_texdecalbuf_destroy_vert_buf(
	VidgfxTexDecalBuf *buf)
{
	TexDecalVertBuf *ptr = reinterpret_cast<TexDecalVertBuf *>(buf);
	ptr->deleteVertBuf();
}

// Position
void vidgfx_texdecalbuf_set_rect(
	VidgfxTexDecalBuf *buf,
	const QRectF &rect)
{
	TexDecalVertBuf *ptr = reinterpret_cast<TexDecalVertBuf *>(buf);
	ptr->setRect(rect);
}

QRectF vidgfx_texdecalbuf_get_rect(
	VidgfxTexDecalBuf *buf)
{
	TexDecalVertBuf *ptr = reinterpret_cast<TexDecalVertBuf *>(buf);
	return ptr->getRect();
}

// Scrolling
void vidgfx_texdecalbuf_scroll_by(
	VidgfxTexDecalBuf *buf,
	const QPointF &delta)
{
	TexDecalVertBuf *ptr = reinterpret_cast<TexDecalVertBuf *>(buf);
	ptr->scrollBy(delta);
}

void vidgfx_texdecalbuf_scroll_by(
	VidgfxTexDecalBuf *buf,
	float x_delta,
	float y_delta)
{
	TexDecalVertBuf *ptr = reinterpret_cast<TexDecalVertBuf *>(buf);
	ptr->scrollBy(x_delta, y_delta);
}

void vidgfx_texdecalbuf_reset_scrolling(
	VidgfxTexDecalBuf *buf)
{
	TexDecalVertBuf *ptr = reinterpret_cast<TexDecalVertBuf *>(buf);
	ptr->resetScrolling();
}

void vidgfx_texdecalbuf_set_round_offset(
	VidgfxTexDecalBuf *buf,
	bool round)
{
	TexDecalVertBuf *ptr = reinterpret_cast<TexDecalVertBuf *>(buf);
	ptr->setRoundOffset(round);
}

bool vidgfx_texdecalbuf_get_round_offset(
	VidgfxTexDecalBuf *buf)
{
	TexDecalVertBuf *ptr = reinterpret_cast<TexDecalVertBuf *>(buf);
	return ptr->getRoundOffset();
}

// Texture UV
void vidgfx_texdecalbuf_set_tex_uv(
	VidgfxTexDecalBuf *buf,
	const QPointF &top_left,
	const QPointF &top_right,
	const QPointF &bot_left,
	const QPointF &bot_right)
{
	TexDecalVertBuf *ptr = reinterpret_cast<TexDecalVertBuf *>(buf);
	ptr->setTextureUv(top_left, top_right, bot_left, bot_right);
}

void vidgfx_texdecalbuf_set_tex_uv(
	VidgfxTexDecalBuf *buf,
	const QRectF &norm_rect,
	GfxOrientation orient)
{
	TexDecalVertBuf *ptr = reinterpret_cast<TexDecalVertBuf *>(buf);
	ptr->setTextureUv(norm_rect, orient);
}

void vidgfx_texdecalbuf_set_tex_uv(
	VidgfxTexDecalBuf *buf,
	const QPointF &top_left,
	const QPointF &bot_right,
	GfxOrientation orient)
{
	TexDecalVertBuf *ptr = reinterpret_cast<TexDecalVertBuf *>(buf);
	ptr->setTextureUv(top_left, bot_right, orient);
}

void vidgfx_texdecalbuf_get_tex_uv(
	VidgfxTexDecalBuf *buf,
	QPointF *top_left,
	QPointF *top_right,
	QPointF *bot_left,
	QPointF *bot_right)
{
	TexDecalVertBuf *ptr = reinterpret_cast<TexDecalVertBuf *>(buf);
	ptr->getTextureUv(top_left, top_right, bot_left, bot_right);
}

//=============================================================================
// Texture C interface

//-----------------------------------------------------------------------------
// Methods

bool vidgfx_tex_is_valid(
	VidgfxTex *tex)
{
	if(tex == NULL)
		return false;
	Texture *ptr = reinterpret_cast<Texture *>(tex);
	return ptr->isValid();
}

bool vidgfx_tex_is_mapped(
	VidgfxTex *tex)
{
	Texture *ptr = reinterpret_cast<Texture *>(tex);
	return ptr->isMapped();
}

void *vidgfx_tex_get_data_ptr(
	VidgfxTex *tex)
{
	Texture *ptr = reinterpret_cast<Texture *>(tex);
	return ptr->getDataPtr();
}

int vidgfx_tex_get_stride(
	VidgfxTex *tex)
{
	Texture *ptr = reinterpret_cast<Texture *>(tex);
	return ptr->getStride();
}

bool vidgfx_tex_is_writable(
	VidgfxTex *tex)
{
	Texture *ptr = reinterpret_cast<Texture *>(tex);
	return ptr->isWritable();
}

bool vidgfx_tex_is_targetable(
	VidgfxTex *tex)
{
	Texture *ptr = reinterpret_cast<Texture *>(tex);
	return ptr->isTargetable();
}

bool vidgfx_tex_is_staging(
	VidgfxTex *tex)
{
	Texture *ptr = reinterpret_cast<Texture *>(tex);
	return ptr->isStaging();
}

QSize vidgfx_tex_get_size(
	VidgfxTex *tex)
{
	Texture *ptr = reinterpret_cast<Texture *>(tex);
	return ptr->getSize();
}

int vidgfx_tex_get_width(
	VidgfxTex *tex)
{
	Texture *ptr = reinterpret_cast<Texture *>(tex);
	return ptr->getWidth();
}

int vidgfx_tex_get_height(
	VidgfxTex *tex)
{
	Texture *ptr = reinterpret_cast<Texture *>(tex);
	return ptr->getHeight();
}

void vidgfx_tex_update_data(
	VidgfxTex *tex,
	const QImage &img)
{
	Texture *ptr = reinterpret_cast<Texture *>(tex);
	ptr->updateData(img);
}

//-----------------------------------------------------------------------------
// Interface

void *vidgfx_tex_map(
	VidgfxTex *tex)
{
	Texture *ptr = reinterpret_cast<Texture *>(tex);
	return ptr->map();
}

void vidgfx_tex_unmap(
	VidgfxTex *tex)
{
	Texture *ptr = reinterpret_cast<Texture *>(tex);
	ptr->unmap();
}

bool vidgfx_tex_is_srgb_hack(
	VidgfxTex *tex)
{
	Texture *ptr = reinterpret_cast<Texture *>(tex);
	return ptr->isSrgbHack();
}

//=============================================================================
// GraphicsContext C interface

//-----------------------------------------------------------------------------
// Static methods

bool vidgfx_create_solid_rect(
	VidgfxVertBuf *out_buf,
	const QRectF &rect,
	const QColor &col)
{
	VertexBuffer *ptr = reinterpret_cast<VertexBuffer *>(out_buf);
	return GraphicsContext::createSolidRect(ptr, rect, col);
}

bool vidgfx_create_solid_rect(
	VidgfxVertBuf *out_buf,
	const QRectF &rect,
	const QColor &tl_col,
	const QColor &tr_col,
	const QColor &bl_col,
	const QColor &br_col)
{
	VertexBuffer *ptr = reinterpret_cast<VertexBuffer *>(out_buf);
	return GraphicsContext::createSolidRect(
		ptr, rect, tl_col, tr_col, bl_col, br_col);
}

bool vidgfx_create_solid_rect_outline(
	VidgfxVertBuf *out_buf,
	const QRectF &rect,
	const QColor &col,
	const QPointF &half_width)
{
	VertexBuffer *ptr = reinterpret_cast<VertexBuffer *>(out_buf);
	return GraphicsContext::createSolidRectOutline(
		ptr, rect, col, half_width);
}

bool vidgfx_create_solid_rect_outline(
	VidgfxVertBuf *out_buf,
	const QRectF &rect,
	const QColor &tl_col,
	const QColor &tr_col,
	const QColor &bl_col,
	const QColor &br_col,
	const QPointF &half_width)
{
	VertexBuffer *ptr = reinterpret_cast<VertexBuffer *>(out_buf);
	return GraphicsContext::createSolidRectOutline(
		ptr, rect, tl_col, tr_col, bl_col, br_col, half_width);
}

bool vidgfx_create_tex_decal_rect(
	VidgfxVertBuf *out_buf,
	const QRectF &rect)
{
	VertexBuffer *ptr = reinterpret_cast<VertexBuffer *>(out_buf);
	return GraphicsContext::createTexDecalRect(ptr, rect);
}

bool vidgfx_create_tex_decal_rect(
	VidgfxVertBuf *out_buf,
	const QRectF &rect,
	const QPointF &br_uv)
{
	VertexBuffer *ptr = reinterpret_cast<VertexBuffer *>(out_buf);
	return GraphicsContext::createTexDecalRect(ptr, rect, br_uv);
}

bool vidgfx_create_tex_decal_rect(
	VidgfxVertBuf *out_buf,
	const QRectF &rect,
	const QPointF &tl_uv,
	const QPointF &tr_uv,
	const QPointF &bl_uv,
	const QPointF &br_uv)
{
	VertexBuffer *ptr = reinterpret_cast<VertexBuffer *>(out_buf);
	return GraphicsContext::createTexDecalRect(
		ptr, rect, tl_uv, tr_uv, bl_uv, br_uv);
}

bool vidgfx_create_resize_rect(
	VidgfxVertBuf *out_buf,
	const QRectF &rect,
	float handle_size,
	const QPointF &half_width)
{
	VertexBuffer *ptr = reinterpret_cast<VertexBuffer *>(out_buf);
	return GraphicsContext::createResizeRect(
		ptr, rect, handle_size, half_width);
}

quint32 vidgfx_next_pow_two(
	quint32 n)
{
	return GraphicsContext::nextPowTwo(n);
}

//-----------------------------------------------------------------------------
// Methods

void vidgfx_context_set_view_mat(
	VidgfxContext *context,
	const QMatrix4x4 &matrix)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	ptr->setViewMatrix(matrix);
}

QMatrix4x4 vidgfx_context_get_view_mat(
	VidgfxContext *context)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	return ptr->getViewMatrix();
}

void vidgfx_context_set_proj_mat(
	VidgfxContext *context,
	const QMatrix4x4 &matrix)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	ptr->setProjectionMatrix(matrix);
}

QMatrix4x4 vidgfx_context_get_proj_mat(
	VidgfxContext *context)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	return ptr->getProjectionMatrix();
}

void vidgfx_context_set_screen_view_mat(
	VidgfxContext *context,
	const QMatrix4x4 &matrix)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	ptr->setScreenViewMatrix(matrix);
}

QMatrix4x4 vidgfx_context_get_screen_view_mat(
	VidgfxContext *context)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	return ptr->getScreenViewMatrix();
}

void vidgfx_context_set_screen_proj_mat(
	VidgfxContext *context,
	const QMatrix4x4 &matrix)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	ptr->setScreenProjectionMatrix(matrix);
}

QMatrix4x4 vidgfx_context_get_screen_proj_mat(
	VidgfxContext *context)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	return ptr->getScreenProjectionMatrix();
}

void vidgfx_context_set_user_render_target(
	VidgfxContext *context,
	VidgfxTex *tex_a,
	VidgfxTex *tex_b)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	Texture *texA = reinterpret_cast<Texture *>(tex_a);
	Texture *texB = reinterpret_cast<Texture *>(tex_b);
	ptr->setUserRenderTarget(texA, texB);
}

VidgfxTex *vidgfx_context_get_user_render_target(
	VidgfxContext *context,
	int index)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	Texture *ret = ptr->getUserRenderTarget(index);
	return reinterpret_cast<VidgfxTex *>(ret);
}

void vidgfx_context_set_user_render_target_viewport(
	VidgfxContext *context,
	const QRect &rect)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	ptr->setUserRenderTargetViewport(rect);
}

void vidgfx_context_set_user_render_target_viewport(
	VidgfxContext *context,
	const QSize &size)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	ptr->setUserRenderTargetViewport(size);
}

QRect vidgfx_context_get_user_render_target_viewport(
	VidgfxContext *context)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	return ptr->getUserRenderTargetViewport();
}

void vidgfx_context_set_resize_layer_rect(
	VidgfxContext *context,
	const QRectF &rect)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	ptr->setResizeLayerRect(rect);
}

QRectF vidgfx_context_get_resize_layer_rect(
	VidgfxContext *context)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	return ptr->getResizeLayerRect();
}

void vidgfx_context_set_rgb_nv16_px_size(
	VidgfxContext *context,
	const QPointF &size)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	ptr->setRgbNv16PxSize(size);
}

QPointF vidgfx_context_get_rgb_nv16_px_size(
	VidgfxContext *context)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	return ptr->getRgbNv16PxSize();
}

void vidgfx_context_set_tex_decal_mod_color(
	VidgfxContext *context,
	const QColor &color)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	ptr->setTexDecalModColor(color);
}

QColor vidgfx_context_get_tex_decal_mod_color(
	VidgfxContext *context)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	return ptr->getTexDecalModColor();
}

void vidgfx_context_set_tex_decal_effects(
	VidgfxContext *context,
	float gamma,
	float brightness,
	float contrast,
	float saturation)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	ptr->setTexDecalEffects(gamma, brightness, contrast, saturation);
}

bool vidgfx_context_set_tex_decal_effects_helper(
	VidgfxContext *context,
	float gamma,
	int brightness,
	int contrast,
	int saturation)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	return ptr->setTexDecalEffectsHelper(
		gamma, brightness, contrast, saturation);
}

const float *vidgfx_context_get_tex_decal_effects(
	VidgfxContext *context)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	return ptr->getTexDecalEffects();
}

bool vidgfx_context_dilute_img(
	VidgfxContext *context,
	QImage &img)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	return ptr->diluteImage(img);
}

//-----------------------------------------------------------------------------
// Interface

bool vidgfx_context_is_valid(
	VidgfxContext *context)
{
	if(context == NULL)
		return false;
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	return ptr->isValid();
}

void vidgfx_context_flush(
	VidgfxContext *context)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	ptr->flush();
}

VidgfxVertBuf *vidgfx_context_new_vertbuf(
	VidgfxContext *context,
	int size)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	VertexBuffer *ret = ptr->createVertexBuffer(size);
	return reinterpret_cast<VidgfxVertBuf *>(ret);
}

void vidgfx_context_destroy_vertbuf(
	VidgfxContext *context,
	VidgfxVertBuf *buf)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	VertexBuffer *vertBuf = reinterpret_cast<VertexBuffer *>(buf);
	ptr->deleteVertexBuffer(vertBuf);
}

VidgfxTex *vidgfx_context_new_tex(
	VidgfxContext *context,
	QImage img,
	bool writable,
	bool targetable)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	Texture *ret = ptr->createTexture(img, writable, targetable);
	return reinterpret_cast<VidgfxTex *>(ret);
}

VidgfxTex *vidgfx_context_new_tex(
	VidgfxContext *context,
	const QSize &size,
	bool writable,
	bool targetable,
	bool use_bgra)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	Texture *ret = ptr->createTexture(size, writable, targetable, use_bgra);
	return reinterpret_cast<VidgfxTex *>(ret);
}

VidgfxTex *vidgfx_context_new_tex(
	VidgfxContext *context,
	const QSize &size,
	VidgfxTex *same_format,
	bool writable,
	bool targetable)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	Texture *ret = ptr->createTexture(size, same_format, writable, targetable);
	return reinterpret_cast<VidgfxTex *>(ret);
}

VidgfxTex *vidgfx_context_new_staging_tex(
	VidgfxContext *context,
	const QSize &size)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	Texture *ret = ptr->createStagingTexture(size);
	return reinterpret_cast<VidgfxTex *>(ret);
}

void vidgfx_context_destroy_tex(
	VidgfxContext *context,
	VidgfxTex *tex)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	Texture *texture = reinterpret_cast<Texture *>(tex);
	ptr->deleteTexture(texture);
}

bool vidgfx_context_copy_tex_data(
	VidgfxContext *context,
	VidgfxTex *dst,
	VidgfxTex *src,
	const QPoint &dst_pos,
	const QRect &src_rect)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	Texture *dest = reinterpret_cast<Texture *>(dst);
	Texture *source = reinterpret_cast<Texture *>(src);
	return ptr->copyTextureData(dest, source, dst_pos, src_rect);
}

void vidgfx_context_resize_screen_target(
	VidgfxContext *context,
	const QSize &new_size)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	ptr->resizeScreenTarget(new_size);
}

void vidgfx_context_resize_canvas_target(
	VidgfxContext *context,
	const QSize &new_size)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	ptr->resizeCanvasTarget(new_size);
}

void vidgfx_context_resize_scratch_target(
	VidgfxContext *context,
	const QSize &new_size)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	ptr->resizeScratchTarget(new_size);
}

void vidgfx_context_swap_screen_bufs(
	VidgfxContext *context)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	ptr->swapScreenBuffers();
}

VidgfxTex *vidgfx_context_get_target_tex(
	VidgfxContext *context,
	VidgfxRendTarget target)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	Texture *ret = ptr->getTargetTexture(target);
	return reinterpret_cast<VidgfxTex *>(ret);
}

GfxRenderTarget vidgfx_context_get_next_scratch_target(
	VidgfxContext *context)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	return ptr->getNextScratchTarget();
}

QPointF vidgfx_context_get_scratch_target_to_tex_ratio(
	VidgfxContext *context)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	return ptr->getScratchTargetToTextureRatio();
}

VidgfxTex *vidgfx_context_prepare_tex(
	VidgfxContext *context,
	VidgfxTex *tex,
	const QSize &size,
	VidgfxFilter filter,
	bool set_filter,
	QPointF &px_size_out,
	QPointF &bot_right_out)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	Texture *texture = reinterpret_cast<Texture *>(tex);
	Texture *ret = ptr->prepareTexture(
		texture, size, filter, set_filter, px_size_out, bot_right_out);
	return reinterpret_cast<VidgfxTex *>(ret);
}

VidgfxTex *vidgfx_context_prepare_tex(
	VidgfxContext *context,
	VidgfxTex *tex,
	const QRect &crop_rect,
	const QSize &size,
	VidgfxFilter filter,
	bool set_filter,
	QPointF &px_size_out,
	QPointF &top_left_out,
	QPointF &bot_right_out)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	Texture *texture = reinterpret_cast<Texture *>(tex);
	Texture *ret = ptr->prepareTexture(
		texture, crop_rect, size, filter, set_filter, px_size_out,
		top_left_out, bot_right_out);
	return reinterpret_cast<VidgfxTex *>(ret);
}

VidgfxTex *vidgfx_context_convert_to_bgrx(
	VidgfxContext *context,
	VidgfxPixFormat format,
	VidgfxTex *plane_a,
	VidgfxTex *plane_b,
	VidgfxTex *plane_c)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	Texture *planeA = reinterpret_cast<Texture *>(plane_a);
	Texture *planeB = reinterpret_cast<Texture *>(plane_b);
	Texture *planeC = reinterpret_cast<Texture *>(plane_c);
	Texture *ret = ptr->convertToBgrx(format, planeA, planeB, planeC);
	return reinterpret_cast<VidgfxTex *>(ret);
}

void vidgfx_context_set_render_target(
	VidgfxContext *context,
	VidgfxRendTarget target)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	ptr->setRenderTarget(target);
}

void vidgfx_context_set_shader(
	VidgfxContext *context,
	VidgfxShader shader)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	ptr->setShader(shader);
}

void vidgfx_context_set_topology(
	VidgfxContext *context,
	VidgfxTopology topology)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	ptr->setTopology(topology);
}

void vidgfx_context_set_blending(
	VidgfxContext *context,
	VidgfxBlending blending)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	ptr->setBlending(blending);
}

void vidgfx_context_set_tex(
	VidgfxContext *context,
	VidgfxTex *tex_a,
	VidgfxTex *tex_b,
	VidgfxTex *tex_c)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	Texture *texA = reinterpret_cast<Texture *>(tex_a);
	Texture *texB = reinterpret_cast<Texture *>(tex_b);
	Texture *texC = reinterpret_cast<Texture *>(tex_c);
	ptr->setTexture(texA, texB, texC);
}

void vidgfx_context_set_tex_filter(
	VidgfxContext *context,
	VidgfxFilter filter)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	ptr->setTextureFilter(filter);
}

void vidgfx_context_clear(
	VidgfxContext *context,
	const QColor &color)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	ptr->clear(color);
}

void vidgfx_context_draw_buf(
	VidgfxContext *context,
	VidgfxVertBuf *buf,
	int num_vertices,
	int start_vertex)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	VertexBuffer *vertBuf = reinterpret_cast<VertexBuffer *>(buf);
	ptr->drawBuffer(vertBuf, num_vertices, start_vertex);
}

//-----------------------------------------------------------------------------
// Signals

void vidgfx_context_add_initialized_callback(
	VidgfxContext *context,
	VidgfxContextInitializedCallback *initialized,
	void *opaque)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	ptr->addInitializedCallback(initialized, opaque);
}

void vidgfx_context_remove_initialized_callback(
	VidgfxContext *context,
	VidgfxContextInitializedCallback *initialized,
	void *opaque)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	ptr->removeInitializedCallback(initialized, opaque);
}

void vidgfx_context_add_destroying_callback(
	VidgfxContext *context,
	VidgfxContextDestroyingCallback *destroying,
	void *opaque)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	ptr->addDestroyingCallback(destroying, opaque);
}

void vidgfx_context_remove_destroying_callback(
	VidgfxContext *context,
	VidgfxContextDestroyingCallback *destroying,
	void *opaque)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	ptr->removeDestroyingCallback(destroying, opaque);
}

//=============================================================================
// D3DTexture C API

#if VIDGFX_D3D_ENABLED

//-----------------------------------------------------------------------------
// Constructor/destructor

VidgfxD3DTex *vidgfx_tex_get_d3dtex(
	VidgfxTex *tex)
{
	Texture *ptr = reinterpret_cast<Texture *>(tex);
	D3DTexture *d3dTex = static_cast<D3DTexture *>(ptr);
	return reinterpret_cast<VidgfxD3DTex *>(d3dTex);
}

VidgfxTex *vidgfx_d3dtex_get_tex(
	VidgfxD3DTex *tex)
{
	D3DTexture *ptr = reinterpret_cast<D3DTexture *>(tex);
	Texture *texture = static_cast<Texture *>(ptr);
	return reinterpret_cast<VidgfxTex *>(texture);
}

//-----------------------------------------------------------------------------
// Methods

HDC vidgfx_d3dtex_get_dc(
	VidgfxD3DTex *tex)
{
	D3DTexture *ptr = reinterpret_cast<D3DTexture *>(tex);
	return ptr->getDC();
}

void vidgfx_d3dtex_release_dc(
	VidgfxD3DTex *tex)
{
	D3DTexture *ptr = reinterpret_cast<D3DTexture *>(tex);
	ptr->releaseDC();
}

#endif // VIDGFX_D3D_ENABLED

//=============================================================================
// D3DContext C API

#if VIDGFX_D3D_ENABLED

//-----------------------------------------------------------------------------
// Static methods

HRESULT vidgfx_d3d_create_dxgifactory1_dyn(
	IDXGIFactory1 **factory_out)
{
	return D3DContext::createDXGIFactory1Dynamic(factory_out);
}

void vidgfx_d3d_log_display_adapters()
{
	D3DContext::logDisplayAdapters();
}

//-----------------------------------------------------------------------------
// Constructor/destructor

VidgfxD3DContext *vidgfx_d3dcontext_new()
{
	D3DContext *d3dContext = new D3DContext();
	return reinterpret_cast<VidgfxD3DContext *>(d3dContext);
}

void vidgfx_d3dcontext_destroy(
	VidgfxD3DContext *context)
{
	D3DContext *ptr = reinterpret_cast<D3DContext *>(context);
	if(ptr != NULL)
		delete ptr;
}

VidgfxD3DContext *vidgfx_context_get_d3dcontext(
	VidgfxContext *context)
{
	GraphicsContext *ptr = reinterpret_cast<GraphicsContext *>(context);
	D3DContext *d3dContext = static_cast<D3DContext *>(ptr);
	return reinterpret_cast<VidgfxD3DContext *>(d3dContext);
}

VidgfxContext *vidgfx_d3dcontext_get_context(
	VidgfxD3DContext *context)
{
	D3DContext *ptr = reinterpret_cast<D3DContext *>(context);
	GraphicsContext *gfx = static_cast<GraphicsContext *>(ptr);
	return reinterpret_cast<VidgfxContext *>(gfx);
}

//-----------------------------------------------------------------------------
// Methods

bool vidgfx_d3dcontext_is_valid(
	VidgfxD3DContext *context)
{
	if(context == NULL)
		return false;
	D3DContext *ptr = reinterpret_cast<D3DContext *>(context);
	return ptr->isValid();
}

bool vidgfx_d3dcontext_init(
	VidgfxD3DContext *context,
	HWND hwnd,
	const QSize &size,
	const QColor &resize_border_col)
{
	D3DContext *ptr = reinterpret_cast<D3DContext *>(context);
	return ptr->initialize(hwnd, size, resize_border_col);
}

ID3D10Device *vidgfx_d3dcontext_get_device(
	VidgfxD3DContext *context)
{
	D3DContext *ptr = reinterpret_cast<D3DContext *>(context);
	return ptr->getDevice();
}

bool vidgfx_d3dcontext_has_dxgi11(
	VidgfxD3DContext *context)
{
	D3DContext *ptr = reinterpret_cast<D3DContext *>(context);
	return ptr->hasDxgi11();
}

bool vidgfx_d3dcontext_has_bgra_tex_support(
	VidgfxD3DContext *context)
{
	D3DContext *ptr = reinterpret_cast<D3DContext *>(context);
	return ptr->hasBgraTexSupport();
}

VidgfxTex *vidgfx_d3dcontext_new_gdi_tex(
	VidgfxD3DContext *context,
	const QSize &size)
{
	D3DContext *ptr = reinterpret_cast<D3DContext *>(context);
	Texture *ret = ptr->createGDITexture(size);
	return reinterpret_cast<VidgfxTex *>(ret);
}

VidgfxTex *vidgfx_d3dcontext_open_shared_tex(
	VidgfxD3DContext *context,
	HANDLE shared_handle)
{
	D3DContext *ptr = reinterpret_cast<D3DContext *>(context);
	Texture *ret = ptr->openSharedTexture(shared_handle);
	return reinterpret_cast<VidgfxTex *>(ret);
}

VidgfxTex *vidgfx_d3dcontext_open_dx10_tex(
	VidgfxD3DContext *context,
	ID3D10Texture2D *tex)
{
	D3DContext *ptr = reinterpret_cast<D3DContext *>(context);
	Texture *ret = ptr->openDX10Texture(tex);
	return reinterpret_cast<VidgfxTex *>(ret);
}

//-----------------------------------------------------------------------------
// Signals

void vidgfx_d3dcontext_add_dxgi11_changed_callback(
	VidgfxD3DContext *context,
	VidgfxD3DContextDxgi11ChangedCallback *dxgi11_changed,
	void *opaque)
{
	D3DContext *ptr = reinterpret_cast<D3DContext *>(context);
	ptr->addDxgi11ChangedCallback(dxgi11_changed, opaque);
}

void vidgfx_d3dcontext_remove_dxgi11_changed_callback(
	VidgfxD3DContext *context,
	VidgfxD3DContextDxgi11ChangedCallback *dxgi11_changed,
	void *opaque)
{
	D3DContext *ptr = reinterpret_cast<D3DContext *>(context);
	ptr->removeDxgi11ChangedCallback(dxgi11_changed, opaque);
}

void vidgfx_d3dcontext_add_bgra_tex_support_changed_callback(
	VidgfxD3DContext *context,
	VidgfxD3DContextBgraTexSupportChangedCallback *bgra_tex_support_changed,
	void *opaque)
{
	D3DContext *ptr = reinterpret_cast<D3DContext *>(context);
	ptr->addBgraTexSupportChangedCallback(
		bgra_tex_support_changed, opaque);
}

void vidgfx_d3dcontext_remove_bgra_tex_support_changed_callback(
	VidgfxD3DContext *context,
	VidgfxD3DContextBgraTexSupportChangedCallback *bgra_tex_support_changed,
	void *opaque)
{
	D3DContext *ptr = reinterpret_cast<D3DContext *>(context);
	ptr->removeBgraTexSupportChangedCallback(
		bgra_tex_support_changed, opaque);
}

#endif // VIDGFX_D3D_ENABLED
