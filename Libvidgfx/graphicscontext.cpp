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

#include "include/graphicscontext.h"
#include "include/gfxlog.h"
#include <QtGui/QImage>
#include <QtGui/QVector2D>

const QString LOG_CAT = QStringLiteral("Gfx");

//=============================================================================
// Helpers

/// <summary>
/// Interpolates between `a` and `b` by factor `t`.
/// </summary>
double dblLerp(double a, double b, double t)
{
	return a + t * (b - a);
}

/// <summary>
/// Loops `num` within the range [0..`max`) (0.0 to just below `max`).
/// Negative numbers are not inverted, i.e. "-0.1"  becomes "`max` - 0.1".
/// </summary>
double dblRepeat(double num, double max)
{
	double tmp;
	tmp = num - (double)((int)(num / max)) * max;
	if(tmp < 0.0)
		return max + tmp;
	return tmp;
}

int triListLine(
	float *data, const QVector2D &start, const QVector2D &end,
	const QPointF &halfWidth, int extraDataPerVert = 0)
{
	int i = 0;

	// Create perpendicular vector of AB with a length of `halfWidth` where
	// `halfWidth` takes into account the viewport's aspect ratio.
	QVector2D delta = start - end;
	QVector2D perp = QVector2D(-delta.y(), delta.x());
	if(!perp.isNull())
		perp.normalize();
	perp *= QVector2D(halfWidth);

	// NOTE: Comments below assume that the line is horizontal with `a` to the
	// left and the perpendicular vector points downwards

	// Ensure that our generated triangles have a clockwise winding
	if(perp.x() * delta.y() - perp.y() * delta.x() >= 0.0f)
		perp = -perp;

	// Calculate quad vertices
	const QVector2D tl = start - perp; // Top-left
	const QVector2D bl = start + perp; // Bottom-left
	const QVector2D tr = end - perp; // Top-right
	const QVector2D br = end + perp; // Bottom-right

	//-------------------------------------------------------------------------
	// Triangle 1

	// Top-left
	data[i++] = tl.x();
	data[i++] = tl.y();
	data[i++] = 0.0f;
	data[i++] = 1.0f;
	i += extraDataPerVert;

	// Top-right
	data[i++] = tr.x();
	data[i++] = tr.y();
	data[i++] = 0.0f;
	data[i++] = 1.0f;
	i += extraDataPerVert;

	// Bottom-left
	data[i++] = bl.x();
	data[i++] = bl.y();
	data[i++] = 0.0f;
	data[i++] = 1.0f;
	i += extraDataPerVert;

	//-------------------------------------------------------------------------
	// Triangle 2

	// Bottom-left
	data[i++] = bl.x();
	data[i++] = bl.y();
	data[i++] = 0.0f;
	data[i++] = 1.0f;
	i += extraDataPerVert;

	// Top-right
	data[i++] = tr.x();
	data[i++] = tr.y();
	data[i++] = 0.0f;
	data[i++] = 1.0f;
	i += extraDataPerVert;

	// Bottom-right
	data[i++] = br.x();
	data[i++] = br.y();
	data[i++] = 0.0f;
	data[i++] = 1.0f;
	i += extraDataPerVert;

	//-------------------------------------------------------------------------

	return i;
}

int rectOutline(float *data, const QRectF &rect, const QPointF &halfWidth)
{
	int i = 0;

	// Top line
	i += triListLine(&data[i],
		QVector2D(rect.topLeft() + QPointF(halfWidth.x(), 0.0f)),
		QVector2D(rect.topRight() - QPointF(halfWidth.x(), 0.0f)),
		halfWidth);

	// Bottom line
	i += triListLine(&data[i],
		QVector2D(rect.bottomLeft() + QPointF(halfWidth.x(), 0.0f)),
		QVector2D(rect.bottomRight() - QPointF(halfWidth.x(), 0.0f)),
		halfWidth);

	// Left line
	i += triListLine(&data[i],
		QVector2D(rect.topLeft() - QPointF(0.0f, halfWidth.y())),
		QVector2D(rect.bottomLeft() + QPointF(0.0f, halfWidth.y())),
		halfWidth);

	// Right line
	i += triListLine(&data[i],
		QVector2D(rect.topRight() - QPointF(0.0f, halfWidth.y())),
		QVector2D(rect.bottomRight() + QPointF(0.0f, halfWidth.y())),
		halfWidth);

	return i;
}

int rectOutlineColor(
	float *data, const QRectF &rect, const QPointF &halfWidth,
	const QColor &tlCol, const QColor &trCol, const QColor &blCol,
	const QColor &brCol)
{
	int i = 0;
	int off = 0;

	// Line triangle order = Start, End, Start, Start, End, End
#define ADD_VERT_COLOR(off, vert, col) \
	data[(off)+(vert)*8+4] = (col).redF(); \
	data[(off)+(vert)*8+5] = (col).greenF(); \
	data[(off)+(vert)*8+6] = (col).blueF(); \
	data[(off)+(vert)*8+7] = (col).alphaF()
#define ADD_LINE_COLOR(off, startCol, endCol) \
	ADD_VERT_COLOR((off), 0, (startCol)); \
	ADD_VERT_COLOR((off), 1, (endCol)); \
	ADD_VERT_COLOR((off), 2, (startCol)); \
	ADD_VERT_COLOR((off), 3, (startCol)); \
	ADD_VERT_COLOR((off), 4, (endCol)); \
	ADD_VERT_COLOR((off), 5, (endCol))

	// Top line
	off = triListLine(&data[i],
		QVector2D(rect.topLeft() + QPointF(halfWidth.x(), 0.0f)),
		QVector2D(rect.topRight() - QPointF(halfWidth.x(), 0.0f)),
		halfWidth, 4);
	ADD_LINE_COLOR(i, tlCol, trCol);
	i += off;

	// Bottom line
	off = triListLine(&data[i],
		QVector2D(rect.bottomLeft() + QPointF(halfWidth.x(), 0.0f)),
		QVector2D(rect.bottomRight() - QPointF(halfWidth.x(), 0.0f)),
		halfWidth, 4);
	ADD_LINE_COLOR(i, blCol, brCol);
	i += off;

	// Left line
	off = triListLine(&data[i],
		QVector2D(rect.topLeft() - QPointF(0.0f, halfWidth.y())),
		QVector2D(rect.bottomLeft() + QPointF(0.0f, halfWidth.y())),
		halfWidth, 4);
	ADD_LINE_COLOR(i, tlCol, blCol);
	i += off;

	// Right line
	off = triListLine(&data[i],
		QVector2D(rect.topRight() - QPointF(0.0f, halfWidth.y())),
		QVector2D(rect.bottomRight() + QPointF(0.0f, halfWidth.y())),
		halfWidth, 4);
	ADD_LINE_COLOR(i, trCol, brCol);
	i += off;

	return i;
}

/// <summary>
/// Copies the nearby colour information to the specified pixel.
/// </summary>
/// <returns>True if the pixel was modified</returns>
bool dilutePixel(QImage &imgIn, QImage &imgOut, int x, int y, int dMax)
{
	// WARNING: We do a VERY quick and nasty "nearest pixel" algorithm here
	// that is nowhere near ideal.
	const int w = imgIn.width();
	const int h = imgIn.height();
	bool found = false;
	for(int d = 1; d <= dMax; d++) {
		int xStart = x - d;
		int xEnd = x + d;
		int yStart = y - d;
		int yEnd = y + d;

		if(yStart >= 0) { // Top
			xStart = qMax(xStart, 0);
			xEnd = qMin(xEnd, w - 1);
			for(int s = xStart; s <= xEnd; s++) {
				QRgb pix = imgIn.pixel(s, yStart);
				if(qAlpha(pix) > 0) {
					imgOut.setPixel(x, y, pix & RGB_MASK);
					found = true;
					break;
				}
			}
		}
		if(xStart >= 0) { // Left
			yStart = qMax(yStart, 0);
			yEnd = qMin(yEnd, h - 1);
			for(int t = yStart; t <= yEnd; t++) {
				QRgb pix = imgIn.pixel(xStart, t);
				if(qAlpha(pix) > 0) {
					imgOut.setPixel(x, y, pix & RGB_MASK);
					found = true;
					break;
				}
			}
		}
		if(xEnd < w) { // Right
			yStart = qMax(yStart, 0);
			yEnd = qMin(yEnd, h - 1);
			for(int t = yStart; t <= yEnd; t++) {
				QRgb pix = imgIn.pixel(xEnd, t);
				if(qAlpha(pix) > 0) {
					imgOut.setPixel(x, y, pix & RGB_MASK);
					found = true;
					break;
				}
			}
		}
		if(yEnd < h) { // Bottom
			xStart = qMax(xStart, 0);
			xEnd = qMin(xEnd, w - 1);
			for(int s = xStart; s <= xEnd; s++) {
				QRgb pix = imgIn.pixel(s, yEnd);
				if(qAlpha(pix) > 0) {
					imgOut.setPixel(x, y, pix & RGB_MASK);
					found = true;
					break;
				}
			}
		}

		if(found)
			break;
	}

	return found;
}

//=============================================================================
// VertexBuffer class

/// <summary>
/// WARNING: Create with `GraphicsContext::createVertexBuffer()` only!
/// </summary>
VertexBuffer::VertexBuffer(int numFloats)
	: m_data(new float[numFloats])
	, m_numFloats(numFloats)
	, m_numVerts(0)
	, m_vertSize(0)
	, m_dirty(false)
{
	memset(m_data, 0, numFloats * sizeof(float));
}

VertexBuffer::~VertexBuffer()
{
	delete[] m_data;
}

//=============================================================================
// TexDecalVertBuf class

TexDecalVertBuf::TexDecalVertBuf(GraphicsContext *context)
	: m_context(context)
	, m_vertBuf(NULL)
	, m_dirty(true)
	, m_hasScrolling(false)

	// Position
	, m_rect()

	// Scrolling
	, m_scrollOffset()
	, m_roundOffset(true)

	// Texture UV
	, m_tlUv(0.0f, 0.0f)
	, m_trUv(1.0f, 0.0f)
	, m_blUv(0.0f, 1.0f)
	, m_brUv(1.0f, 1.0f)
{
}

TexDecalVertBuf::~TexDecalVertBuf()
{
	if(m_vertBuf != NULL)
		deleteVertBuf();
}

/// <summary>
/// Retrieves the vertex buffer, creating and/or updating it if required.
/// </summary>
VertexBuffer *TexDecalVertBuf::getVertBuf()
{
	if(!m_dirty)
		return m_vertBuf;
	if(m_context == NULL || !m_context->isValid())
		return NULL; // No context operations can be done

	// Create the vertex buffer object if it doesn't already exist
	if(m_vertBuf == NULL) {
		int size = GraphicsContext::TexDecalRectBufSize;
		if(m_hasScrolling)
			size = ScrollRectBufSize;
		m_vertBuf = m_context->createVertexBuffer(size);
		if(m_vertBuf == NULL)
			return NULL; // Failed to create vertex buffer
	}

	// Update the vertex buffer
	if(m_hasScrolling)
		createScrollTexDecalRect(m_vertBuf);
	else {
		m_context->createTexDecalRect(
			m_vertBuf, m_rect, m_tlUv, m_trUv, m_blUv, m_brUv);
	}

	m_dirty = false;
	return m_vertBuf;
}

/// <summary>
/// Returns the topology that the vertex buffer should be rendered with.
/// </summary>
GfxTopology TexDecalVertBuf::getTopology() const
{
	if(m_hasScrolling)
		return GfxTriangleListTopology;
	return GfxTriangleStripTopology;
}

void TexDecalVertBuf::deleteVertBuf()
{
	if(m_vertBuf == NULL)
		return;
	if(m_context == NULL || !m_context->isValid())
		return;
	m_context->deleteVertexBuffer(m_vertBuf);
	m_vertBuf = NULL;
}

void TexDecalVertBuf::setRect(const QRectF &rect)
{
	if(m_rect == rect)
		return; // Nothing to do
	m_rect = rect;
	m_dirty = true;
}

void TexDecalVertBuf::scrollBy(const QPointF &delta)
{
	if(delta.isNull())
		return; // Nothing to do
	if(!m_hasScrolling)
		deleteVertBuf(); // We must enlarge the buffer size
	m_hasScrolling = true;
	m_scrollOffset += delta;

	// Keep within a sane range so we don't get resolution errors after a long
	// period of time
	m_scrollOffset.setX(dblRepeat(m_scrollOffset.x(), 1.0));
	m_scrollOffset.setY(dblRepeat(m_scrollOffset.y(), 1.0));

	m_dirty = true;
}

void TexDecalVertBuf::resetScrolling()
{
	if(m_scrollOffset.isNull())
		return; // Nothing to do
	if(m_hasScrolling)
		deleteVertBuf(); // We can shrink the buffer size
	m_hasScrolling = false;
	m_scrollOffset = QPointF(0.0f, 0.0f);
	m_dirty = true;
}

/// <summary>
/// Enable or disable rounding of texture coordinates in UV space so that the
/// texture's texels remain in the same position when scrolling. Used to remove
/// the shimmering effect due to interpolation. Enabled by default.
/// </summary>
void TexDecalVertBuf::setRoundOffset(bool round)
{
	if(m_roundOffset == round)
		return; // Nothing to do
	m_roundOffset = round;
	m_dirty = true;
}

void TexDecalVertBuf::setTextureUv(
	const QPointF &topLeft, const QPointF &topRight, const QPointF &botLeft,
	const QPointF &botRight)
{
	if(m_tlUv == topLeft && m_trUv == topRight && m_blUv == botLeft &&
		m_brUv == botRight)
	{
		return; // Nothing to do
	}
	m_tlUv = topLeft;
	m_trUv = topRight;
	m_blUv = botLeft;
	m_brUv = botRight;
	m_dirty = true;
}

void TexDecalVertBuf::setTextureUv(
	const QRectF &normRect, GfxOrientation orient)
{
	QPointF rectTl = normRect.topLeft();
	QPointF rectBr = normRect.bottomRight();

	QPointF tl, tr, bl, br;
	switch(orient) {
	default:
	case GfxUnchangedOrient:
		tl = rectTl;
		tr = QPointF(rectBr.x(), rectTl.y());
		bl = QPointF(rectTl.x(), rectBr.y());
		br = rectBr;
		break;
	case GfxFlippedOrient:
		tl = QPointF(rectTl.x(), rectBr.y());
		tr = rectBr;
		bl = rectTl;
		br = QPointF(rectBr.x(), rectTl.y());
		break;
	case GfxMirroredOrient:
		tl = QPointF(rectBr.x(), rectTl.y());
		tr = rectTl;
		bl = rectBr;
		br = QPointF(rectTl.x(), rectBr.y());
		break;
	case GfxFlippedMirroredOrient:
		tl = rectBr;
		tr = QPointF(rectTl.x(), rectBr.y());
		bl = QPointF(rectBr.x(), rectTl.y());
		br = rectTl;
		break;
	}

	setTextureUv(tl, tr, bl, br);
}

void TexDecalVertBuf::getTextureUv(
	QPointF *topLeft, QPointF *topRight, QPointF *botLeft,
	QPointF *botRight) const
{
	if(topLeft != NULL)
		*topLeft = m_tlUv;
	if(topRight != NULL)
		*topRight = m_trUv;
	if(botLeft != NULL)
		*botLeft = m_blUv;
	if(botRight != NULL)
		*botRight = m_brUv;
}

bool TexDecalVertBuf::createScrollTexDecalRect(VertexBuffer *outBuf)
{
	if(outBuf == NULL)
		return false;
	outBuf->setNumVerts(0);
	if(outBuf->getNumFloats() < ScrollRectNumFloats)
		return false;
	outBuf->setNumVerts(ScrollRectNumVerts);

	// Shader expects vertex format: X, Y, Z, -, U, V, -, -
	outBuf->setVertSize(8);
	float *data = outBuf->getDataPtr();
	int i = 0;

	// Shared variables
	QRectF rect;
	QPointF tlUv, trUv, blUv, brUv;
	qreal xLerp = m_scrollOffset.x();
	qreal yLerp = m_scrollOffset.y();
	if(m_roundOffset) {
		// We assume the texture UV is orthogonal
		if(!m_rect.size().isEmpty()) {
			QSizeF invRectSize(1.0 / m_rect.width(), 1.0 / m_rect.height());
			xLerp =
				(qreal)qRound(xLerp * m_rect.width()) * invRectSize.width();
			yLerp =
				(qreal)qRound(yLerp * m_rect.height()) * invRectSize.height();
		}
	}

	//-------------------------------------------------------------------------
	// Write rectangles to the buffer

	// Top-left rectangle
	rect = m_rect.adjusted(
		0.0,
		0.0,
		m_rect.width() * xLerp - m_rect.width(),
		m_rect.height() * yLerp - m_rect.height());
	tlUv = m_tlUv;
	trUv = m_trUv;
	blUv = m_blUv;
	brUv = m_brUv;
	tlUv.setX(dblLerp(trUv.x(), tlUv.x(), xLerp));
	blUv.setX(dblLerp(brUv.x(), blUv.x(), xLerp));
	tlUv.setY(dblLerp(blUv.y(), tlUv.y(), yLerp));
	trUv.setY(dblLerp(brUv.y(), trUv.y(), yLerp));
	i = writeScrollRect(data, i, rect, tlUv, trUv, blUv, brUv);

	// Top-right rectangle
	rect = m_rect.adjusted(
		m_rect.width() * xLerp,
		0.0,
		0.0,
		m_rect.height() * yLerp - m_rect.height());
	tlUv = m_tlUv;
	trUv = m_trUv;
	blUv = m_blUv;
	brUv = m_brUv;
	trUv.setX(dblLerp(trUv.x(), tlUv.x(), xLerp));
	brUv.setX(dblLerp(brUv.x(), blUv.x(), xLerp));
	tlUv.setY(dblLerp(blUv.y(), tlUv.y(), yLerp));
	trUv.setY(dblLerp(brUv.y(), trUv.y(), yLerp));
	i = writeScrollRect(data, i, rect, tlUv, trUv, blUv, brUv);

	// Bottom-left rectangle
	rect = m_rect.adjusted(
		0.0,
		m_rect.height() * yLerp,
		m_rect.width() * xLerp - m_rect.width(),
		0.0);
	tlUv = m_tlUv;
	trUv = m_trUv;
	blUv = m_blUv;
	brUv = m_brUv;
	tlUv.setX(dblLerp(trUv.x(), tlUv.x(), xLerp));
	blUv.setX(dblLerp(brUv.x(), blUv.x(), xLerp));
	blUv.setY(dblLerp(blUv.y(), tlUv.y(), yLerp));
	brUv.setY(dblLerp(brUv.y(), trUv.y(), yLerp));
	i = writeScrollRect(data, i, rect, tlUv, trUv, blUv, brUv);

	// Bottom-right rectangle
	rect = m_rect.adjusted(
		m_rect.width() * xLerp,
		m_rect.height() * yLerp,
		0.0,
		0.0);
	tlUv = m_tlUv;
	trUv = m_trUv;
	blUv = m_blUv;
	brUv = m_brUv;
	trUv.setX(dblLerp(trUv.x(), tlUv.x(), xLerp));
	brUv.setX(dblLerp(brUv.x(), blUv.x(), xLerp));
	blUv.setY(dblLerp(blUv.y(), tlUv.y(), yLerp));
	brUv.setY(dblLerp(brUv.y(), trUv.y(), yLerp));
	i = writeScrollRect(data, i, rect, tlUv, trUv, blUv, brUv);

	//-------------------------------------------------------------------------

	outBuf->setDirty(true);
	return true;
}

int TexDecalVertBuf::writeScrollRect(
	float *data, int i, const QRectF &rect, const QPointF &tlUv,
	const QPointF &trUv, const QPointF &blUv, const QPointF &brUv) const
{
	//-------------------------------------------------------------------------
	// Triangle 1

	// Top-left
	data[i++] = rect.left();
	data[i++] = rect.top();
	data[i++] = 0.0f;
	data[i++] = 1.0f;
	data[i++] = tlUv.x();
	data[i++] = tlUv.y();
	data[i++] = 0.0f;
	data[i++] = 0.0f;

	// Top-right
	data[i++] = rect.right();
	data[i++] = rect.top();
	data[i++] = 0.0f;
	data[i++] = 1.0f;
	data[i++] = trUv.x();
	data[i++] = trUv.y();
	data[i++] = 0.0f;
	data[i++] = 0.0f;

	// Bottom-left
	data[i++] = rect.left();
	data[i++] = rect.bottom();
	data[i++] = 0.0f;
	data[i++] = 1.0f;
	data[i++] = blUv.x();
	data[i++] = blUv.y();
	data[i++] = 0.0f;
	data[i++] = 0.0f;

	//-------------------------------------------------------------------------
	// Triangle 2

	// Bottom-left
	data[i++] = rect.left();
	data[i++] = rect.bottom();
	data[i++] = 0.0f;
	data[i++] = 1.0f;
	data[i++] = blUv.x();
	data[i++] = blUv.y();
	data[i++] = 0.0f;
	data[i++] = 0.0f;

	// Top-right
	data[i++] = rect.right();
	data[i++] = rect.top();
	data[i++] = 0.0f;
	data[i++] = 1.0f;
	data[i++] = trUv.x();
	data[i++] = trUv.y();
	data[i++] = 0.0f;
	data[i++] = 0.0f;

	// Bottom-right
	data[i++] = rect.right();
	data[i++] = rect.bottom();
	data[i++] = 0.0f;
	data[i++] = 1.0f;
	data[i++] = brUv.x();
	data[i++] = brUv.y();
	data[i++] = 0.0f;
	data[i++] = 0.0f;

	//-------------------------------------------------------------------------

	return i;
}

//=============================================================================
// Texture class

/// <summary>
/// WARNING: Create with `GraphicsContext::createTexture()` only!
/// </summary>
Texture::Texture(GfxTextureFlags flags, const QSize &size)
	: m_flags(flags)
	, m_mappedData(NULL)
	, m_size(size)
	, m_stride(0)
	, m_isValid(false)
{
}

Texture::~Texture()
{
}

/// <summary>
/// Maps the texture and copies the pixel data from the `QImage` to it if the
/// texture is writable.
/// </summary>
void Texture::updateData(const QImage &img)
{
	if(!isWritable() || img.isNull())
		return;
	void *data = map();
	if(data == NULL)
		return;

	// TODO: We always assume each pixel is 32-bit
	// TODO: There is a chance of overflowing if the image size changes
	//       between frames
	// TODO: Use the same copy function as the scaler?
	int stride = getStride();
	int texLen = stride * getSize().height();
	if(stride == img.width() * 4) {
		// Stride matches, fast copy
		texLen = qMin(texLen, img.byteCount());
		memcpy(data, img.constBits(), texLen);
	} else {
		// Stride doesn't match, copy each line separately
		texLen = qMin(texLen, img.width() * 4);
		for(int i = 0; i < img.height(); i++) {
			char *cData = reinterpret_cast<char *>(data);
			memcpy(cData + i * stride, img.constScanLine(i), texLen);
		}
	}

	unmap();
}

//=============================================================================
// GraphicsContext class

GraphicsContext::GraphicsContext()
	: QObject()
	, m_currentTarget(GfxScreenTarget)
	, m_screenViewMat()
	, m_screenProjMat()
	, m_canvasViewMat()
	, m_canvasProjMat()
	, m_scratchViewMat()
	, m_scratchProjMat()
	, m_userViewMat()
	, m_userProjMat()
	, m_cameraConstantsDirty(false)
	//, m_userTargets() // Done below
	, m_userTargetViewport(0, 0, 0, 0)
	, m_resizeRect()
	, m_resizeConstantsDirty(false)
	, m_rgbNv16PxSize(0.0f, 0.0f)
	, m_rgbNv16ConstantsDirty(false)
	, m_texDecalModulate(255, 255, 255, 255)
	//, m_texDecalEffects() // Done below
	, m_texDecalConstantsDirty(false)
	, m_initializedCallbackList()
	, m_destroyingCallbackList()
{
	m_userTargets[0] = NULL;
	m_userTargets[1] = NULL;

	m_texDecalEffects[0] = 1.0f; // Gamma
	m_texDecalEffects[1] = 0.0f; // Brightness
	m_texDecalEffects[2] = 1.0f; // Contrast
	m_texDecalEffects[3] = 1.0f; // Saturation
}

GraphicsContext::~GraphicsContext()
{
}

/// <summary>
/// Fills a `VertexBuffer` with the required data to draw a filled rectangle
/// with a single solid colour. Designed to be rendered with the
/// `TriangleStrip` topology.
/// </summary>
/// <returns>True if the buffer is valid.</returns>
bool GraphicsContext::createSolidRect(
	VertexBuffer *outBuf, const QRectF &rect, const QColor &col)
{
	return createSolidRect(outBuf, rect, col, col, col, col);
}

/// <summary>
/// Fills a `VertexBuffer` with the required data to draw a filled rectangle
/// with a different solid colour for each vertex. Designed to be rendered with
/// the `TriangleStrip` topology.
/// </summary>
/// <returns>True if the buffer is valid.</returns>
bool GraphicsContext::createSolidRect(
	VertexBuffer *outBuf, const QRectF &rect, const QColor &tlCol,
	const QColor &trCol, const QColor &blCol, const QColor &brCol)
{
	if(outBuf == NULL)
		return false;
	outBuf->setNumVerts(0);
	if(outBuf->getNumFloats() < SolidRectNumFloats)
		return false;
	outBuf->setNumVerts(SolidRectNumVerts);

	// Shader expects vertex format: X, Y, Z, -, R, G, B, A
	outBuf->setVertSize(8);
	float *data = outBuf->getDataPtr();
	int i = 0;

	// Top-left
	data[i++] = rect.left();
	data[i++] = rect.top();
	data[i++] = 0.0f;
	data[i++] = 1.0f;
	data[i++] = tlCol.redF();
	data[i++] = tlCol.greenF();
	data[i++] = tlCol.blueF();
	data[i++] = tlCol.alphaF();

	// Top-right
	data[i++] = rect.right();
	data[i++] = rect.top();
	data[i++] = 0.0f;
	data[i++] = 1.0f;
	data[i++] = trCol.redF();
	data[i++] = trCol.greenF();
	data[i++] = trCol.blueF();
	data[i++] = trCol.alphaF();

	// Bottom-left
	data[i++] = rect.left();
	data[i++] = rect.bottom();
	data[i++] = 0.0f;
	data[i++] = 1.0f;
	data[i++] = blCol.redF();
	data[i++] = blCol.greenF();
	data[i++] = blCol.blueF();
	data[i++] = blCol.alphaF();

	// Bottom-right
	data[i++] = rect.right();
	data[i++] = rect.bottom();
	data[i++] = 0.0f;
	data[i++] = 1.0f;
	data[i++] = brCol.redF();
	data[i++] = brCol.greenF();
	data[i++] = brCol.blueF();
	data[i++] = brCol.alphaF();

	outBuf->setDirty(true);
	return true;
}

/// <summary>
/// Fills a `VertexBuffer` with the required data to draw a rectangle outline
/// with a single solid colour. Designed to be rendered with
/// `TriangleStripTopology`.
/// </summary>
/// <returns>True if the buffer is valid.</returns>
bool GraphicsContext::createSolidRectOutline(
	VertexBuffer *outBuf, const QRectF &rect, const QColor &col,
	const QPointF &halfWidth)
{
	return createSolidRectOutline(outBuf, rect, col, col, col, col, halfWidth);
}

/// <summary>
/// Fills a `VertexBuffer` with the required data to draw a rectangle outline
/// with a different solid colour for each vertex. Designed to be rendered with
/// `TriangleStripTopology`.
/// </summary>
/// <returns>True if the buffer is valid.</returns>
bool GraphicsContext::createSolidRectOutline(
	VertexBuffer *outBuf, const QRectF &rect, const QColor &tlCol,
	const QColor &trCol, const QColor &blCol, const QColor &brCol,
	const QPointF &halfWidth)
{
	if(outBuf == NULL)
		return false;
	outBuf->setNumVerts(0);
	if(outBuf->getNumFloats() < SolidRectOutlineNumFloats)
		return false;
	outBuf->setNumVerts(SolidRectOutlineNumVerts);

	// Shader expects vertex format: X, Y, Z, -, R, G, B, A
	outBuf->setVertSize(8);
	float *data = outBuf->getDataPtr();
	int i = 0;

	// Add rectangle
	i += rectOutlineColor(
		&data[i], rect, halfWidth, tlCol, trCol, blCol, brCol);

	outBuf->setDirty(true);
	return true;
}

/// <summary>
/// Assumes that the top-left UV coordinate is (0, 0) and the bottom-right is
/// (1, 1).
/// </summary>
bool GraphicsContext::createTexDecalRect(
	VertexBuffer *outBuf, const QRectF &rect)
{
	return createTexDecalRect(outBuf, rect, QPointF(1.0f, 1.0f));
}

/// <summary>
/// Assumes that the top-left UV coordinate is (0, 0).
/// </summary>
bool GraphicsContext::createTexDecalRect(
	VertexBuffer *outBuf, const QRectF &rect, const QPointF &brUv)
{
	return createTexDecalRect(
		outBuf, rect, QPointF(0.0f, 0.0f), QPointF(brUv.x(), 0.0f),
		QPointF(0.0f, brUv.y()), brUv);
}

bool GraphicsContext::createTexDecalRect(
	VertexBuffer *outBuf, const QRectF &rect, const QPointF &tlUv,
	const QPointF &trUv, const QPointF &blUv, const QPointF &brUv)
{
	if(outBuf == NULL)
		return false;
	outBuf->setNumVerts(0);
	if(outBuf->getNumFloats() < TexDecalRectNumFloats)
		return false;
	outBuf->setNumVerts(TexDecalRectNumVerts);

	// Shader expects vertex format: X, Y, Z, -, U, V, -, -
	outBuf->setVertSize(8);
	float *data = outBuf->getDataPtr();
	int i = 0;

	// Top-left
	data[i++] = rect.left();
	data[i++] = rect.top();
	data[i++] = 0.0f;
	data[i++] = 1.0f;
	data[i++] = tlUv.x();
	data[i++] = tlUv.y();
	data[i++] = 0.0f;
	data[i++] = 0.0f;

	// Top-right
	data[i++] = rect.right();
	data[i++] = rect.top();
	data[i++] = 0.0f;
	data[i++] = 1.0f;
	data[i++] = trUv.x();
	data[i++] = trUv.y();
	data[i++] = 0.0f;
	data[i++] = 0.0f;

	// Bottom-left
	data[i++] = rect.left();
	data[i++] = rect.bottom();
	data[i++] = 0.0f;
	data[i++] = 1.0f;
	data[i++] = blUv.x();
	data[i++] = blUv.y();
	data[i++] = 0.0f;
	data[i++] = 0.0f;

	// Bottom-right
	data[i++] = rect.right();
	data[i++] = rect.bottom();
	data[i++] = 0.0f;
	data[i++] = 1.0f;
	data[i++] = brUv.x();
	data[i++] = brUv.y();
	data[i++] = 0.0f;
	data[i++] = 0.0f;

	outBuf->setDirty(true);
	return true;
}

/// <summary>
/// Fills a `VertexBuffer` with the required data to draw a the rectangle
/// outline and handles of the resize layer graphic. Designed to be rendered
/// with `TriangleListTopology`.
/// </summary>
/// <returns>True if the buffer is valid.</returns>
bool GraphicsContext::createResizeRect(
	VertexBuffer *outBuf, const QRectF &rect, float handleSize,
	const QPointF &halfWidth)
{
	if(outBuf == NULL)
		return false;
	outBuf->setNumVerts(0);
	if(outBuf->getNumFloats() < ResizeRectNumFloats)
		return false;
	outBuf->setNumVerts(ResizeRectNumVerts);

	// Shader expects vertex format: X, Y, Z, -
	outBuf->setVertSize(4);
	float *data = outBuf->getDataPtr();
	int i = 0;

	// Add main rectangle
	i += rectOutline(&data[i], rect, halfWidth);

	// Add handle rectangles
	QRectF htRect(rect.topLeft(), QSizeF(handleSize, handleSize));
	htRect.translate(-handleSize * 0.5f, -handleSize * 0.5f);
	QRectF hmRect(htRect);
	QRectF hbRect(htRect);
	hmRect.translate(0.0f, rect.height() * 0.5f);
	hbRect.translate(0.0f, rect.height());
	i += rectOutline(&data[i], htRect, halfWidth); // Top-left
	i += rectOutline(&data[i], hmRect, halfWidth); // Middle-left
	i += rectOutline(&data[i], hbRect, halfWidth); // Bottom-left
	htRect.translate(rect.width() * 0.5f, 0.0f);
	hmRect.translate(rect.width() * 0.5f, 0.0f);
	hbRect.translate(rect.width() * 0.5f, 0.0f);
	i += rectOutline(&data[i], htRect, halfWidth); // Top-center
	i += rectOutline(&data[i], hmRect, halfWidth); // Middle-center
	i += rectOutline(&data[i], hbRect, halfWidth); // Bottom-center
	htRect.translate(rect.width() * 0.5f, 0.0f);
	hmRect.translate(rect.width() * 0.5f, 0.0f);
	hbRect.translate(rect.width() * 0.5f, 0.0f);
	i += rectOutline(&data[i], htRect, halfWidth); // Top-right
	i += rectOutline(&data[i], hmRect, halfWidth); // Middle-right
	i += rectOutline(&data[i], hbRect, halfWidth); // Bottom-right

	outBuf->setDirty(true);
	return true;
}

/// <summary>
/// Return the smallest power-of-two that's equal or greater than `n`. Valid
/// for unsigned 32-bit integer inputs only.
/// </summary>
quint32 GraphicsContext::nextPowTwo(quint32 n)
{
	n--;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	n++;

	return n;
}

/// <summary>
/// Set the view matrix for the currently selected render target.
/// WARNING: Unlike the others the user target matrices are shared between
/// multiple render targets and therefore is in an undefined state when the
/// application switches to it.
/// </summary>
void GraphicsContext::setViewMatrix(const QMatrix4x4 &matrix)
{
	switch(m_currentTarget) {
	default:
	case GfxScreenTarget:
		if(m_screenViewMat != matrix)
			m_cameraConstantsDirty = true;
		m_screenViewMat = matrix;
		break;
	case GfxCanvas1Target:
	case GfxCanvas2Target:
		if(m_canvasViewMat != matrix)
			m_cameraConstantsDirty = true;
		m_canvasViewMat = matrix;
		break;
	case GfxScratch1Target:
	case GfxScratch2Target:
		if(m_scratchViewMat != matrix)
			m_cameraConstantsDirty = true;
		m_scratchViewMat = matrix;
		break;
	case GfxUserTarget:
		if(m_userViewMat != matrix)
			m_cameraConstantsDirty = true;
		m_userViewMat = matrix;
		break;
	}
}

/// <summary>
/// Get the view matrix for the currently selected render target.
/// </summary>
QMatrix4x4 GraphicsContext::getViewMatrix() const
{
	switch(m_currentTarget) {
	default:
	case GfxScreenTarget:
		return m_screenViewMat;
	case GfxCanvas1Target:
	case GfxCanvas2Target:
		return m_canvasViewMat;
	case GfxScratch1Target:
	case GfxScratch2Target:
		return m_scratchViewMat;
	case GfxUserTarget:
		return m_userViewMat;
	}
}

/// <summary>
/// Set the projection matrix for the currently selected render target.
/// WARNING: Unlike the others the user target matrices are shared between
/// multiple render targets and therefore is in an undefined state when the
/// application switches to it.
/// </summary>
void GraphicsContext::setProjectionMatrix(const QMatrix4x4 &matrix)
{
	switch(m_currentTarget) {
	default:
	case GfxScreenTarget:
		if(m_screenProjMat != matrix)
			m_cameraConstantsDirty = true;
		m_screenProjMat = matrix;
		break;
	case GfxCanvas1Target:
	case GfxCanvas2Target:
		if(m_canvasProjMat != matrix)
			m_cameraConstantsDirty = true;
		m_canvasProjMat = matrix;
		break;
	case GfxScratch1Target:
	case GfxScratch2Target:
		if(m_scratchProjMat != matrix)
			m_cameraConstantsDirty = true;
		m_scratchProjMat = matrix;
		break;
	case GfxUserTarget:
		if(m_userProjMat != matrix)
			m_cameraConstantsDirty = true;
		m_userProjMat = matrix;
		break;
	}
}

/// <summary>
/// Get the projection matrix for the currently selected render target.
/// </summary>
QMatrix4x4 GraphicsContext::getProjectionMatrix() const
{
	switch(m_currentTarget) {
	default:
	case GfxScreenTarget:
		return m_screenProjMat;
	case GfxCanvas1Target:
	case GfxCanvas2Target:
		return m_canvasProjMat;
	case GfxScratch1Target:
	case GfxScratch2Target:
		return m_scratchProjMat;
	case GfxUserTarget:
		return m_userProjMat;
	}
}

/// <summary>
/// Set the view matrix for the screen render target without actually having to
/// switch render targets.
/// </summary>
void GraphicsContext::setScreenViewMatrix(const QMatrix4x4 &matrix)
{
	if(m_currentTarget == GfxScreenTarget && m_screenViewMat != matrix)
		m_cameraConstantsDirty = true;
	m_screenViewMat = matrix;
}

/// <summary>
/// Get the view matrix for the screen render target without actually having to
/// switch render targets.
/// </summary>
QMatrix4x4 GraphicsContext::getScreenViewMatrix() const
{
	return m_screenViewMat;
}

/// <summary>
/// Set the projection matrix for the screen render target without actually
/// having to switch render targets.
/// </summary>
void GraphicsContext::setScreenProjectionMatrix(const QMatrix4x4 &matrix)
{
	if(m_currentTarget == GfxScreenTarget && m_screenProjMat != matrix)
		m_cameraConstantsDirty = true;
	m_screenProjMat = matrix;
}

/// <summary>
/// Get the projection matrix for the screen render target without actually
/// having to switch render targets.
/// </summary>
QMatrix4x4 GraphicsContext::getScreenProjectionMatrix() const
{
	return m_screenProjMat;
}

void GraphicsContext::setUserRenderTarget(Texture *texA, Texture *texB)
{
	if((texA != NULL && !texA->isTargetable()) ||
		(texB != NULL && !texB->isTargetable()))
	{
		gfxLog(LOG_CAT, GfxLog::Warning)
			<< "Tried to set the user render target to a texture that isn't "
			<< "targetable";
		return;
	}
	m_userTargets[0] = texA;
	m_userTargets[1] = texB;
	if(m_currentTarget == GfxUserTarget)
		setRenderTarget(GfxUserTarget); // Rebind target
}

void GraphicsContext::setUserRenderTargetViewport(const QRect &rect)
{
	m_userTargetViewport = rect;
	if(m_currentTarget == GfxUserTarget)
		setRenderTarget(GfxUserTarget); // Rebind target
}

void GraphicsContext::setResizeLayerRect(const QRectF &rect)
{
	if(m_resizeRect != rect)
		m_resizeConstantsDirty = true;
	m_resizeRect = rect;
}

void GraphicsContext::setRgbNv16PxSize(const QPointF &size)
{
	if(m_rgbNv16PxSize != size)
		m_rgbNv16ConstantsDirty = true;
	m_rgbNv16PxSize = size;
}

void GraphicsContext::setTexDecalModColor(const QColor &color)
{
	if(m_texDecalModulate != color)
		m_texDecalConstantsDirty = true;
	m_texDecalModulate = color;
}

/// <summary>
/// Sets the gamma, brightness, contrast and saturation constants for shaders
/// that use them. Gamma controls the linearity of the colour, has a default
/// value of `1.0f` and is in the range [0.1, 10.0]. Brightness is a
/// constant that is added or subtracted to each RGB component, has a default
/// value of `0.0f` and is in the range [-1.0f, 1.0f]. Contrast is how far each
/// RGB component is stretched from `0.5f`, has a default value of `1.0f` and
/// is in the range [0.0f, 3.0f]. Saturation is how far each RGB component is
/// stretched from the combined luminance, has a default value of `1.0f` and is
/// in the range [0.0f, 3.0f].
/// </summary>
void GraphicsContext::setTexDecalEffects(
	float gamma, float brightness, float contrast, float saturation)
{
	if(gamma <= 0.0f)
		gamma = 0.01f;
	gamma = 1.0f / gamma;
	if(m_texDecalEffects[0] != gamma ||
		m_texDecalEffects[1] != brightness ||
		m_texDecalEffects[2] != contrast ||
		m_texDecalEffects[3] != saturation)
	{
		m_texDecalConstantsDirty = true;
	}
	m_texDecalEffects[0] = gamma;
	m_texDecalEffects[1] = brightness;
	m_texDecalEffects[2] = contrast;
	m_texDecalEffects[3] = saturation;
}

/// <summary>
/// A helper method for `setTexDecalEffects()` that converts user-friendly
/// numbers into the required format, sets the constants and returns whether
/// or not the caller should actually use the more expensive effects shader.
/// Gamma has a default value of `0.0f` and is in the range [0.1, 10.0].
/// Brightness has a default value of `0` and is in the range [-250, 250].
/// Contrast has a default value of `0` and is in the range [-100, 200].
/// Saturation has a default value of `0` and is in the range [-100, 200].
/// </summary>
/// <returns>True if the caller should use the effects shader</returns>
bool GraphicsContext::setTexDecalEffectsHelper(
	float gamma, int brightness, int contrast, int saturation)
{
	// Is the effects shader required?
	gamma = qFuzzyCompare(gamma, 1.0f) ? 1.0f : gamma;
	if(gamma == 1.0f && brightness == 0 && contrast == 0 && saturation == 0)
		return false; // Effects shader not required

	// Convert to our required format
	float bright = (float)brightness / 250.0f;
	float contr = (float)(contrast + 100) / 100.0f;
	float satur = (float)(saturation + 100) / 100.0f;

	// Set the constants and return
	setTexDecalEffects(gamma, bright, contr, satur);
	return true;
}

/// <summary>
/// Copies pixel colour information to nearby pixels that are fully
/// transparent. In order to improve image compression some image encoders,
/// such as Photoshop, remove the colour information from pixels that are fully
/// transparent when saved to certain image formats such as PNG. This results
/// in colour "fringing" when rendered in a 3D scene if the image does not have
/// a perfect 1:1 pixel mapping to the screen. This is because bilinear
/// filtering uses the invalid colour information of the transparent pixels
/// when interpolating.
/// </summary>
/// <returns>True if the image was successfully diluted</returns>
bool GraphicsContext::diluteImage(QImage &img) const
{
	if(!img.hasAlphaChannel())
		return false; // No transparent pixels

	// We cannot modify the pixels of our input image as it'll cause artifacts
	// with our algorithm
	const int w = img.width();
	const int h = img.height();
	QImage imgOut = QImage(w, h, QImage::Format_ARGB32);
	imgOut.fill(Qt::transparent);

	// The maximum distance from the pixel to search for colour information
	const int MAX_DILUTION = 2;

	bool modified = false;
	for(int y = 0; y < h; y++) {
		for(int x = 0; x < w; x++) {
			QRgb pix = img.pixel(x, y);
			if(qAlpha(pix) == 0) {
				if(dilutePixel(img, imgOut, x, y, MAX_DILUTION))
					modified = true;
			}
		}
	}

	if(modified) {
		// Merge our images together. We cannot use `QPainter::drawImage()` as
		// it overwrites the colour information of fully transparent pixels.

		// We can only safely cast scan lines to `QRgb` if we are using a
		// 32-bit pixel format
		if(img.format() != QImage::Format_ARGB32)
			img = img.convertToFormat(QImage::Format_ARGB32);

		for(int y = 0; y < h; y++) {
			QRgb *rowIn = (QRgb *)img.scanLine(y);
			QRgb *rowOut = (QRgb *)imgOut.scanLine(y);
			for(int x = 0; x < w; x++) {
				if(!qAlpha(rowIn[x])) // TODO: Optimize condition out?
					rowIn[x] = rowOut[x];
			}
		}
	}

	return true;
}

void GraphicsContext::callInitializedCallbacks()
{
	for(int i = 0; i < m_initializedCallbackList.size(); i++) {
		const InitializedCallback &callback = m_initializedCallbackList.at(i);
		callback.callback(callback.opaque, this);
	}
}

void GraphicsContext::addInitializedCallback(
	VidgfxContextInitializedCallback *initialized, void *opaque)
{
	InitializedCallback callback;
	callback.callback = initialized;
	callback.opaque = opaque;
	m_initializedCallbackList.append(callback);
}

void GraphicsContext::removeInitializedCallback(
	VidgfxContextInitializedCallback *initialized, void *opaque)
{
	InitializedCallback callback;
	callback.callback = initialized;
	callback.opaque = opaque;
	int id = m_initializedCallbackList.indexOf(callback);
	if(id >= 0)
		m_initializedCallbackList.remove(id);
}

void GraphicsContext::callDestroyingCallbacks()
{
	for(int i = 0; i < m_destroyingCallbackList.size(); i++) {
		const DestroyingCallback &callback = m_destroyingCallbackList.at(i);
		callback.callback(callback.opaque, this);
	}
}

void GraphicsContext::addDestroyingCallback(
	VidgfxContextDestroyingCallback *destroying, void *opaque)
{
	DestroyingCallback callback;
	callback.callback = destroying;
	callback.opaque = opaque;
	m_destroyingCallbackList.append(callback);
}

void GraphicsContext::removeDestroyingCallback(
	VidgfxContextDestroyingCallback *destroying, void *opaque)
{
	DestroyingCallback callback;
	callback.callback = destroying;
	callback.opaque = opaque;
	int id = m_destroyingCallbackList.indexOf(callback);
	if(id >= 0)
		m_destroyingCallbackList.remove(id);
}
