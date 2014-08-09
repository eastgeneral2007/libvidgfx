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

#ifndef D3DCONTEXT_H
#define D3DCONTEXT_H

#include "graphicscontext.h"
#include <QtCore/QSize>
#include <windows.h>

struct ID3D10BlendState;
struct ID3D10Buffer;
struct ID3D10Device;
struct ID3D10InputLayout;
struct ID3D10PixelShader;
struct ID3D10RasterizerState;
struct ID3D10RenderTargetView;
struct ID3D10SamplerState;
struct ID3D10ShaderResourceView;
struct ID3D10Texture2D;
struct ID3D10VertexShader;
struct IDXGIAdapter;
struct IDXGIFactory1; // DXGI 1.1
struct IDXGISurface1; // DXGI 1.1
struct IDXGISwapChain;
struct D3D10_INPUT_ELEMENT_DESC;
enum DXGI_FORMAT;

class D3DContext;

//=============================================================================
class LVG_EXPORT D3DVertexBuffer : public VertexBuffer
{
private: // Members -----------------------------------------------------------
	D3DContext *	m_context;
	ID3D10Buffer *	m_buffer;

public: // Constructor/destructor ---------------------------------------------
	D3DVertexBuffer(D3DContext *context, int numFloats);
	virtual ~D3DVertexBuffer();

public: // Methods ------------------------------------------------------------
	void			update();
	void			bind();
	ID3D10Buffer *	getBuffer() const;
};
//=============================================================================

inline ID3D10Buffer *D3DVertexBuffer::getBuffer() const
{
	return m_buffer;
}

//=============================================================================
class LVG_EXPORT D3DTexture : public Texture
{
protected: // Members ---------------------------------------------------------
	ID3D10Texture2D *			m_tex;
	ID3D10ShaderResourceView *	m_view;
	ID3D10RenderTargetView *	m_target;
	bool						m_doBgraSwizzle;
	bool						m_isSrgb;

	// GDI-compatible textures only
	IDXGISurface1 *				m_surface;
	HDC							m_hdc;

public: // Constructor/destructor ---------------------------------------------
	D3DTexture(
		D3DContext *context, GfxTextureFlags flags, const QSize &size,
		DXGI_FORMAT format, void *initialData = NULL, int stride = 0);
	D3DTexture(D3DContext *context, ID3D10Texture2D *tex);
	virtual ~D3DTexture();

public: // Methods ------------------------------------------------------------
	ID3D10Texture2D *			getTexture() const;
	ID3D10ShaderResourceView *	getResourceView() const;
	ID3D10RenderTargetView *	getTargetView() const;
	bool						doBgraSwizzle() const;

	HDC							getDC();
	void						releaseDC();

	DXGI_FORMAT					getPixelFormat();
private:
	bool						isSrgbFormat(DXGI_FORMAT format);

public: // Interface ----------------------------------------------------------
	virtual void *		map();
	virtual void		unmap();

	virtual bool		isSrgbHack();
};
//=============================================================================

inline ID3D10Texture2D *D3DTexture::getTexture() const
{
	return m_tex;
}

inline ID3D10ShaderResourceView *D3DTexture::getResourceView() const
{
	return m_view;
}

inline ID3D10RenderTargetView *D3DTexture::getTargetView() const
{
	return m_target;
}

inline bool D3DTexture::doBgraSwizzle() const
{
	return m_doBgraSwizzle;
}

//=============================================================================
class LVG_EXPORT D3DContext : public GraphicsContext
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	bool						m_hasDxgi11;
	bool						m_hasDxgi11Valid;
	bool						m_hasBgraTexSupport;
	bool						m_hasBgraTexSupportValid;
	IDXGISwapChain *			m_swapChain;
	ID3D10Device *				m_device;
	ID3D10RasterizerState *		m_rasterizerState;
	ID3D10SamplerState *		m_pointClampSampler;
	ID3D10SamplerState *		m_bilinearClampSampler;
	ID3D10SamplerState *		m_resizeSampler;
	ID3D10BlendState *			m_noBlend;
	ID3D10BlendState *			m_alphaBlend;
	ID3D10BlendState *			m_premultiBlend;

	// Render targets
	ID3D10RenderTargetView *	m_screenTarget;
	QSize						m_screenTargetSize;
	D3DTexture *				m_canvas1Texture;
	D3DTexture *				m_canvas2Texture;
	QSize						m_canvasTargetSize;
	D3DTexture *				m_scratch1Texture;
	D3DTexture *				m_scratch2Texture;
	QSize						m_scratchTargetSize;
	int							m_scratchNextTarget;

	// Constant buffers
	float						m_cameraConstantsLocal[(4*4)*2]; // 2 4x4 matrices
	ID3D10Buffer *				m_cameraConstants;
	float						m_resizeConstantsLocal[4]; // 1 XYWH rectangle
	ID3D10Buffer *				m_resizeConstants;
	float						m_rgbNv16ConstantsLocal[4]; // 4 horizontal offsets
	ID3D10Buffer *				m_rgbNv16Constants;
	// 1 RGBA colour + 1 integer for flags + 3 unused + 4 effect floats
	float						m_texDecalConstantsLocal[12];
	ID3D10Buffer *				m_texDecalConstants;
	quint32						m_texDecalFlags;

	// Shaders
	GfxShader					m_boundShader;
	ID3D10VertexShader *		m_solidVS;
	ID3D10PixelShader *			m_solidPS;
	ID3D10InputLayout *			m_solidIL;
	ID3D10VertexShader *		m_texDecalVS;
	ID3D10PixelShader *			m_texDecalPS;
	ID3D10PixelShader *			m_texDecalGbcsPS;
	ID3D10PixelShader *			m_texDecalRgbPS;
	ID3D10InputLayout *			m_texDecalIL;
	ID3D10VertexShader *		m_resizeVS;
	ID3D10PixelShader *			m_resizePS;
	ID3D10InputLayout *			m_resizeIL;
	// All these share `texDecalVS` and IL
	ID3D10PixelShader *			m_rgbNv16PS;
	ID3D10PixelShader *			m_yv12RgbPS;
	ID3D10PixelShader *			m_UyvyRgbPS;
	ID3D10PixelShader *			m_HdycRgbPS;
	ID3D10PixelShader *			m_Yuy2RgbPS;

	// Advanced rendering
	VertexBuffer *				m_mipmapBuf;

public: // Static methods -----------------------------------------------------
	static HRESULT	createDXGIFactory1Dynamic(IDXGIFactory1 **factoryOut);
	static void		logDisplayAdapters();

public: // Constructor/destructor ---------------------------------------------
	D3DContext();
	virtual ~D3DContext();

public: // Methods ------------------------------------------------------------
	bool			initialize(
		HWND hwnd, const QSize &size, const QColor &resizeBorderCol);
	ID3D10Device *	getDevice() const;
	bool			hasDxgi11();
	bool			hasBgraTexSupport();
	Texture *		createGDITexture(const QSize &size);
	Texture *		openSharedTexture(HANDLE sharedHandle);
	Texture *		openDX10Texture(ID3D10Texture2D *tex);

private:
	IDXGIAdapter *	getFirstDxgi11Adapter();

	bool			createScreenTarget();
	bool			createShaders();
	bool			createVertexShaderAndInputLayout(
		const QString &shaderName, ID3D10VertexShader **shader,
		ID3D10InputLayout **layout,
		const D3D10_INPUT_ELEMENT_DESC *layoutDesc, int layoutDescSize);
	bool			createPixelShader(
		const QString &shaderName, ID3D10PixelShader **shader);
	QByteArray		getShaderFileData(const QString &shaderName) const;

	void			updateCameraConstants();
	void			updateResizeConstants();
	void			updateRgbNv16Constants();
	void			updateTexDecalConstants();

	void			setSwizzleInTexDecal(bool doSwizzle);

public: // Interface ----------------------------------------------------------
	virtual bool	isValid() const;
	virtual void	flush();

	// Buffers
	virtual VertexBuffer *	createVertexBuffer(int size);
	virtual void			deleteVertexBuffer(VertexBuffer *buf);
	virtual Texture *		createTexture(
		QImage img, bool writable = false, bool targetable = false);
	virtual Texture *		createTexture(
		const QSize &size, bool writable = false, bool targetable = false,
		bool useBgra = false);
	virtual Texture *		createTexture(
		const QSize &size, Texture *sameFormat, bool writable = false,
		bool targetable = false);
	virtual Texture *		createStagingTexture(const QSize &size);
	virtual void			deleteTexture(Texture *tex);
	virtual bool			copyTextureData(
		Texture *dst, Texture *src, const QPoint &dstPos,
		const QRect &srcRect);

	// Render targets
	virtual void			resizeScreenTarget(const QSize &newSize);
	virtual void			resizeCanvasTarget(const QSize &newSize);
	virtual void			resizeScratchTarget(const QSize &newSize);
	virtual void			swapScreenBuffers();
	virtual	Texture *		getTargetTexture(GfxRenderTarget target);
	virtual GfxRenderTarget	getNextScratchTarget();
	virtual QPointF			getScratchTargetToTextureRatio();

	// Advanced rendering
	virtual Texture *	prepareTexture(
		Texture *tex, const QSize &size, GfxFilter filter, bool setFilter,
		QPointF &pxSizeOut, QPointF &botRightOut);
	virtual Texture *	prepareTexture(
		Texture *tex, const QRect &cropRect, const QSize &size,
		GfxFilter filter, bool setFilter, QPointF &pxSizeOut,
		QPointF &topLeftOut, QPointF &botRightOut);
	virtual Texture *	convertToBgrx(
		GfxPixelFormat format, Texture *planeA, Texture *planeB,
		Texture *planeC);

	// Drawing
	virtual void		setRenderTarget(GfxRenderTarget target);
	virtual void		setShader(GfxShader shader);
	virtual void		setTopology(GfxTopology topology);
	virtual void		setBlending(GfxBlending blending);
	virtual void		setTexture(
		Texture *texA, Texture *texB = NULL, Texture *texC = NULL);
	virtual void		setTextureFilter(GfxFilter filter);
	virtual void		clear(const QColor &color);
	virtual void		drawBuffer(
		VertexBuffer *buf, int numVertices = -1, int startVertex = 0);

Q_SIGNALS: // Signals ---------------------------------------------------------
	void			hasDxgi11Changed(bool hasDxgi11);
	void			hasBgraTexSupportChanged(bool hasBgraTexSupport);
};
//=============================================================================

inline ID3D10Device *D3DContext::getDevice() const
{
	return m_device;
}

#endif // D3DCONTEXT_H
