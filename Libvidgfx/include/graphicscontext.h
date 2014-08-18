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

#ifndef GRAPHICSCONTEXT_H
#define GRAPHICSCONTEXT_H

#include "libvidgfx.h"
#include <QtCore/QObject>
#include <QtGui/QColor>
#include <QtGui/QMatrix4x4>

class GraphicsContext;
class QRectF;
class QSize;

//=============================================================================
// TODO: It would be beter if this class behaved similarly to `Texture` by
// having `map()` and `unmap()` methods instead of keeping a copy of the data
// always in memory. However some functions such as `drawTextureRect()` rely
// on the data being in memory as well.
class LVG_EXPORT VertexBuffer
{
protected: // Members ---------------------------------------------------------
	float *	m_data;
	int		m_numFloats;
	int		m_numVerts;
	int		m_vertSize;
	bool	m_dirty;

protected: // Constructor/destructor ------------------------------------------
	VertexBuffer(int numFloats);
	virtual ~VertexBuffer();

public: // Methods ------------------------------------------------------------
	float *	getDataPtr() const;
	int		getNumFloats() const;

	void	setNumVerts(int numVerts);
	int		getNumVerts() const;
	void	setVertSize(int vertSize);
	int		getVertSize() const;

	void	setDirty(bool dirty = true);
	bool	isDirty() const;
};
//=============================================================================

inline float *VertexBuffer::getDataPtr() const
{
	return m_data;
}

inline int VertexBuffer::getNumFloats() const
{
	return m_numFloats;
}

inline void VertexBuffer::setNumVerts(int numVerts)
{
	m_numVerts = numVerts;
}

inline int VertexBuffer::getNumVerts() const
{
	return m_numVerts;
}

inline void VertexBuffer::setVertSize(int vertSize)
{
	m_vertSize = vertSize;
}

inline int VertexBuffer::getVertSize() const
{
	return m_vertSize;
}

inline void VertexBuffer::setDirty(bool dirty)
{
	m_dirty = dirty;
}

inline bool VertexBuffer::isDirty() const
{
	return m_dirty;
}

//=============================================================================
/// <summary>
/// A vertex buffer helper class for rendering rectangles that have a single
/// decal texture. It is up to the user to either call `deleteVertBuf()` or
/// delete the whole object when the graphics context is released.
/// </summary>
class LVG_EXPORT TexDecalVertBuf
{
private: // Constants ---------------------------------------------------------

	// Buffer information for `createTexDecalRect()` (1 vertex = 8 floats)
	static const int	ScrollRectNumVerts = 4 * 6; // 4 rects of 6 verts
	static const int	ScrollRectNumFloats = ScrollRectNumVerts * 8;
	static const int	ScrollRectBufSize = ScrollRectNumFloats * sizeof(float);

protected: // Members ---------------------------------------------------------
	GraphicsContext *	m_context;
	VertexBuffer *		m_vertBuf;
	bool				m_dirty;
	bool				m_hasScrolling;

	// Position
	QRectF	m_rect;

	// Scrolling
	QPointF	m_scrollOffset; // In `m_rect` space
	bool	m_roundOffset;

	// Texture UV
	QPointF	m_tlUv;
	QPointF	m_trUv;
	QPointF	m_blUv;
	QPointF	m_brUv;

public: // Constructor/destructor ---------------------------------------------
	TexDecalVertBuf(GraphicsContext *context = NULL);
	virtual ~TexDecalVertBuf();

public: // Methods ------------------------------------------------------------
	void			setContext(GraphicsContext *context);
	VertexBuffer *	getVertBuf(); // Applies settings
	GfxTopology		getTopology() const;
	void			deleteVertBuf();

	// Position
	void	setRect(const QRectF &rect);
	QRectF	getRect() const;

	// Scrolling
	void	scrollBy(const QPointF &delta);
	void	scrollBy(float xDelta, float yDelta);
	void	resetScrolling();
	void	setRoundOffset(bool round);
	bool	getRoundOffset() const;

	// Texture UV
	void	setTextureUv(
		const QPointF &topLeft, const QPointF &topRight,
		const QPointF &botLeft, const QPointF &botRight);
	void	setTextureUv(
		const QRectF &normRect, GfxOrientation orient = GfxUnchangedOrient);
	void	setTextureUv(
		const QPointF &topLeft, const QPointF &botRight,
		GfxOrientation orient = GfxUnchangedOrient);
	void	getTextureUv(
		QPointF *topLeft, QPointF *topRight, QPointF *botLeft,
		QPointF *botRight) const;

private:
	bool	createScrollTexDecalRect(VertexBuffer *outBuf);
	int		writeScrollRect(
		float *data, int i, const QRectF &rect, const QPointF &tlUv,
		const QPointF &trUv, const QPointF &blUv, const QPointF &brUv) const;
};
//=============================================================================

inline void TexDecalVertBuf::setContext(GraphicsContext *context)
{
	m_context = context;
}

inline QRectF TexDecalVertBuf::getRect() const
{
	return m_rect;
}

inline void TexDecalVertBuf::scrollBy(float xDelta, float yDelta)
{
	scrollBy(QPointF(xDelta, yDelta));
}

inline bool TexDecalVertBuf::getRoundOffset() const
{
	return m_roundOffset;
}

inline void TexDecalVertBuf::setTextureUv(
	const QPointF &topLeft, const QPointF &botRight, GfxOrientation orient)
{
	setTextureUv(QRectF(topLeft, botRight), orient);
}

//=============================================================================
class LVG_EXPORT Texture
{
protected: // Members ---------------------------------------------------------
	bool			m_isValid;
	GfxTextureFlags	m_flags;
	void *			m_mappedData;
	QSize			m_size;
	int				m_stride;

protected: // Constructor/destructor ------------------------------------------
	Texture(GfxTextureFlags flags, const QSize &size);
	virtual ~Texture();

public: // Methods ------------------------------------------------------------
	bool			isValid() const;
	bool			isMapped() const;
	void *			getDataPtr() const;
	int				getStride() const;

	bool			isWritable() const;
	bool			isTargetable() const;
	bool			isStaging() const;
	QSize			getSize() const;
	int				getWidth() const;
	int				getHeight() const;

	void			updateData(const QImage &img);

public: // Interface ----------------------------------------------------------
	virtual void *	map() = 0;
	virtual void	unmap() = 0;

	virtual bool	isSrgbHack() = 0;
};
//=============================================================================

inline bool Texture::isValid() const
{
	return m_isValid;
}

inline bool Texture::isMapped() const
{
	return m_mappedData != NULL;
}

inline void *Texture::getDataPtr() const
{
	return m_mappedData;
}

inline int Texture::getStride() const
{
	return m_stride;
}

inline bool Texture::isWritable() const
{
	return m_flags & GfxWritableFlag;
}

inline bool Texture::isTargetable() const
{
	return m_flags & GfxTargetableFlag;
}

inline bool Texture::isStaging() const
{
	return m_flags & GfxStagingFlag;
}

inline QSize Texture::getSize() const
{
	return m_size;
}

inline int Texture::getWidth() const
{
	return m_size.width();
}

inline int Texture::getHeight() const
{
	return m_size.height();
}

//=============================================================================
class LVG_EXPORT GraphicsContext : public QObject
{
	Q_OBJECT

public: // Constants ----------------------------------------------------------

	// The number of vertices required to represent one line
	static const int	NumVertsPerLine = 6;

	// The number of vertices required to represent one rectangle
	static const int	NumVertsPerRect = 4 * NumVertsPerLine;

	// Buffer information for `createSolidRect()` (1 vertex = 8 floats)
	static const int	SolidRectNumVerts = 4;
	static const int	SolidRectNumFloats = SolidRectNumVerts * 8;
	static const int	SolidRectBufSize = SolidRectNumFloats * sizeof(float);

	// Buffer information for `createSolidRectOutline()` (1 vertex = 8 floats)
	static const int	SolidRectOutlineNumVerts = NumVertsPerRect;
	static const int	SolidRectOutlineNumFloats = SolidRectOutlineNumVerts * 8;
	static const int	SolidRectOutlineBufSize = SolidRectOutlineNumFloats * sizeof(float);

	// Buffer information for `createTexDecalRect()` (1 vertex = 8 floats)
	static const int	TexDecalRectNumVerts = 4;
	static const int	TexDecalRectNumFloats = TexDecalRectNumVerts * 8;
	static const int	TexDecalRectBufSize = TexDecalRectNumFloats * sizeof(float);

	// Buffer information for `createResizeRect()` (1 vertex = 4 floats)
	static const int	ResizeRectNumVerts = 10 * NumVertsPerRect;
	static const int	ResizeRectNumFloats = ResizeRectNumVerts * 4;
	static const int	ResizeRectBufSize = ResizeRectNumFloats * sizeof(float);

protected: // Members ---------------------------------------------------------
	GfxRenderTarget	m_currentTarget;

	QMatrix4x4		m_screenViewMat;
	QMatrix4x4		m_screenProjMat;
	QMatrix4x4		m_canvasViewMat;
	QMatrix4x4		m_canvasProjMat;
	QMatrix4x4		m_scratchViewMat;
	QMatrix4x4		m_scratchProjMat;
	QMatrix4x4		m_userViewMat;
	QMatrix4x4		m_userProjMat;
	bool			m_cameraConstantsDirty;

	Texture *		m_userTargets[2];
	QRect			m_userTargetViewport;

	QRectF			m_resizeRect;
	bool			m_resizeConstantsDirty;

	QPointF			m_rgbNv16PxSize;
	bool			m_rgbNv16ConstantsDirty;

	QColor			m_texDecalModulate;
	float			m_texDecalEffects[4]; // Gamma, brightness, contrast, saturation
	bool			m_texDecalConstantsDirty;

public: // Static methods -----------------------------------------------------
	static bool		createSolidRect(
		VertexBuffer *outBuf, const QRectF &rect, const QColor &col);
	static bool		createSolidRect(
		VertexBuffer *outBuf, const QRectF &rect, const QColor &tlCol,
		const QColor &trCol, const QColor &blCol, const QColor &brCol);

	static bool		createSolidRectOutline(
		VertexBuffer *outBuf, const QRectF &rect, const QColor &col,
		const QPointF &halfWidth = QPointF(0.5f, 0.5f));
	static bool		createSolidRectOutline(
		VertexBuffer *outBuf, const QRectF &rect, const QColor &tlCol,
		const QColor &trCol, const QColor &blCol, const QColor &brCol,
		const QPointF &halfWidth = QPointF(0.5f, 0.5f));

	static bool		createTexDecalRect(
		VertexBuffer *outBuf, const QRectF &rect);
	static bool		createTexDecalRect(
		VertexBuffer *outBuf, const QRectF &rect, const QPointF &brUv);
	static bool		createTexDecalRect(
		VertexBuffer *outBuf, const QRectF &rect, const QPointF &tlUv,
		const QPointF &trUv, const QPointF &blUv, const QPointF &brUv);

	static bool		createResizeRect(
		VertexBuffer *outBuf, const QRectF &rect, float handleSize,
		const QPointF &halfWidth = QPointF(0.5f, 0.5f));

	// Helpers
	static quint32	nextPowTwo(quint32 n);

public: // Constructor/destructor ---------------------------------------------
	GraphicsContext();
	virtual ~GraphicsContext();

public: // Methods ------------------------------------------------------------
	void			setViewMatrix(const QMatrix4x4 &matrix);
	QMatrix4x4		getViewMatrix() const;
	void			setProjectionMatrix(const QMatrix4x4 &matrix);
	QMatrix4x4		getProjectionMatrix() const;

	void			setScreenViewMatrix(const QMatrix4x4 &matrix);
	QMatrix4x4		getScreenViewMatrix() const;
	void			setScreenProjectionMatrix(const QMatrix4x4 &matrix);
	QMatrix4x4		getScreenProjectionMatrix() const;

	void			setUserRenderTarget(Texture *texA, Texture *texB = NULL);
	Texture *		getUserRenderTarget(int index) const;
	void			setUserRenderTargetViewport(const QRect &rect);
	void			setUserRenderTargetViewport(const QSize &size);
	QRect			getUserRenderTargetViewport() const;

	void			setResizeLayerRect(const QRectF &rect);
	QRectF			getResizeLayerRect() const;

	void			setRgbNv16PxSize(const QPointF &size);
	QPointF			getRgbNv16PxSize() const;

	void			setTexDecalModColor(const QColor &color);
	QColor			getTexDecalModColor() const;

	void			setTexDecalEffects(
		float gamma, float brightness, float contrast, float saturation);
	bool			setTexDecalEffectsHelper(
		float gamma, int brightness, int contrast, int saturation);
	const float *	getTexDecalEffects() const;

	bool			diluteImage(QImage &img) const;

public: // Interface ----------------------------------------------------------
	virtual bool	isValid() const = 0;
	virtual void	flush() = 0;

	// Buffers
	virtual VertexBuffer *	createVertexBuffer(int size) = 0;
	virtual void			deleteVertexBuffer(VertexBuffer *buf) = 0;
	virtual Texture *		createTexture(
		QImage img, bool writable = false, bool targetable = false) = 0;
	virtual Texture *		createTexture(
		const QSize &size, bool writable = false, bool targetable = false,
		bool useBgra = false) = 0;
	virtual Texture *		createTexture(
		const QSize &size, Texture *sameFormat, bool writable = false,
		bool targetable = false) = 0;
	virtual Texture *		createStagingTexture(const QSize &size) = 0;
	virtual void			deleteTexture(Texture *tex) = 0;
	virtual bool			copyTextureData(
		Texture *dst, Texture *src, const QPoint &dstPos,
		const QRect &srcRect) = 0;

	// Render targets
	virtual void			resizeScreenTarget(const QSize &newSize) = 0;
	virtual void			resizeCanvasTarget(const QSize &newSize) = 0;
	virtual void			resizeScratchTarget(const QSize &newSize) = 0;
	virtual void			swapScreenBuffers() = 0;
	virtual	Texture *		getTargetTexture(GfxRenderTarget target) = 0;
	virtual GfxRenderTarget	getNextScratchTarget() = 0;
	virtual QPointF			getScratchTargetToTextureRatio() = 0;

	// Advanced rendering
	virtual Texture *	prepareTexture(
		Texture *tex, const QSize &size, GfxFilter filter, bool setFilter,
		QPointF &pxSizeOut, QPointF &botRightOut) = 0;
	virtual Texture *	prepareTexture(
		Texture *tex, const QRect &cropRect, const QSize &size,
		GfxFilter filter, bool setFilter, QPointF &pxSizeOut,
		QPointF &topLeftOut, QPointF &botRightOut) = 0;
	virtual Texture *	convertToBgrx(
		GfxPixelFormat format, Texture *planeA, Texture *planeB,
		Texture *planeC) = 0;

	// Drawing
	virtual void		setRenderTarget(GfxRenderTarget target) = 0;
	virtual void		setShader(GfxShader shader) = 0;
	virtual void		setTopology(GfxTopology topology) = 0;
	virtual void		setBlending(GfxBlending blending) = 0;
	virtual void		setTexture(
		Texture *texA, Texture *texB = NULL, Texture *texC = NULL) = 0;
	virtual void		setTextureFilter(GfxFilter filter) = 0;
	virtual void		clear(const QColor &color) = 0;
	virtual void		drawBuffer(
		VertexBuffer *buf, int numVertices = -1, int startVertex = 0) = 0;

Q_SIGNALS: // Signals ---------------------------------------------------------
	void	initialized(GraphicsContext *gfx);
	void	destroying(GraphicsContext *gfx);
};
//=============================================================================

inline Texture *GraphicsContext::getUserRenderTarget(int index) const
{
	if(index < 0 || index > 1)
		return NULL; // Invalid index
	return m_userTargets[index];
}

inline void GraphicsContext::setUserRenderTargetViewport(const QSize &size)
{
	setUserRenderTargetViewport(QRect(QPoint(0, 0), size));
}

inline QRect GraphicsContext::getUserRenderTargetViewport() const
{
	return m_userTargetViewport;
}

inline QRectF GraphicsContext::getResizeLayerRect() const
{
	return m_resizeRect;
}

inline QPointF GraphicsContext::getRgbNv16PxSize() const
{
	return m_rgbNv16PxSize;
}

inline QColor GraphicsContext::getTexDecalModColor() const
{
	return m_texDecalModulate;
}

inline const float *GraphicsContext::getTexDecalEffects() const
{
	return m_texDecalEffects;
}

#endif // GRAPHICSCONTEXT_H
