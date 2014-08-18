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
#include "include/graphicscontext.h"
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
// Texture C interface

//-----------------------------------------------------------------------------
// Methods

bool vidgfx_tex_is_valid(
	VidgfxTex *tex)
{
	if(tex == NULL)
		return false;
	return tex->isValid();
}

bool vidgfx_tex_is_mapped(
	VidgfxTex *tex)
{
	return tex->isMapped();
}

void *vidgfx_tex_get_data_ptr(
	VidgfxTex *tex)
{
	return tex->getDataPtr();
}

int vidgfx_tex_get_stride(
	VidgfxTex *tex)
{
	return tex->getStride();
}

bool vidgfx_tex_is_writable(
	VidgfxTex *tex)
{
	return tex->isWritable();
}

bool vidgfx_tex_is_targetable(
	VidgfxTex *tex)
{
	return tex->isTargetable();
}

bool vidgfx_tex_is_staging(
	VidgfxTex *tex)
{
	return tex->isStaging();
}

QSize vidgfx_tex_get_size(
	VidgfxTex *tex)
{
	return tex->getSize();
}

int vidgfx_tex_get_width(
	VidgfxTex *tex)
{
	return tex->getWidth();
}

int vidgfx_tex_get_height(
	VidgfxTex *tex)
{
	return tex->getHeight();
}

void vidgfx_tex_update_data(
	VidgfxTex *tex,
	const QImage &img)
{
	tex->updateData(img);
}

//-----------------------------------------------------------------------------
// Interface

void *vidgfx_tex_map(
	VidgfxTex *tex)
{
	return tex->map();
}

void vidgfx_tex_unmap(
	VidgfxTex *tex)
{
	tex->unmap();
}

bool vidgfx_tex_is_srgb_hack(
	VidgfxTex *tex)
{
	return tex->isSrgbHack();
}

//=============================================================================
// GraphicsContext C interface

//-----------------------------------------------------------------------------
// Static methods

bool vidgfx_create_solid_rect(
	VidgfxVertbuf *out_buf,
	const QRectF &rect,
	const QColor &col)
{
	return GraphicsContext::createSolidRect(out_buf, rect, col);
}

bool vidgfx_create_solid_rect(
	VidgfxVertbuf *out_buf,
	const QRectF &rect,
	const QColor &tl_col,
	const QColor &tr_col,
	const QColor &bl_col,
	const QColor &br_col)
{
	return GraphicsContext::createSolidRect(
		out_buf, rect, tl_col, tr_col, bl_col, br_col);
}

bool vidgfx_create_solid_rect_outline(
	VidgfxVertbuf *out_buf,
	const QRectF &rect,
	const QColor &col,
	const QPointF &half_width)
{
	return GraphicsContext::createSolidRectOutline(
		out_buf, rect, col, half_width);
}

bool vidgfx_create_solid_rect_outline(
	VidgfxVertbuf *out_buf,
	const QRectF &rect,
	const QColor &tl_col,
	const QColor &tr_col,
	const QColor &bl_col,
	const QColor &br_col,
	const QPointF &half_width)
{
	return GraphicsContext::createSolidRectOutline(
		out_buf, rect, tl_col, tr_col, bl_col, br_col, half_width);
}

bool vidgfx_create_tex_decal_rect(
	VidgfxVertbuf *out_buf,
	const QRectF &rect)
{
	return GraphicsContext::createTexDecalRect(out_buf, rect);
}

bool vidgfx_create_tex_decal_rect(
	VidgfxVertbuf *out_buf,
	const QRectF &rect,
	const QPointF &br_uv)
{
	return GraphicsContext::createTexDecalRect(out_buf, rect, br_uv);
}

bool vidgfx_create_tex_decal_rect(
	VidgfxVertbuf *out_buf,
	const QRectF &rect,
	const QPointF &tl_uv,
	const QPointF &tr_uv,
	const QPointF &bl_uv,
	const QPointF &br_uv)
{
	return GraphicsContext::createTexDecalRect(
		out_buf, rect, tl_uv, tr_uv, bl_uv, br_uv);
}

bool vidgfx_create_resize_rect(
	VidgfxVertbuf *out_buf,
	const QRectF &rect,
	float handle_size,
	const QPointF &half_width)
{
	return GraphicsContext::createResizeRect(
		out_buf, rect, handle_size, half_width);
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
	context->setViewMatrix(matrix);
}

QMatrix4x4 vidgfx_context_get_view_mat(
	VidgfxContext *context)
{
	return context->getViewMatrix();
}

void vidgfx_context_set_proj_mat(
	VidgfxContext *context,
	const QMatrix4x4 &matrix)
{
	context->setProjectionMatrix(matrix);
}

QMatrix4x4 vidgfx_context_get_proj_mat(
	VidgfxContext *context)
{
	return context->getProjectionMatrix();
}

void vidgfx_context_set_screen_view_mat(
	VidgfxContext *context,
	const QMatrix4x4 &matrix)
{
	context->setScreenViewMatrix(matrix);
}

QMatrix4x4 vidgfx_context_get_screen_view_mat(
	VidgfxContext *context)
{
	return context->getScreenViewMatrix();
}

void vidgfx_context_set_screen_proj_mat(
	VidgfxContext *context,
	const QMatrix4x4 &matrix)
{
	context->setScreenProjectionMatrix(matrix);
}

QMatrix4x4 vidgfx_context_get_screen_proj_mat(
	VidgfxContext *context)
{
	return context->getScreenProjectionMatrix();
}

void vidgfx_context_set_user_render_target(
	VidgfxContext *context,
	VidgfxTex *tex_a,
	VidgfxTex *tex_b)
{
	context->setUserRenderTarget(tex_a, tex_b);
}

VidgfxTex *vidgfx_context_get_user_render_target(
	VidgfxContext *context,
	int index)
{
	return context->getUserRenderTarget(index);
}

void vidgfx_context_set_user_render_target_viewport(
	VidgfxContext *context,
	const QRect &rect)
{
	context->setUserRenderTargetViewport(rect);
}

void vidgfx_context_set_user_render_target_viewport(
	VidgfxContext *context,
	const QSize &size)
{
	context->setUserRenderTargetViewport(size);
}

QRect vidgfx_context_get_user_render_target_viewport(
	VidgfxContext *context)
{
	return context->getUserRenderTargetViewport();
}

void vidgfx_context_set_resize_layer_rect(
	VidgfxContext *context,
	const QRectF &rect)
{
	context->setResizeLayerRect(rect);
}

QRectF vidgfx_context_get_resize_layer_rect(
	VidgfxContext *context)
{
	return context->getResizeLayerRect();
}

void vidgfx_context_set_rgb_nv16_px_size(
	VidgfxContext *context,
	const QPointF &size)
{
	context->setRgbNv16PxSize(size);
}

QPointF vidgfx_context_get_rgb_nv16_px_size(
	VidgfxContext *context)
{
	return context->getRgbNv16PxSize();
}

void vidgfx_context_set_tex_decal_mod_color(
	VidgfxContext *context,
	const QColor &color)
{
	context->setTexDecalModColor(color);
}

QColor vidgfx_context_get_tex_decal_mod_color(
	VidgfxContext *context)
{
	return context->getTexDecalModColor();
}

void vidgfx_context_set_tex_decal_effects(
	VidgfxContext *context,
	float gamma,
	float brightness,
	float contrast,
	float saturation)
{
	context->setTexDecalEffects(gamma, brightness, contrast, saturation);
}

bool vidgfx_context_set_tex_decal_effects_helper(
	VidgfxContext *context,
	float gamma,
	int brightness,
	int contrast,
	int saturation)
{
	return context->setTexDecalEffectsHelper(
		gamma, brightness, contrast, saturation);
}

const float *vidgfx_context_get_tex_decal_effects(
	VidgfxContext *context)
{
	return context->getTexDecalEffects();
}

bool vidgfx_context_dilute_img(
	VidgfxContext *context,
	QImage &img)
{
	return context->diluteImage(img);
}

//-----------------------------------------------------------------------------
// Interface

bool vidgfx_context_is_valid(
	VidgfxContext *context)
{
	if(context == NULL)
		return false;
	return context->isValid();
}

void vidgfx_context_flush(
	VidgfxContext *context)
{
	context->flush();
}

VidgfxVertbuf *vidgfx_context_new_vertbuf(
	VidgfxContext *context,
	int size)
{
	return context->createVertexBuffer(size);
}

void vidgfx_context_destroy_vertbuf(
	VidgfxContext *context,
	VidgfxVertbuf *buf)
{
	context->deleteVertexBuffer(buf);
}

VidgfxTex *vidgfx_context_new_tex(
	VidgfxContext *context,
	QImage img,
	bool writable,
	bool targetable)
{
	return context->createTexture(img, writable, targetable);
}

VidgfxTex *vidgfx_context_new_tex(
	VidgfxContext *context,
	const QSize &size,
	bool writable,
	bool targetable,
	bool use_bgra)
{
	return context->createTexture(size, writable, targetable, use_bgra);
}

VidgfxTex *vidgfx_context_new_tex(
	VidgfxContext *context,
	const QSize &size,
	VidgfxTex *same_format,
	bool writable,
	bool targetable)
{
	return context->createTexture(size, same_format, writable, targetable);
}

VidgfxTex *vidgfx_context_new_staging_tex(
	VidgfxContext *context,
	const QSize &size)
{
	return context->createStagingTexture(size);
}

void vidgfx_context_destroy_tex(
	VidgfxContext *context,
	VidgfxTex *tex)
{
	context->deleteTexture(tex);
}

bool vidgfx_context_copy_tex_data(
	VidgfxContext *context,
	VidgfxTex *dst,
	VidgfxTex *src,
	const QPoint &dst_pos,
	const QRect &src_rect)
{
	return context->copyTextureData(dst, src, dst_pos, src_rect);
}

void vidgfx_context_resize_screen_target(
	VidgfxContext *context,
	const QSize &new_size)
{
	context->resizeScreenTarget(new_size);
}

void vidgfx_context_resize_canvas_target(
	VidgfxContext *context,
	const QSize &new_size)
{
	context->resizeCanvasTarget(new_size);
}

void vidgfx_context_resize_scratch_target(
	VidgfxContext *context,
	const QSize &new_size)
{
	context->resizeScratchTarget(new_size);
}

void vidgfx_context_swap_screen_bufs(
	VidgfxContext *context)
{
	context->swapScreenBuffers();
}

VidgfxTex *vidgfx_context_get_target_tex(
	VidgfxContext *context,
	VidgfxRendTarget target)
{
	return context->getTargetTexture(target);
}

GfxRenderTarget vidgfx_context_get_next_scratch_target(
	VidgfxContext *context)
{
	return context->getNextScratchTarget();
}

QPointF vidgfx_context_get_scratch_target_to_tex_ratio(
	VidgfxContext *context)
{
	return context->getScratchTargetToTextureRatio();
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
	return context->prepareTexture(
		tex, size, filter, set_filter, px_size_out, bot_right_out);
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
	return context->prepareTexture(
		tex, crop_rect, size, filter, set_filter, px_size_out, top_left_out,
		bot_right_out);
}

VidgfxTex *vidgfx_context_convert_to_bgrx(
	VidgfxContext *context,
	VidgfxPixFormat format,
	VidgfxTex *plane_a,
	VidgfxTex *plane_b,
	VidgfxTex *plane_c)
{
	return context->convertToBgrx(format, plane_a, plane_b, plane_c);
}

void vidgfx_context_set_render_target(
	VidgfxContext *context,
	VidgfxRendTarget target)
{
	context->setRenderTarget(target);
}

void vidgfx_context_set_shader(
	VidgfxContext *context,
	VidgfxShader shader)
{
	context->setShader(shader);
}

void vidgfx_context_set_topology(
	VidgfxContext *context,
	VidgfxTopology topology)
{
	context->setTopology(topology);
}

void vidgfx_context_set_blending(
	VidgfxContext *context,
	VidgfxBlending blending)
{
	context->setBlending(blending);
}

void vidgfx_context_set_tex(
	VidgfxContext *context,
	VidgfxTex *tex_a,
	VidgfxTex *tex_b,
	VidgfxTex *tex_c)
{
	context->setTexture(tex_a, tex_b, tex_c);
}

void vidgfx_context_set_tex_filter(
	VidgfxContext *context,
	VidgfxFilter filter)
{
	context->setTextureFilter(filter);
}

void vidgfx_context_clear(
	VidgfxContext *context,
	const QColor &color)
{
	context->clear(color);
}

void vidgfx_context_draw_buf(
	VidgfxContext *context,
	VidgfxVertbuf *buf,
	int num_vertices,
	int start_vertex)
{
	context->drawBuffer(buf, num_vertices, start_vertex);
}

//-----------------------------------------------------------------------------
// Signals

void vidgfx_context_add_initialized_callback(
	VidgfxContext *context,
	VidgfxContextInitializedCallback *initialized,
	void *opaque)
{
	context->addInitializedCallback(initialized, opaque);
}

void vidgfx_context_remove_initialized_callback(
	VidgfxContext *context,
	VidgfxContextInitializedCallback *initialized,
	void *opaque)
{
	context->removeInitializedCallback(initialized, opaque);
}

void vidgfx_context_add_destroying_callback(
	VidgfxContext *context,
	VidgfxContextDestroyingCallback *destroying,
	void *opaque)
{
	context->addDestroyingCallback(destroying, opaque);
}

void vidgfx_context_remove_destroying_callback(
	VidgfxContext *context,
	VidgfxContextDestroyingCallback *destroying,
	void *opaque)
{
	context->removeDestroyingCallback(destroying, opaque);
}
