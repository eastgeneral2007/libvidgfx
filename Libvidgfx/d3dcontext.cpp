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

#include "include/d3dcontext.h"
#include "include/gfxlog.h"
#include "include/versionhelpers.h"
#include "pciidparser.h"
#include <d3d10_1.h>
#include <QtCore/QFile>
#include <QtGui/QImage>

// By default Mishira creates a DirectX 10.1 level 10.0 device. As we want to
// support DirectX 9 hardware we must also test everything in DirectX 10 Level
// 9 mode in order to make sure we're not using any incompatible features. Set
// this definition to "1" in order to test in DirectX 10 Level 9 mode.
//
// See the following URLs for compatibility notes:
// http://msdn.microsoft.com/en-us/library/windows/desktop/ff476150(v=vs.85).aspx
#define FORCE_DIRECTX_10_1_LEVEL_9_3 0

// Set this definition to "1" in order to test how Mishira would behave if
// BGRA textures were not supported.
#define FORCE_NO_BGRA_SUPPORT 0

const QString LOG_CAT = QStringLiteral("Gfx");

// Function pointer to `CreateDXGIFactory1()`
typedef HRESULT (WINAPI* PFN_DXGI_CREATE_DXGI_FACTORY1)(REFIID, void **);

//=============================================================================
// Helpers

QString numberToHexString(quint64 num)
{
	return QStringLiteral("0x") + QString::number(num, 16).toUpper();
}

/// <summary>
/// Returns the number of bytes adding SI units such as "K" or "M" using orders
/// of 1024 of `metric` is false or 1000 if it's true. It is up to the caller
/// to append "B", "b", "B/s" or "b/s" to the end of the returned string.
/// </summary>
QString humanBitsBytes(quint64 bytes, int numDecimals = 0, bool metric = false)
{
	// Don't use floating points due to rounding
	QString units;
	quint64 mag = 1024ULL;
	if(metric)
		mag = 1000ULL;
	quint64 order = 1ULL;
	if(bytes < mag) {
		numDecimals = 0;
		units = QStringLiteral(" ");
		goto humanBytesReturn;
	}
	order *= mag;
	if(bytes < mag * mag) {
		numDecimals = qMin(3, numDecimals); // Not really true due to 1024
		units = QStringLiteral(" K");
		goto humanBytesReturn;
	}
	order *= mag;
	if(bytes < mag * mag * mag) {
		numDecimals = qMin(6, numDecimals); // Not really true due to 1024
		units = QStringLiteral(" M");
		goto humanBytesReturn;
	}
	order *= mag;
	numDecimals = qMin(9, numDecimals); // Not really true due to 1024
	units = QStringLiteral(" G");

humanBytesReturn:
	qreal flt = (qreal)bytes / (qreal)order; // Result as a float
	if(numDecimals <= 0)
		return QStringLiteral("%L1%2").arg(qRound(flt)).arg(units);
	quint64 pow = 1ULL;
	for(int i = 0; i < numDecimals; i++)
		pow *= 10ULL;
	quint64 dec = qRound(flt * (qreal)pow);
	quint64 whole = dec / pow; // Whole part
	dec %= pow; // Decimal part as an integer
	return QStringLiteral("%L1.%2%3").arg(whole)
		.arg(dec, numDecimals, 10, QChar('0')).arg(units);
}

QString getDXErrorCode(HRESULT res)
{
	switch(res) {
	case DXGI_ERROR_INVALID_CALL:
		return QStringLiteral("DXGI_ERROR_INVALID_CALL");
	case DXGI_ERROR_NOT_FOUND:
		return QStringLiteral("DXGI_ERROR_NOT_FOUND");
	case DXGI_ERROR_MORE_DATA:
		return QStringLiteral("DXGI_ERROR_MORE_DATA");
	case DXGI_ERROR_UNSUPPORTED:
		return QStringLiteral("DXGI_ERROR_UNSUPPORTED");
	case DXGI_ERROR_DEVICE_REMOVED:
		return QStringLiteral("DXGI_ERROR_DEVICE_REMOVED");
	case DXGI_ERROR_DEVICE_HUNG:
		return QStringLiteral("DXGI_ERROR_DEVICE_HUNG");
	case DXGI_ERROR_DEVICE_RESET:
		return QStringLiteral("DXGI_ERROR_DEVICE_RESET");
	case DXGI_ERROR_WAS_STILL_DRAWING:
		return QStringLiteral("DXGI_ERROR_WAS_STILL_DRAWING");
	case DXGI_ERROR_FRAME_STATISTICS_DISJOINT:
		return QStringLiteral("DXGI_ERROR_FRAME_STATISTICS_DISJOINT");
	case DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE:
		return QStringLiteral("DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE");
	case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
		return QStringLiteral("DXGI_ERROR_DRIVER_INTERNAL_ERROR");
	case DXGI_ERROR_NONEXCLUSIVE:
		return QStringLiteral("DXGI_ERROR_NONEXCLUSIVE");
	case DXGI_ERROR_NOT_CURRENTLY_AVAILABLE:
		return QStringLiteral("DXGI_ERROR_NOT_CURRENTLY_AVAILABLE");
	case DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED:
		return QStringLiteral("DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED");
	case DXGI_ERROR_REMOTE_OUTOFMEMORY:
		return QStringLiteral("DXGI_ERROR_REMOTE_OUTOFMEMORY");
	case DXGI_ERROR_ACCESS_LOST:
		return QStringLiteral("DXGI_ERROR_ACCESS_LOST");
	case DXGI_ERROR_WAIT_TIMEOUT:
		return QStringLiteral("DXGI_ERROR_WAIT_TIMEOUT");
	case DXGI_ERROR_SESSION_DISCONNECTED:
		return QStringLiteral("DXGI_ERROR_SESSION_DISCONNECTED");
	case DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE:
		return QStringLiteral("DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE");
	case DXGI_ERROR_CANNOT_PROTECT_CONTENT:
		return QStringLiteral("DXGI_ERROR_CANNOT_PROTECT_CONTENT");
	case DXGI_ERROR_ACCESS_DENIED:
		return QStringLiteral("DXGI_ERROR_ACCESS_DENIED");
	case DXGI_ERROR_NAME_ALREADY_EXISTS:
		return QStringLiteral("DXGI_ERROR_NAME_ALREADY_EXISTS");
	case DXGI_ERROR_MODE_CHANGE_IN_PROGRESS:
		return QStringLiteral("DXGI_ERROR_MODE_CHANGE_IN_PROGRESS");
	case DXGI_DDI_ERR_WASSTILLDRAWING:
		return QStringLiteral("DXGI_DDI_ERR_WASSTILLDRAWING");
	case DXGI_DDI_ERR_UNSUPPORTED:
		return QStringLiteral("DXGI_DDI_ERR_UNSUPPORTED");
	case DXGI_DDI_ERR_NONEXCLUSIVE:
		return QStringLiteral("DXGI_DDI_ERR_NONEXCLUSIVE");
	case D3D10_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS:
		return QStringLiteral("D3D10_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS");
	case D3D10_ERROR_FILE_NOT_FOUND:
		return QStringLiteral("D3D10_ERROR_FILE_NOT_FOUND");
	case D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS:
		return QStringLiteral("D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS");
	case D3D11_ERROR_FILE_NOT_FOUND:
		return QStringLiteral("D3D11_ERROR_FILE_NOT_FOUND");
	case D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS:
		return QStringLiteral("D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS");
	case D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD:
		return QStringLiteral(
			"D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD");
	case E_UNEXPECTED:
		return QStringLiteral("E_UNEXPECTED");
	case E_NOTIMPL:
		return QStringLiteral("E_NOTIMPL");
	case E_OUTOFMEMORY:
		return QStringLiteral("E_OUTOFMEMORY");
	case E_INVALIDARG:
		return QStringLiteral("E_INVALIDARG");
	case E_NOINTERFACE:
		return QStringLiteral("E_NOINTERFACE");
	case E_POINTER:
		return QStringLiteral("E_POINTER");
	case E_HANDLE:
		return QStringLiteral("E_HANDLE");
	case E_ABORT:
		return QStringLiteral("E_ABORT");
	case E_FAIL:
		return QStringLiteral("E_FAIL");
	case E_ACCESSDENIED:
		return QStringLiteral("E_ACCESSDENIED");
	case S_FALSE:
		return QStringLiteral("S_FALSE");
	case S_OK:
		return QStringLiteral("S_OK");
	default:
		return numberToHexString((uint)res);
	}
}

bool createDXBuffer(
	ID3D10Device *device, D3D10_BUFFER_DESC *desc, void *initialData,
	ID3D10Buffer **buffer)
{
	// Initial buffer contents
	D3D10_SUBRESOURCE_DATA data;
	if(initialData != NULL) {
		data.pSysMem = initialData;
		data.SysMemPitch = 0;
		data.SysMemSlicePitch = 0;
	}

	HRESULT res;
	if(initialData == NULL)
		res = device->CreateBuffer(desc, NULL, buffer);
	else
		res = device->CreateBuffer(desc, &data, buffer);
	if(FAILED(res)) {
		gfxLog(LOG_CAT, GfxLog::Warning)
			<< "Failed to allocate DirectX buffer of " << desc->ByteWidth
			<< " bytes. Reason = " << getDXErrorCode(res);
		return false;
	}

	return true;
}

bool updateDXBuffer(
	ID3D10Device *device, ID3D10Buffer *buffer, void *newData, int numBytes)
{
	// Mapping is apparently faster than this, TODO: Verify
	//device->UpdateSubresource(buffer, 0, NULL, newData, 0, 0);

	// Map buffer to CPU RAM
	float *data;
	HRESULT res = buffer->Map(
		D3D10_MAP_WRITE_DISCARD, 0, reinterpret_cast<void **>(&data));
	if(FAILED(res)) {
		gfxLog(LOG_CAT, GfxLog::Warning)
			<< "Failed to map DirectX buffer into RAM. "
			<< "Reason = " << getDXErrorCode(res);
		return false;
	}

	// Copy data, TODO: SSE?
	memcpy(data, newData, numBytes);

	// Unmap buffer
	buffer->Unmap();

	return true;
}

//=============================================================================
// D3DVertexBuffer class

D3DVertexBuffer::D3DVertexBuffer(D3DContext *context, int numFloats)
	: VertexBuffer(numFloats)
	, m_context(context)
	, m_buffer(NULL)
{
	// Get device
	ID3D10Device *device = m_context->getDevice();

	// Create hardware buffer
	D3D10_BUFFER_DESC desc;
	desc.ByteWidth = numFloats * sizeof(float);
	desc.Usage = D3D10_USAGE_DYNAMIC;
	desc.BindFlags = D3D10_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	if(!createDXBuffer(device, &desc, m_data, &m_buffer)) {
		// Failed to create buffer
		return;
	}
}

D3DVertexBuffer::~D3DVertexBuffer()
{
	if(m_buffer)
		m_buffer->Release();
}

void D3DVertexBuffer::update()
{
	if(m_buffer == NULL)
		return; // Buffer doesn't exist
	if(!m_dirty)
		return; // Buffer is up-to-date

	// Get device
	ID3D10Device *device = m_context->getDevice();

	// Update hardware buffer
	int bufSize = m_numFloats * sizeof(float);
	if(!updateDXBuffer(device, m_buffer, m_data, bufSize)) {
		// Failed to update buffer contents
		return;
	}

	m_dirty = false;
}

void D3DVertexBuffer::bind()
{
	if(m_buffer == NULL)
		return; // Buffer doesn't exist

	// Make sure the buffer isn't dirty
	if(m_dirty)
		update();

	// Get device
	ID3D10Device *device = m_context->getDevice();

	// Bind the buffer
	if(m_vertSize <= 0)
		return; // Invalid stride
	uint stride = m_vertSize * sizeof(float);
	uint offset = 0;
	device->IASetVertexBuffers(0, 1, &m_buffer, &stride, &offset);
}

//=============================================================================
// D3DTexture class

D3DTexture::D3DTexture(
	D3DContext *context, GfxTextureFlags flags, const QSize &size,
	DXGI_FORMAT format, void *initialData, int stride)
	: Texture(flags, size)
	, m_tex(NULL)
	, m_view(NULL)
	, m_target(NULL)
	, m_doBgraSwizzle(false)
	, m_isSrgb(false)

	// GDI-compatible textures only
	, m_surface(NULL)
	, m_hdc(NULL)
{
	// Get device
	ID3D10Device *device = context->getDevice();

	// If the device doesn't support BGRA textures but the pixel format was
	// requested we instead use an RGBA pixel format and do a swizzle in the
	// pixel shader.
	if(!context->hasBgraTexSupport()) {
		if(format == DXGI_FORMAT_B8G8R8A8_UNORM) {
			format = DXGI_FORMAT_R8G8B8A8_UNORM;
			m_doBgraSwizzle = true;
		} else if(format == DXGI_FORMAT_B8G8R8X8_UNORM) {
			format = DXGI_FORMAT_R8G8B8A8_UNORM; // RGBX format doesn't exist
			m_doBgraSwizzle = true;
		}
	}

	//-------------------------------------------------------------------------
	// Create texture object

	m_isSrgb = isSrgbFormat(format);

	D3D10_TEXTURE2D_DESC desc;
	desc.Width = size.width();
	desc.Height = size.height();
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	if(isStaging()) {
		// Used for reading back data only
		desc.Usage = D3D10_USAGE_STAGING;
		desc.BindFlags = 0;
		desc.CPUAccessFlags = D3D10_CPU_ACCESS_READ;
	} else {
		desc.Usage = isWritable() ? D3D10_USAGE_DYNAMIC : D3D10_USAGE_DEFAULT;
		desc.BindFlags = isTargetable()
			? (D3D10_BIND_SHADER_RESOURCE | D3D10_BIND_RENDER_TARGET)
			: (D3D10_BIND_SHADER_RESOURCE);
		desc.CPUAccessFlags = isWritable() ? D3D10_CPU_ACCESS_WRITE : 0;
	}
	if(flags & GfxGDIFlag)
		desc.MiscFlags = D3D10_RESOURCE_MISC_GDI_COMPATIBLE;
	else
		desc.MiscFlags = 0;

	HRESULT res;
	if(stride <= 0)
		stride = size.width() * 4; // Each pixel = 32 bits = 4 bytes
	if(initialData != NULL) {
		D3D10_SUBRESOURCE_DATA data;
		data.pSysMem = initialData;
		data.SysMemPitch = stride;
		data.SysMemSlicePitch = 0;
		res = device->CreateTexture2D(&desc, &data, &m_tex);
	}
	else
		res = device->CreateTexture2D(&desc, NULL, &m_tex);
	if(FAILED(res)) {
		gfxLog(LOG_CAT, GfxLog::Warning)
			<< "Failed to create DirectX texture. "
			<< "Reason = " << getDXErrorCode(res);
		return;
	}

	//-------------------------------------------------------------------------
	// Create shader resource view

	if(!isStaging()) {
		D3D10_SHADER_RESOURCE_VIEW_DESC viewDesc;
		viewDesc.Format = desc.Format;
		viewDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
		viewDesc.Texture2D.MostDetailedMip = 0;
		viewDesc.Texture2D.MipLevels = desc.MipLevels;
		res = device->CreateShaderResourceView(m_tex, &viewDesc, &m_view);
		if(FAILED(res)) {
			gfxLog(LOG_CAT, GfxLog::Warning)
				<< "Failed to create DirectX shader resource view. "
				<< "Reason = " << getDXErrorCode(res);
			return;
		}
	}

	//-------------------------------------------------------------------------
	// Create render target view

	if(isTargetable()) {
		res = device->CreateRenderTargetView(m_tex, NULL, &m_target);
		if(FAILED(res)) {
			gfxLog(LOG_CAT, GfxLog::Warning)
				<< "Failed to create DirectX render target view. "
				<< "Reason = " << getDXErrorCode(res);
			m_flags &= ~GfxTargetableFlag; // Unset flag
			return;
		}
	}

	// Texture was successfully created
	m_isValid = true;
}

D3DTexture::D3DTexture(D3DContext *context, ID3D10Texture2D *tex)
	: Texture(0, QSize(0, 0)) // We update the size later
	, m_tex(tex)
	, m_view(NULL)
	, m_target(NULL)
	, m_doBgraSwizzle(false)
	, m_isSrgb(false)

	// GDI-compatible textures only
	, m_surface(NULL)
	, m_hdc(NULL)
{
	if(m_tex == NULL)
		return;

	// Get device
	ID3D10Device *device = context->getDevice();

	// Query texture information
	D3D10_TEXTURE2D_DESC desc;
	m_tex->GetDesc(&desc);
	m_size = QSize(desc.Width, desc.Height);
	// desc.Format?
	m_isSrgb = isSrgbFormat(desc.Format);
	if(desc.Usage & D3D10_USAGE_STAGING)
		m_flags |= GfxStagingFlag;
	if(desc.Usage & D3D10_USAGE_DYNAMIC)
		m_flags |= GfxWritableFlag;
	if(desc.BindFlags & D3D10_BIND_RENDER_TARGET)
		m_flags |= GfxTargetableFlag;
	if(desc.MiscFlags & D3D10_RESOURCE_MISC_GDI_COMPATIBLE)
		m_flags |= GfxGDIFlag;

	// Create shader resource view
	if(!isStaging()) {
		D3D10_SHADER_RESOURCE_VIEW_DESC viewDesc;
		viewDesc.Format = desc.Format;
		viewDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
		viewDesc.Texture2D.MostDetailedMip = 0;
		viewDesc.Texture2D.MipLevels = desc.MipLevels;
		HRESULT res =
			device->CreateShaderResourceView(m_tex, &viewDesc, &m_view);
		if(FAILED(res)) {
			gfxLog(LOG_CAT, GfxLog::Warning)
				<< "Failed to create DirectX shader resource view. "
				<< "Reason = " << getDXErrorCode(res);
			return;
		}
	}

	// Texture was successfully created
	m_isValid = true;
}

D3DTexture::~D3DTexture()
{
	if(m_surface) {
		m_surface->ReleaseDC(NULL);
		m_surface->Release();
	}
	if(m_view)
		m_view->Release();
	if(m_tex)
		m_tex->Release();
	if(m_target)
		m_target->Release();
}

void *D3DTexture::map()
{
	if(m_tex == NULL)
		return NULL; // Texture doesn't exist

	D3D10_MAPPED_TEXTURE2D mapInfo;
	D3D10_MAP mapType = D3D10_MAP_WRITE_DISCARD;
	if(isStaging())
		mapType = D3D10_MAP_READ;
	HRESULT res = m_tex->Map(
		D3D10CalcSubresource(0, 0, 0), mapType, 0, &mapInfo);
	if(FAILED(res)) {
		gfxLog(LOG_CAT, GfxLog::Warning)
			<< "Failed to map texture buffer into RAM. "
			<< "Reason = " << getDXErrorCode(res);
		return NULL;
	}

	m_mappedData = mapInfo.pData;
	m_stride = mapInfo.RowPitch;

	return m_mappedData;
}

void D3DTexture::unmap()
{
	if(m_tex == NULL)
		return; // Texture doesn't exist
	if(!isMapped())
		return;

	m_mappedData = NULL;
	m_stride = 0;
	m_tex->Unmap(D3D10CalcSubresource(0, 0, 0));
}

DXGI_FORMAT D3DTexture::getPixelFormat()
{
	D3D10_TEXTURE2D_DESC desc;
	m_tex->GetDesc(&desc);
	return desc.Format;
}

bool D3DTexture::isSrgbFormat(DXGI_FORMAT format)
{
	switch(format) {
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_BC1_UNORM_SRGB:
	case DXGI_FORMAT_BC2_UNORM_SRGB:
	case DXGI_FORMAT_BC3_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
	case DXGI_FORMAT_BC7_UNORM_SRGB:
		return true;
	default:
		return false;
	}
	return false; // Should never be reached
}

/// <summary>
/// Returns true if the texture uses a hardware sRGB texture format.
/// </summary>
bool D3DTexture::isSrgbHack()
{
	return m_isSrgb;
}

/// <summary>
/// WARNING: The usage of the returned HDC must abide by the remarks on the
/// "IDXGISurface1::GetDC()" documentation page.
/// http://msdn.microsoft.com/en-us/library/windows/desktop/ff471345%28v=vs.85%29.aspx
/// </summary>
HDC D3DTexture::getDC()
{
	if(!(m_flags & GfxGDIFlag))
		return NULL; // Texture isn't GDI compatible
	if(m_hdc != NULL)
		return m_hdc;
	HRESULT res = m_tex->QueryInterface(
		__uuidof(IDXGISurface1), (void **)&m_surface);
	if(FAILED(res)) {
		gfxLog(LOG_CAT, GfxLog::Warning)
			<< "Failed to get the DXGI 1.1 surface for a texture. "
			<< "Reason = " << getDXErrorCode(res);
		m_surface = NULL;
		return NULL;
	}
	res = m_surface->GetDC(TRUE, &m_hdc);
	if(FAILED(res)) {
		gfxLog(LOG_CAT, GfxLog::Warning)
			<< "Failed to get the device context for a texture surface. "
			<< "Reason = " << getDXErrorCode(res);
		m_surface->Release();
		m_surface = NULL;
		m_hdc = NULL;
		return NULL;
	}
	return m_hdc;
}

void D3DTexture::releaseDC()
{
	if(m_surface == NULL)
		return;
	m_surface->ReleaseDC(NULL);
	m_surface->Release();
	m_surface = NULL;
	m_hdc = NULL;
}

//=============================================================================
// D3DContext class

/// <summary>
/// Dynamically loads the `IDXGIFactory1()` method if we're on Windows 8 and
/// it's available (Should always be), otherwise returns NULL for `factoryOut`.
/// </summary>
HRESULT D3DContext::createDXGIFactory1Dynamic(IDXGIFactory1 **factoryOut)
{
	if(factoryOut == NULL)
		return E_FAIL;
	*factoryOut = NULL;

	if(!IsWindows8OrGreater())
		return E_NOTIMPL;

	// Find the DXGI module. We statically link it so it's guarenteed to
	// already be loaded and it allows us to `FreeLibrary()` immediately.
	HMODULE dxgiMod = LoadLibrary(TEXT("dxgi.dll"));
	PFN_DXGI_CREATE_DXGI_FACTORY1 CreateDXGIFactory1Dyn = NULL;
	if(dxgiMod != NULL) {
		CreateDXGIFactory1Dyn =
			(PFN_DXGI_CREATE_DXGI_FACTORY1)
			GetProcAddress(dxgiMod, "CreateDXGIFactory1");
	}
	if(CreateDXGIFactory1Dyn == NULL) {
		if(dxgiMod != NULL)
			FreeLibrary(dxgiMod);
		return E_NOTIMPL;
	}

	// Actually create the factory and return
	HRESULT res = CreateDXGIFactory1Dyn(
		__uuidof(IDXGIFactory1), (void **)factoryOut);
	if(dxgiMod != NULL)
		FreeLibrary(dxgiMod);
	return res;
}

static void logAdapterDesc(int i, PCIIDParser *pciid, IDXGIAdapter *adapter)
{
	DXGI_ADAPTER_DESC desc;
	HRESULT res = adapter->GetDesc(&desc);
	if(FAILED(res)) {
		gfxLog()
			<< QStringLiteral(" %1: Failed to get description").arg(i);
		adapter->Release();
		return;
	}

	// Adapter "description" (Not valid for feature level 9 hardware)
	QString str = QString::fromUtf16(desc.Description);
	if(str == QStringLiteral("Software Adapter"))
		str += QStringLiteral(" (Feature level 9 hardware)");
	gfxLog() << QStringLiteral(" %1: Description: %2").arg(i).arg(str);

	// Device PCI IDs
	QString vendor, device, subSys;
	pciid->lookup(
		desc.VendorId, desc.DeviceId, desc.SubSysId, vendor, device,
		subSys);
	if(vendor.isEmpty())
		vendor = QStringLiteral("ID=%1").arg(desc.VendorId);
	if(device.isEmpty())
		device = QStringLiteral("ID=%1").arg(desc.DeviceId);
	if(subSys.isEmpty())
		subSys = QStringLiteral("ID=%1").arg(desc.SubSysId);
	gfxLog() << QStringLiteral("    Vendor: %1")
		.arg(vendor);
	gfxLog() << QStringLiteral("    Device: %1")
		.arg(device);
	gfxLog() << QStringLiteral("    Subsystem: %1")
		.arg(subSys);
	gfxLog() << QStringLiteral("    Revision: %1")
		.arg(desc.Revision);

	// Memory amounts
	gfxLog() << QStringLiteral("    Dedicated video memory: %1B")
		.arg(humanBitsBytes(desc.DedicatedVideoMemory, 2));
	gfxLog() << QStringLiteral("    Dedicated system memory: %1B")
		.arg(humanBitsBytes(desc.DedicatedSystemMemory, 2));
	gfxLog() << QStringLiteral("    Shared system memory: %1B")
		.arg(humanBitsBytes(desc.SharedSystemMemory, 2));

	adapter->Release();
}

/// <summary>
/// Queries the system for a list of display adapters and logs the results.
/// </summary>
void D3DContext::logDisplayAdapters()
{
	// WARNING: We must not mix `IDXGIFactory` and `IDXGIFactory1` in the same
	// process!

	IDXGIFactory *factory = NULL;
	IDXGIFactory1 *factory1 = NULL;
	HRESULT res = createDXGIFactory1Dynamic(&factory1);
	if(factory1 == NULL)
		res = CreateDXGIFactory(__uuidof(IDXGIFactory), (void **)&factory);
	if(FAILED(res)) {
		gfxLog(GfxLog::Warning) << QStringLiteral(
			"Failed to create DXGI factory. Reason = %1")
			.arg(getDXErrorCode(res));
		return;
	}

	// Initialize PCI ID database
	PCIIDParser pciid(":/Libvidgfx/Resources/pci.ids");

	if(factory1 != NULL)
		gfxLog() << QStringLiteral("Using DXGI 1.1 factories");
	else
		gfxLog() << QStringLiteral("Using DXGI 1.0 factories");

	gfxLog() << QStringLiteral("Available graphics adapters:");
	if(factory1 != NULL) {
		// DXGI 1.1
		IDXGIAdapter1 *adapter = NULL;
		for(uint i = 0;
			factory1->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND;
			i++)
		{
			logAdapterDesc(i, &pciid, adapter);
		}
	} else {
		// DXGI 1.0
		IDXGIAdapter *adapter = NULL;
		for(uint i = 0;
			factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND;
			i++)
		{
			logAdapterDesc(i, &pciid, adapter);
		}
	}

	if(factory1 != NULL)
		factory1->Release();
	if(factory != NULL)
		factory->Release();
}

D3DContext::D3DContext()
	: GraphicsContext()
	, m_hasDxgi11(false)
	, m_hasDxgi11Valid(false)
	, m_hasBgraTexSupport(false)
	, m_hasBgraTexSupportValid(false)
	, m_swapChain(NULL)
	, m_device(NULL)
	, m_rasterizerState(NULL)
	, m_pointClampSampler(NULL)
	, m_bilinearClampSampler(NULL)
	, m_resizeSampler(NULL)
	, m_noBlend(NULL)
	, m_alphaBlend(NULL)
	, m_premultiBlend(NULL)

	// Render targets
	, m_screenTarget(NULL)
	, m_screenTargetSize(0, 0)
	, m_canvas1Texture(NULL)
	, m_canvas2Texture(NULL)
	, m_canvasTargetSize(0, 0)
	, m_scratch1Texture(NULL)
	, m_scratch2Texture(NULL)
	, m_scratchTargetSize(0, 0)
	, m_scratchNextTarget(0)

	// Constant buffers
	//, m_cameraConstantsLocal() // Compiler warning if this is uncommented
	, m_cameraConstants(NULL)
	//, m_resizeConstantsLocal()
	, m_resizeConstants(NULL)
	//, m_rgbNv16ConstantsLocal()
	, m_rgbNv16Constants(NULL)
	//, m_texDecalConstantsLocal()
	, m_texDecalConstants(NULL)
	, m_texDecalFlags(0)

	// Shaders
	, m_boundShader(GfxNoShader)
	, m_solidVS(NULL)
	, m_solidPS(NULL)
	, m_solidIL(NULL)
	, m_texDecalVS(NULL)
	, m_texDecalPS(NULL)
	, m_texDecalGbcsPS(NULL)
	, m_texDecalRgbPS(NULL)
	, m_texDecalIL(NULL)
	, m_resizeVS(NULL)
	, m_resizePS(NULL)
	, m_resizeIL(NULL)
	, m_rgbNv16PS(NULL)
	, m_yv12RgbPS(NULL)
	, m_UyvyRgbPS(NULL)
	, m_HdycRgbPS(NULL)
	, m_Yuy2RgbPS(NULL)

	// Advanced rendering
	, m_mipmapBuf(NULL)
{
	memset(m_cameraConstantsLocal, 0, sizeof(m_cameraConstantsLocal));
	memset(m_resizeConstantsLocal, 0, sizeof(m_resizeConstantsLocal));
	memset(m_rgbNv16ConstantsLocal, 0, sizeof(m_rgbNv16ConstantsLocal));
	memset(m_texDecalConstantsLocal, 0, sizeof(m_texDecalConstantsLocal));
}

D3DContext::~D3DContext()
{
	// Only continue if DirectX actually initialized
	if(m_device == NULL || m_swapChain == NULL)
		return;

	// Emit destroyed signal so that other parts of the application can cleanly
	// release their hardware resources
	emit destroying(this);

	// Release advanced rendering objects
	deleteVertexBuffer(m_mipmapBuf);

	// Release constant buffers
	if(m_cameraConstants)
		m_cameraConstants->Release();
	if(m_resizeConstants)
		m_resizeConstants->Release();
	if(m_rgbNv16Constants)
		m_rgbNv16Constants->Release();
	if(m_texDecalConstants)
		m_texDecalConstants->Release();

	// Release shaders
	if(m_solidVS)
		m_solidVS->Release();
	if(m_solidPS)
		m_solidPS->Release();
	if(m_solidIL)
		m_solidIL->Release();
	if(m_texDecalVS)
		m_texDecalVS->Release();
	if(m_texDecalPS)
		m_texDecalPS->Release();
	if(m_texDecalGbcsPS)
		m_texDecalGbcsPS->Release();
	if(m_texDecalRgbPS)
		m_texDecalRgbPS->Release();
	if(m_texDecalIL)
		m_texDecalIL->Release();
	if(m_resizeVS)
		m_resizeVS->Release();
	if(m_resizePS)
		m_resizePS->Release();
	if(m_resizeIL)
		m_resizeIL->Release();
	if(m_rgbNv16PS)
		m_rgbNv16PS->Release();
	if(m_yv12RgbPS)
		m_yv12RgbPS->Release();
	if(m_UyvyRgbPS)
		m_UyvyRgbPS->Release();
	if(m_HdycRgbPS)
		m_HdycRgbPS->Release();
	if(m_Yuy2RgbPS)
		m_Yuy2RgbPS->Release();

	// Release render targets
	ID3D10RenderTargetView *nullView[2] = { NULL, NULL };
	m_device->OMSetRenderTargets(2, nullView, NULL);
	if(m_screenTarget)
		m_screenTarget->Release();

	// Release textures
	delete m_canvas1Texture;
	delete m_canvas2Texture;
	delete m_scratch1Texture;
	delete m_scratch2Texture;

	// Release sampler states
	if(m_pointClampSampler)
		m_pointClampSampler->Release();
	if(m_bilinearClampSampler)
		m_bilinearClampSampler->Release();
	if(m_resizeSampler)
		m_resizeSampler->Release();

	// Release blend states
	if(m_noBlend)
		m_noBlend->Release();
	if(m_alphaBlend)
		m_alphaBlend->Release();
	if(m_premultiBlend)
		m_premultiBlend->Release();

	// Release rasterizer
	if(m_rasterizerState)
		m_rasterizerState->Release();

	// Release device and swap chain
	m_device->Release();
	m_swapChain->Release();
	m_device = NULL;
	m_swapChain = NULL;
}

/// <summary>
/// Finds the first graphics adapter on the system and constructs the adapter
/// object in such a way that works with the Windows 8 duplicator (Device must
/// be constructed from a `IDXGIFactory1` object). If this method returns
/// non-NULL then the caller must manually release the object when it is
/// finished with it. WARNING: Only attempts to create a DXGI 1.1 adapter on
/// Windows 8 and later.
/// </summary>
IDXGIAdapter *D3DContext::getFirstDxgi11Adapter()
{
	if(!IsWindows8OrGreater())
		return NULL;

	// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
	//-------------------------------------------------------------------------
	// Mixing `CreateDXGIFactory()` and `CreateDXGIFactory1()` in the same
	// process is not supported! We also can't staticly link to
	// `CreateDXGIFactory1()` as it'll prevent the application from running on
	// Windows Vista!
	//-------------------------------------------------------------------------
	// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING

	IDXGIFactory1 *factory = NULL;
	HRESULT res = createDXGIFactory1Dynamic(&factory);
	if(FAILED(res)) {
		if(res != E_NOTIMPL) {
			gfxLog(LOG_CAT, GfxLog::Warning) << QStringLiteral(
				"Failed to create DXGI 1.1 factory. Reason = %1")
				.arg(getDXErrorCode(res));
		}
		return NULL;
	}

	//gfxLog(LOG_CAT) << "Creating DXGI 1.1 adapter";
	IDXGIAdapter1 *adapter = NULL;
	res = factory->EnumAdapters1(0, &adapter);
	if(res == DXGI_ERROR_NOT_FOUND) {
		// No adapters found
	}
	if(adapter == NULL) {
		gfxLog(LOG_CAT, GfxLog::Warning) << QStringLiteral(
			"Failed to find the first DXGI 1.1 adapter");
	}

	factory->Release();
	return adapter;
}

bool D3DContext::initialize(
	HWND hwnd, const QSize &size, const QColor &resizeBorderCol)
{
	// Notes about compatibility:
	//
	// Dispite MSDN claiming otherwise if a system has DirectX 10.1 installed
	// using the DirectX 10.0 API doesn't create an identical 10.1 device with
	// the 10.0 or 10.1 feature level (Depending on hardware support). In fact
	// on one test system (Windows 7 with both GeForce GTS 450 and 9800 GT
	// cards) shaders that were compiled using "shader model 4.0 level 9.3"
	// would not load on a 10.0 device but would load on a 10.1 level 10.0
	// device. On another test system (Windows 8 on a mid-2012 MacBook Pro)
	// BGRA textures were supported on a 10.1 level 10.0 device but not on a
	// 10.0 device.
	//
	// We want to support DirectX 10 level 9 hardware but Windows Vista gold
	// (No service packs) does not support the DirectX 10.1 API which is
	// required for feature levels. The API was first introduced in Windows 7
	// and Windows Vista SP2. In order to work on Windows Vista gold we must
	// attempt to link to DirectX 10.1 at runtime and not compiletime.
	//
	// Creating a DirectX 10 level 9 device forces the API to enter
	// compatibility mode which has slightly different characteristics than
	// DirectX 10 even on DirectX 10 hardware. For this reason we attempt to
	// create a level 10.0 device first.
	//
	// We require a minimum of DirectX 10 level 9.3 support as level 9.2 isn't
	// guarenteed to support multiple render targets.

	//-------------------------------------------------------------------------
	// Create device and swap chain

	// Attempt to dynamically load DirectX 10.1 if it's available
	HMODULE d3d101Mod = LoadLibrary(TEXT("d3d10_1.dll"));
	PFN_D3D10_CREATE_DEVICE_AND_SWAP_CHAIN1
		D3D10CreateDeviceAndSwapChain1 = NULL;
	if(d3d101Mod != NULL) {
		D3D10CreateDeviceAndSwapChain1 =
			(PFN_D3D10_CREATE_DEVICE_AND_SWAP_CHAIN1)
			GetProcAddress(d3d101Mod, "D3D10CreateDeviceAndSwapChain1");
	}

	// Setup swap chain description
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	memset(&swapChainDesc, 0, sizeof(swapChainDesc));
	swapChainDesc.BufferDesc.Width = size.width();
	swapChainDesc.BufferDesc.Height = size.height();
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 0; // We're never fullscreen
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 0;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.ScanlineOrdering =
		DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swapChainDesc.SampleDesc.Count = 1; // No anti-aliasing
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.OutputWindow = hwnd;
	swapChainDesc.Windowed = TRUE;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Flags = 0;

	// Create device and swap chain. Currently always uses the primary adapter
	UINT deviceFlags = D3D10_CREATE_DEVICE_SINGLETHREADED;
	HRESULT reason10level10 = S_OK;
	HRESULT reason10level9 = S_OK;
	bool attempted10level10 = false;
	bool attempted10level9 = false;
	m_device = NULL; // Should already be NULL but just make sure
	if(D3D10CreateDeviceAndSwapChain1 != NULL) {
		// DirectX 10.1 is available, use it

		// We want to use a DXGI 1.1 adapter on Windows 8 and later in order
		// for the monitor duplicator to work. This method returns NULL on
		// older versions of Windows or if there was an error getting an
		// adapter. If we attempt to create a device with a NULL adapter then
		// the OS will automatically find the "first adapter" for us.
		IDXGIAdapter *adapter = getFirstDxgi11Adapter();

		// Attempt to create a feature level 10.0 device first
		ID3D10Device1 *d3d101Dev = NULL;
#if FORCE_DIRECTX_10_1_LEVEL_9_3
		gfxLog(LOG_CAT, GfxLog::Warning) << "Forcing DirectX 10.1 Level 9.3";
		HRESULT res = E_FAIL;
#else
		HRESULT res = D3D10CreateDeviceAndSwapChain1(
			adapter, // _In_ IDXGIAdapter *pAdapter,
			D3D10_DRIVER_TYPE_HARDWARE, // _In_ D3D10_DRIVER_TYPE DriverType,
			NULL, // _In_ HMODULE Software,
			deviceFlags, // _In_ UINT Flags,
			D3D10_FEATURE_LEVEL_10_0, // _In_ D3D10_FEATURE_LEVEL1 HardwareLevel,
			D3D10_1_SDK_VERSION, // _In_ UINT SDKVersion,
			&swapChainDesc, // _In_ DXGI_SWAP_CHAIN_DESC *pSwapChainDesc,
			&m_swapChain, // _Out_ IDXGISwapChain **ppSwapChain,
			&d3d101Dev); // _Out_ ID3D10Device1 **ppDevice
#endif // FORCE_DIRECTX_10_1_LEVEL_9_3
		reason10level10 = res;
		attempted10level10 = true;
		if(FAILED(res)) {
			// Attempt to create a feature level 9.3 device if that failed
			res = D3D10CreateDeviceAndSwapChain1(
				adapter, // _In_ IDXGIAdapter *pAdapter,
				D3D10_DRIVER_TYPE_HARDWARE, // _In_ D3D10_DRIVER_TYPE DriverType,
				NULL, // _In_ HMODULE Software,
				deviceFlags, // _In_ UINT Flags,
				D3D10_FEATURE_LEVEL_9_3, // _In_ D3D10_FEATURE_LEVEL1 HardwareLevel,
				D3D10_1_SDK_VERSION, // _In_ UINT SDKVersion,
				&swapChainDesc, // _In_ DXGI_SWAP_CHAIN_DESC *pSwapChainDesc,
				&m_swapChain, // _Out_ IDXGISwapChain **ppSwapChain,
				&d3d101Dev); // _Out_ ID3D10Device1 **ppDevice
			reason10level9 = res;
			attempted10level9 = true;

			// If this failed then we will attempt to create a device using the
			// DirectX 10.0 API below
		}
		if(SUCCEEDED(res)) {
			// A device was created with either feature level, convert the
			// DirectX 10.1 device to a DirectX 10 device
			res = d3d101Dev->QueryInterface(
				__uuidof(ID3D10Device), (void **)&m_device);
			if(SUCCEEDED(res)) {
				d3d101Dev->Release(); // `QueryInterface()` adds a reference

				// Log what sort of device we created
				if(attempted10level9)
					gfxLog(LOG_CAT) << "Using DirectX 10.1 Level 9.3";
				else
					gfxLog(LOG_CAT) << "Using DirectX 10.1 Level 10.0";
			} else {
				m_device->Release();
				m_swapChain->Release();
				m_device = NULL;
				m_swapChain = NULL;
			}
		}

		if(adapter != NULL)
			adapter->Release();
	}
	if(m_device == NULL) {
		// DirectX 10.1 is not available or creating a DirectX 10.1 device
		// failed, use DirectX 10.0
		HRESULT res = D3D10CreateDeviceAndSwapChain(
			NULL, // _In_ IDXGIAdapter *pAdapter,
			D3D10_DRIVER_TYPE_HARDWARE, // _In_ D3D10_DRIVER_TYPE DriverType,
			NULL, // _In_ HMODULE Software,
			deviceFlags, // _In_ UINT Flags,
			D3D10_SDK_VERSION, // _In_ UINT SDKVersion,
			&swapChainDesc, // _In_ DXGI_SWAP_CHAIN_DESC *pSwapChainDesc,
			&m_swapChain, // _Out_ IDXGISwapChain **ppSwapChain,
			&m_device); // _Out_ ID3D10Device **ppDevice
		if(FAILED(res)) {
			// All attempts to create a device failed
			gfxLog(LOG_CAT, GfxLog::Critical)
				<< "Failed to create DirectX device and swap chain, cannot continue.";
			if(D3D10CreateDeviceAndSwapChain1 == NULL) {
				gfxLog(LOG_CAT, GfxLog::Critical)
					<< "DirectX 10.1 was skipped as it was not available";
			} else {
				gfxLog(LOG_CAT, GfxLog::Critical)
					<< "DirectX 10.1 level 10.0 failure reason = "
					<< getDXErrorCode(reason10level10);
				gfxLog(LOG_CAT, GfxLog::Critical)
					<< "DirectX 10.1 level 9.3 failure reason = "
					<< getDXErrorCode(reason10level9);
			}
			gfxLog(LOG_CAT, GfxLog::Critical)
				<< "DirectX 10.0 failure reason = " << getDXErrorCode(res);
		} else {
			// Log what sort of device we created
			gfxLog(LOG_CAT) << "Using DirectX 10.0";
		}
	}

	//-------------------------------------------------------------------------
	// Initial state

	// Create a render target for the swap chain buffer
	m_screenTargetSize = size;
	if(!createScreenTarget())
		return false;

	// Create rasterizer state
	D3D10_RASTERIZER_DESC desc;
	desc.FillMode = D3D10_FILL_SOLID;
	desc.CullMode = D3D10_CULL_NONE; // Makes it easier to display stuff
	desc.FrontCounterClockwise = FALSE;
	desc.DepthBias = 0;
	desc.DepthBiasClamp = 0.0f;
	desc.SlopeScaledDepthBias = 0.0f;
	desc.DepthClipEnable = TRUE; // Must be true for DX10 Level 9 support
	desc.ScissorEnable = FALSE;
	desc.MultisampleEnable = FALSE;
	desc.AntialiasedLineEnable = FALSE;
	HRESULT res = m_device->CreateRasterizerState(&desc, &m_rasterizerState);
	if(FAILED(res)) {
		gfxLog(LOG_CAT, GfxLog::Critical)
			<< "Failed to create rasterizer state, cannot continue. "
			<< "Reason = " << getDXErrorCode(res);
		return false;
	}
	m_device->RSSetState(m_rasterizerState);

	// Create sampler states
	D3D10_SAMPLER_DESC sampDesc;
	sampDesc.Filter = D3D10_FILTER_MIN_MAG_MIP_POINT;
	sampDesc.AddressU = D3D10_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D10_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D10_TEXTURE_ADDRESS_CLAMP;
	sampDesc.MipLODBias = 0.0f;
	sampDesc.MaxAnisotropy = 0;
	sampDesc.ComparisonFunc = D3D10_COMPARISON_NEVER;
	sampDesc.BorderColor[0] = 0.0f;
	sampDesc.BorderColor[1] = 0.0f;
	sampDesc.BorderColor[2] = 0.0f;
	sampDesc.BorderColor[3] = 0.0f;
	sampDesc.MinLOD = 0.0f;
	sampDesc.MaxLOD = D3D10_FLOAT32_MAX;
	res = m_device->CreateSamplerState(&sampDesc, &m_pointClampSampler);
	if(FAILED(res)) {
		gfxLog(LOG_CAT, GfxLog::Critical)
			<< "Failed to create clamped point sampler state, cannot "
			<< "continue. Reason = " << getDXErrorCode(res);
		return false;
	}
	sampDesc.Filter = D3D10_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	res = m_device->CreateSamplerState(&sampDesc, &m_bilinearClampSampler);
	if(FAILED(res)) {
		gfxLog(LOG_CAT, GfxLog::Critical)
			<< "Failed to create clamped bilinear sampler state, cannot "
			<< "continue. Reason = " << getDXErrorCode(res);
		return false;
	}
	sampDesc.AddressU = D3D10_TEXTURE_ADDRESS_BORDER;
	sampDesc.AddressV = D3D10_TEXTURE_ADDRESS_BORDER;
	sampDesc.AddressW = D3D10_TEXTURE_ADDRESS_BORDER;
	sampDesc.BorderColor[0] = resizeBorderCol.redF();
	sampDesc.BorderColor[1] = resizeBorderCol.greenF();
	sampDesc.BorderColor[2] = resizeBorderCol.blueF();
	sampDesc.BorderColor[3] = resizeBorderCol.alphaF();
	res = m_device->CreateSamplerState(&sampDesc, &m_resizeSampler);
	if(FAILED(res)) {
		gfxLog(LOG_CAT, GfxLog::Critical)
			<< "Failed to create resize layer sampler state, cannot "
			<< "continue. Reason = " << getDXErrorCode(res);
		return false;
	}
	setTextureFilter(GfxBilinearFilter); // Bilinear by default

	// Create blend states
	D3D10_BLEND_DESC blendDesc;
	blendDesc.AlphaToCoverageEnable = FALSE;
	for(int i = 0; i < 8; i++)
		blendDesc.BlendEnable[i] = FALSE;
	blendDesc.SrcBlend = D3D10_BLEND_ONE;
	blendDesc.DestBlend = D3D10_BLEND_ZERO;
	blendDesc.BlendOp = D3D10_BLEND_OP_ADD;
	blendDesc.SrcBlendAlpha = D3D10_BLEND_ONE;
	blendDesc.DestBlendAlpha = D3D10_BLEND_ZERO;
	blendDesc.BlendOpAlpha = D3D10_BLEND_OP_ADD;
	for(int i = 0; i < 8; i++)
		blendDesc.RenderTargetWriteMask[i] = D3D10_COLOR_WRITE_ENABLE_ALL;
	res = m_device->CreateBlendState(&blendDesc, &m_noBlend);
	if(FAILED(res)) {
		gfxLog(LOG_CAT, GfxLog::Critical)
			<< "Failed to create default blending state, cannot "
			<< "continue. Reason = " << getDXErrorCode(res);
		return false;
	}
	for(int i = 0; i < 8; i++)
		blendDesc.BlendEnable[i] = TRUE;
	blendDesc.SrcBlend = D3D10_BLEND_SRC_ALPHA;
	blendDesc.DestBlend = D3D10_BLEND_INV_SRC_ALPHA;
	res = m_device->CreateBlendState(&blendDesc, &m_alphaBlend);
	if(FAILED(res)) {
		gfxLog(LOG_CAT, GfxLog::Critical)
			<< "Failed to create alpha blending state, cannot "
			<< "continue. Reason = " << getDXErrorCode(res);
		return false;
	}
	blendDesc.SrcBlend = D3D10_BLEND_ONE;
	blendDesc.DestBlend = D3D10_BLEND_INV_SRC_ALPHA;
	res = m_device->CreateBlendState(&blendDesc, &m_premultiBlend);
	if(FAILED(res)) {
		gfxLog(LOG_CAT, GfxLog::Critical)
			<< "Failed to create premultiplied alpha blending state, cannot "
			<< "continue. Reason = " << getDXErrorCode(res);
		return false;
	}
	setBlending(GfxNoBlending); // No blending by default

	// If we ever need a depth stencil view or depth stencil create them here

	// Bind the screen render target by default
	setRenderTarget(GfxScreenTarget);

	gfxLog(LOG_CAT) << "Successfully initialized DirectX";

	//-------------------------------------------------------------------------
	// Create shader objects

	if(!createShaders())
		return false;
	//gfxLog(LOG_CAT) << "Successfully created DirectX shaders";

	//-------------------------------------------------------------------------
	// Create camera cbuffer

	// Upload local RAM
	m_cameraConstantsDirty = true; // Force update
	updateCameraConstants();

	// Create hardware buffer
	D3D10_BUFFER_DESC bufDesc;
	bufDesc.ByteWidth = sizeof(m_cameraConstantsLocal);
	bufDesc.Usage = D3D10_USAGE_DYNAMIC;
	bufDesc.BindFlags = D3D10_BIND_CONSTANT_BUFFER;
	bufDesc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
	bufDesc.MiscFlags = 0;
	if(!createDXBuffer(m_device, &bufDesc, m_cameraConstantsLocal,
		&m_cameraConstants)) {
			// Failed to create buffer
			return false;
	}

	//-------------------------------------------------------------------------
	// Create resize layer cbuffer

	// Upload local RAM
	m_resizeConstantsDirty = true; // Force update
	updateResizeConstants();

	// Create hardware buffer
	bufDesc.ByteWidth = sizeof(m_resizeConstantsLocal);
	bufDesc.Usage = D3D10_USAGE_DYNAMIC;
	bufDesc.BindFlags = D3D10_BIND_CONSTANT_BUFFER;
	bufDesc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
	bufDesc.MiscFlags = 0;
	if(!createDXBuffer(m_device, &bufDesc, m_resizeConstantsLocal,
		&m_resizeConstants)) {
			// Failed to create buffer
			return false;
	}

	//-------------------------------------------------------------------------
	// Create RGB->NV16 converter cbuffer

	// Upload local RAM
	m_rgbNv16ConstantsDirty = true; // Force update
	updateRgbNv16Constants();

	// Create hardware buffer
	bufDesc.ByteWidth = sizeof(m_rgbNv16ConstantsLocal);
	bufDesc.Usage = D3D10_USAGE_DYNAMIC;
	bufDesc.BindFlags = D3D10_BIND_CONSTANT_BUFFER;
	bufDesc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
	bufDesc.MiscFlags = 0;
	if(!createDXBuffer(m_device, &bufDesc, m_rgbNv16ConstantsLocal,
		&m_rgbNv16Constants)) {
			// Failed to create buffer
			return false;
	}

	//-------------------------------------------------------------------------
	// Create texture decal cbuffer

	// Upload local RAM
	m_texDecalConstantsDirty = true; // Force update
	updateTexDecalConstants();

	// Create hardware buffer
	bufDesc.ByteWidth = sizeof(m_texDecalConstantsLocal);
	bufDesc.Usage = D3D10_USAGE_DYNAMIC;
	bufDesc.BindFlags = D3D10_BIND_CONSTANT_BUFFER;
	bufDesc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
	bufDesc.MiscFlags = 0;
	if(!createDXBuffer(m_device, &bufDesc, m_texDecalConstantsLocal,
		&m_texDecalConstants)) {
			// Failed to create buffer
			return false;
	}

	//-------------------------------------------------------------------------
	// Set the scratch target's initial size

	m_scratchNextTarget = 0;
	resizeScratchTarget(QSize(512, 512));

	//-------------------------------------------------------------------------
	// Create advanced rendering objects

	m_mipmapBuf = createVertexBuffer(TexDecalRectBufSize);

	//-------------------------------------------------------------------------
	// Emit initialized signal

	// The context is now fully initialized and other parts of the application
	// can begin to create hardware resources. Emit a signal so they know.
	emit initialized(this);

	return true;
}

/// <summary>
/// DXGI 1.1 provides additional features over 1.0 which are useful to us such
/// as GDI textures. As DXGI 1.1 is not available in Windows Vista if the
/// platform update isn't installed we need to test if it's actually available
/// or not.
/// </summary>
bool D3DContext::hasDxgi11()
{
	if(m_hasDxgi11Valid)
		return m_hasDxgi11;

	// We assume that the canvas texture has been created already
	// TODO: Is there a simple "get DXGI version number" function somewhere?
	if(m_canvas1Texture == NULL)
		return false;

	// Test to see if DXGI 1.1 is available by trying to get an interface that
	// is not available in DXGI 1.0
	IDXGISurface1 *surface = NULL;
	HRESULT res = m_canvas1Texture->getTexture()->QueryInterface(
		__uuidof(IDXGISurface1), (void **)(&surface));
	if(FAILED(res)) {
		gfxLog(LOG_CAT) << QStringLiteral("DXGI version: 1.0");
		m_hasDxgi11 = false;
	} else {
		gfxLog(LOG_CAT) << QStringLiteral("DXGI version: 1.1 or later");
		m_hasDxgi11 = true;
		surface->Release();
	}
	m_hasDxgi11Valid = true;

	// Notify that the value has potentially changed
	emit hasDxgi11Changed(m_hasDxgi11);

	// Test for BGRA texture support if we haven't done already in order for it
	// to appear next to DXGI version in the log
	hasBgraTexSupport();

	return m_hasDxgi11;
}

bool D3DContext::hasBgraTexSupport()
{
	if(m_hasBgraTexSupportValid)
		return m_hasBgraTexSupport;

	// Test for DXGI version if we haven't done already in order for it to
	// appear next to BGRA texture support in the log
	hasDxgi11();

	// Test if BGRA textures are supported
	UINT support = 0;
	HRESULT res =
		m_device->CheckFormatSupport(DXGI_FORMAT_B8G8R8A8_UNORM, &support);
	if(FAILED(res))
		support = 0; // Should never happen
	if(support & D3D10_FORMAT_SUPPORT_TEXTURE2D) {
		m_hasBgraTexSupport = true;
		gfxLog(LOG_CAT) << QStringLiteral("BGRA textures: Supported");
	} else {
		m_hasBgraTexSupport = false;
		gfxLog(LOG_CAT) << QStringLiteral("BGRA textures: Not supported");
	}
	m_hasBgraTexSupportValid = true;

#if FORCE_NO_BGRA_SUPPORT
	gfxLog(LOG_CAT, GfxLog::Critical)
		<< QStringLiteral("Forcing no BGRA texture support");
	m_hasBgraTexSupport = false;
#endif // FORCE_NO_BGRA_SUPPORT

	// Notify that the value has potentially changed
	emit hasBgraTexSupportChanged(m_hasBgraTexSupport);

	return m_hasBgraTexSupport;
}

bool D3DContext::createScreenTarget()
{
	ID3D10Texture2D *backBuf;
	HRESULT res = m_swapChain->GetBuffer(
		0, __uuidof(backBuf), reinterpret_cast<void **>(&backBuf));
	if(FAILED(res)) {
		gfxLog(LOG_CAT, GfxLog::Critical)
			<< "Failed to get the DirectX back buffer, cannot continue. "
			<< "Reason = " << getDXErrorCode(res);
		return false;
	}
	res = m_device->CreateRenderTargetView(backBuf, NULL, &m_screenTarget);
	if(FAILED(res)) {
		backBuf->Release();
		gfxLog(LOG_CAT, GfxLog::Critical)
			<< "Failed to create a render target for the back buffer, cannot "
			<< "continue. Reason = " << getDXErrorCode(res);
		return false;
	}
	backBuf->Release();
	return true;
}

bool D3DContext::createShaders()
{
	// Solid colour shaders
	const D3D10_INPUT_ELEMENT_DESC solidILDesc[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D10_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D10_INPUT_PER_VERTEX_DATA, 0},
	};
	if(!createVertexShaderAndInputLayout(
		"solid-vs", &m_solidVS, &m_solidIL, solidILDesc, 2))
		return false;
	if(!createPixelShader("solid-ps", &m_solidPS))
		return false;

	// Texture decal shaders
	const D3D10_INPUT_ELEMENT_DESC texDecalILDesc[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D10_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D10_INPUT_PER_VERTEX_DATA, 0},
	};
	if(!createVertexShaderAndInputLayout(
		"texDecal-vs", &m_texDecalVS, &m_texDecalIL, texDecalILDesc, 2))
		return false;
	if(!createPixelShader("texDecal-ps", &m_texDecalPS))
		return false;
	if(!createPixelShader("texDecalGbcs-ps", &m_texDecalGbcsPS))
		return false;
	if(!createPixelShader("texDecalRgb-ps", &m_texDecalRgbPS))
		return false;

	// Resize layer shaders
	const D3D10_INPUT_ELEMENT_DESC resizeILDesc[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D10_INPUT_PER_VERTEX_DATA, 0}
	};
	if(!createVertexShaderAndInputLayout(
		"resize-vs", &m_resizeVS, &m_resizeIL, resizeILDesc, 1))
		return false;
	if(!createPixelShader("resize-ps", &m_resizePS))
		return false;

	// Colour conversion shaders
	if(!createPixelShader("rgb-nv16-ps", &m_rgbNv16PS))
		return false;
	if(!createPixelShader("yv12-rgb-ps", &m_yv12RgbPS))
		return false;
	if(!createPixelShader("uyvy-rgb-ps", &m_UyvyRgbPS))
		return false;
	if(!createPixelShader("hdyc-rgb-ps", &m_HdycRgbPS))
		return false;
	if(!createPixelShader("yuy2-rgb-ps", &m_Yuy2RgbPS))
		return false;

	return true;
}

bool D3DContext::createVertexShaderAndInputLayout(
	const QString &shaderName, ID3D10VertexShader **shader,
	ID3D10InputLayout **layout, const D3D10_INPUT_ELEMENT_DESC *layoutDesc,
	int layoutDescSize)
{
	QByteArray data = getShaderFileData(shaderName);
	if(data.isEmpty()) {
		gfxLog(LOG_CAT, GfxLog::Critical)
			<< "Failed to read vertex shader \"" << shaderName
			<< "\", cannot continue";
		return false;
	}

	HRESULT res =
		m_device->CreateVertexShader(data.data(), data.length(), shader);
	if(FAILED(res)) {
		gfxLog(LOG_CAT, GfxLog::Critical)
			<< "Failed to load vertex shader \"" << shaderName
			<< "\", cannot continue. Reason = " << getDXErrorCode(res);
		return false;
	}

	// Shader created, create matching input layout
	res = m_device->CreateInputLayout(
		layoutDesc, layoutDescSize, data.data(), data.length(),
		layout);
	if(FAILED(res)) {
		gfxLog(LOG_CAT, GfxLog::Critical)
			<< "Failed to create input layout for \"" << shaderName
			<< "\", cannot continue. Reason = " << getDXErrorCode(res);
		(*shader)->Release();
		*shader = NULL;
		return false;
	}

	return true;
}

bool D3DContext::createPixelShader(
	const QString &shaderName, ID3D10PixelShader **shader)
{
	QByteArray data = getShaderFileData(shaderName);
	if(data.isEmpty()) {
		gfxLog(LOG_CAT, GfxLog::Critical)
			<< "Failed to read pixel shader \"" << shaderName
			<< "\", cannot continue";
		return false;
	}

	HRESULT res =
		m_device->CreatePixelShader(data.data(), data.length(), shader);
	if(FAILED(res)) {
		gfxLog(LOG_CAT, GfxLog::Critical)
			<< "Failed to load pixel shader \"" << shaderName
			<< "\", cannot continue. Reason = " << getDXErrorCode(res);
		return false;
	}

	return true;
}

/// <summary>
/// Reads the entire compiled shader file into memory and returns it as a
/// `QByteArray` buffer.
/// </summary>
QByteArray D3DContext::getShaderFileData(const QString &shaderName) const
{
	// All shader files are stored in the executable as a compressed resource
	// so it is safe to use Qt's synchronous API to access them
	QFile file(":/Libvidgfx/Shaders/" + shaderName + ".cso");
	if(!file.open(QIODevice::ReadOnly))
		return QByteArray();
	QByteArray data = file.readAll();
	file.close();
	return data;
}

void D3DContext::updateCameraConstants()
{
	if(!m_cameraConstantsDirty)
		return; // Nothing to do

	// Update local memory
	switch(m_currentTarget) {
	default:
	case GfxScreenTarget:
		m_screenViewMat.copyDataTo(&m_cameraConstantsLocal[0]);
		m_screenProjMat.copyDataTo(&m_cameraConstantsLocal[16]);
		break;
	case GfxCanvas1Target:
	case GfxCanvas2Target:
		m_canvasViewMat.copyDataTo(&m_cameraConstantsLocal[0]);
		m_canvasProjMat.copyDataTo(&m_cameraConstantsLocal[16]);
		break;
	case GfxScratch1Target:
	case GfxScratch2Target:
		m_scratchViewMat.copyDataTo(&m_cameraConstantsLocal[0]);
		m_scratchProjMat.copyDataTo(&m_cameraConstantsLocal[16]);
		break;
	case GfxUserTarget:
		m_userViewMat.copyDataTo(&m_cameraConstantsLocal[0]);
		m_userProjMat.copyDataTo(&m_cameraConstantsLocal[16]);
		break;
	}

	// Update hardware buffer
	if(m_cameraConstants) {
		if(!updateDXBuffer(
			m_device, m_cameraConstants, m_cameraConstantsLocal,
			sizeof(m_cameraConstantsLocal)))
		{
			return; // Update failed
		}
	}

	m_cameraConstantsDirty = false;
}

void D3DContext::updateResizeConstants()
{
	if(!m_resizeConstantsDirty)
		return; // Nothing to do

	// Update local memory
	m_resizeConstantsLocal[0] = m_resizeRect.x();
	m_resizeConstantsLocal[1] = m_resizeRect.y();
	m_resizeConstantsLocal[2] = m_resizeRect.width();
	m_resizeConstantsLocal[3] = m_resizeRect.height();

	// Update hardware buffer
	if(m_resizeConstants) {
		if(!updateDXBuffer(
			m_device, m_resizeConstants, m_resizeConstantsLocal,
			sizeof(m_resizeConstantsLocal)))
		{
			return; // Update failed
		}
	}

	m_resizeConstantsDirty = false;
}

void D3DContext::updateRgbNv16Constants()
{
	if(!m_rgbNv16ConstantsDirty)
		return; // Nothing to do

	// Calculate values
	float aOff = -1.5f * m_rgbNv16PxSize.x();
	float bOff = -0.5f * m_rgbNv16PxSize.x();
	float cOff =  0.5f * m_rgbNv16PxSize.x();
	float dOff =  1.5f * m_rgbNv16PxSize.x();

	// Update local memory
	m_rgbNv16ConstantsLocal[0] = aOff;
	m_rgbNv16ConstantsLocal[1] = bOff;
	m_rgbNv16ConstantsLocal[2] = cOff;
	m_rgbNv16ConstantsLocal[3] = dOff;

	// Update hardware buffer
	if(m_rgbNv16Constants) {
		if(!updateDXBuffer(
			m_device, m_rgbNv16Constants, m_rgbNv16ConstantsLocal,
			sizeof(m_rgbNv16ConstantsLocal)))
		{
			return; // Update failed
		}
	}

	m_rgbNv16ConstantsDirty = false;
}

void D3DContext::updateTexDecalConstants()
{
	if(!m_texDecalConstantsDirty)
		return; // Nothing to do

	// Update local memory
	m_texDecalConstantsLocal[0] = m_texDecalModulate.redF();
	m_texDecalConstantsLocal[1] = m_texDecalModulate.greenF();
	m_texDecalConstantsLocal[2] = m_texDecalModulate.blueF();
	m_texDecalConstantsLocal[3] = m_texDecalModulate.alphaF();
	uint *uintConstants = (uint *)m_texDecalConstantsLocal;
	uintConstants[4] = m_texDecalFlags;
	uintConstants[5] = 0;
	uintConstants[6] = 0;
	uintConstants[7] = 0;
	m_texDecalConstantsLocal[8] = m_texDecalEffects[0];
	m_texDecalConstantsLocal[9] = m_texDecalEffects[1];
	m_texDecalConstantsLocal[10] = m_texDecalEffects[2];
	m_texDecalConstantsLocal[11] = m_texDecalEffects[3];

	// Update hardware buffer
	if(m_texDecalConstants) {
		if(!updateDXBuffer(
			m_device, m_texDecalConstants, m_texDecalConstantsLocal,
			sizeof(m_texDecalConstantsLocal)))
		{
			return; // Update failed
		}
	}

	m_texDecalConstantsDirty = false;
}

void D3DContext::setSwizzleInTexDecal(bool doSwizzle)
{
	quint32 flag = (doSwizzle ? 0xFFFFFFFF : 0);
	if(m_texDecalFlags != flag)
		m_texDecalConstantsDirty = true;
	m_texDecalFlags = flag;
}

//=============================================================================
// D3DContext public interface

bool D3DContext::isValid() const
{
	return m_swapChain != NULL && m_device != NULL;
}

/// <summary>
/// Flushes the graphic context's command buffer. Calling this method should be
/// avoided whenever possible as it has a significant overhead. The context
/// will automatically flush when required.
/// </summary>
void D3DContext::flush()
{
	if(m_device == NULL)
		return;
	m_device->Flush();
}

//-----------------------------------------------------------------------------
// Buffers

VertexBuffer *D3DContext::createVertexBuffer(int numFloats)
{
	if(!isValid())
		return NULL; // DirectX must be initialized
	if(numFloats <= 0)
		return NULL; // Invalid size

	D3DVertexBuffer *buf = new D3DVertexBuffer(this, numFloats);
	return buf;
}

void D3DContext::deleteVertexBuffer(VertexBuffer *buf)
{
	if(buf == NULL)
		return;
	delete static_cast<D3DVertexBuffer *>(buf);
}

/// <summary>
/// Creates a static texture based off the provided QImage. If `writable` is
/// true then the texture data can be rewritten at any time. If `targetable` is
/// true then the texture can be used as a render target.
/// </summary>
/// <returns>
/// A pointer to the newly created texture or NULL on failure.
/// </returns>
Texture *D3DContext::createTexture(QImage img, bool writable, bool targetable)
{
	if(img.isNull())
		return NULL;

	// Determine format
	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
	switch(img.format()) {
	default:
	case QImage::Format_Invalid:
		gfxLog(LOG_CAT) << "Invalid image format for texture";
		return NULL;
	case QImage::Format_ARGB6666_Premultiplied:
	case QImage::Format_ARGB32_Premultiplied:
	case QImage::Format_ARGB8565_Premultiplied:
	case QImage::Format_ARGB8555_Premultiplied:
	case QImage::Format_ARGB4444_Premultiplied:
	case QImage::Format_Mono:
	case QImage::Format_MonoLSB:
	case QImage::Format_Indexed8:
	case QImage::Format_RGB666:
	case QImage::Format_RGB16: // PF_B5G6R5 results in black textures
		gfxLog(LOG_CAT)
			<< "Unoptimal image format for texture, converting to BGRA";
		img = img.convertToFormat(QImage::Format_ARGB32);
		// Fall though
	case QImage::Format_RGB32: // Qt sets the alpha to 0xFF
	case QImage::Format_ARGB32:
		format = DXGI_FORMAT_B8G8R8A8_UNORM;
		break;
	case QImage::Format_RGB888:
		format = DXGI_FORMAT_B8G8R8X8_UNORM; // TODO: Correct format?
		break;
	case QImage::Format_RGB555:
		format = DXGI_FORMAT_B5G5R5A1_UNORM;
		break;
	case QImage::Format_RGB444:
		format = DXGI_FORMAT_B4G4R4A4_UNORM;
		break;
	}

	GfxTextureFlags flags = 0;
	if(writable)
		flags |= GfxWritableFlag;
	if(targetable)
		flags |= GfxTargetableFlag;

	D3DTexture *tex =
		new D3DTexture(this, flags, img.size(), format, img.bits());
	if(tex->isValid())
		return tex;
	delete tex;
	return NULL;
}

/// <summary>
/// Creates a rewritable texture buffer of the specified size. `writable` means
/// writable by the CPU and `targetable` means the texture can be bound to a
/// render target. It is possible to have a texture that is not writable or
/// targetable if you're only populating it using `copyTextureData()`. Textures
/// are in the `RGBA` format unless `useBgra` is true in which case the texture
/// is in the `BGRA` format.
/// </summary>
/// <returns>
/// A pointer to the newly created texture or NULL on failure.
/// </returns>
Texture *D3DContext::createTexture(
	const QSize &size, bool writable, bool targetable, bool useBgra)
{
	if(size.isEmpty())
		return NULL; // Cannot create empty textures
	GfxTextureFlags flags = 0;
	if(writable)
		flags |= GfxWritableFlag;
	if(targetable)
		flags |= GfxTargetableFlag;

	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
	if(useBgra)
		format = DXGI_FORMAT_B8G8R8A8_UNORM;
	D3DTexture *tex = new D3DTexture(this, flags, size, format);
	if(tex->isValid())
		return tex;
	delete tex;
	return NULL;
}

/// <summary>
/// Creates a rewritable texture buffer of the specified size. `writable` means
/// writable by the CPU and `targetable` means the texture can be bound to a
/// render target. It is possible to have a texture that is not writable or
/// targetable if you're only populating it using `copyTextureData()`. The
/// created texture will have the same pixel format as the texture `sameFormat`
/// so that it's safe to copy pixel data between the two textures with
/// `copyTextureData()`.
/// </summary>
/// <returns>
/// A pointer to the newly created texture or NULL on failure.
/// </returns>
Texture *D3DContext::createTexture(
	const QSize &size, Texture *sameFormat, bool writable, bool targetable)
{
	if(size.isEmpty())
		return NULL; // Cannot create empty textures
	if(sameFormat == NULL)
		return NULL;
	GfxTextureFlags flags = 0;
	if(writable)
		flags |= GfxWritableFlag;
	if(targetable)
		flags |= GfxTargetableFlag;

	D3DTexture *fmtTex = static_cast<D3DTexture *>(sameFormat);
	DXGI_FORMAT format = fmtTex->getPixelFormat();
	D3DTexture *tex = new D3DTexture(this, flags, size, format);
	if(tex->isValid())
		return tex;
	delete tex;
	return NULL;
}

/// <summary>
/// Creates a special texture buffer that cannot be bound with `setTexture()`
/// but can be used to read back pixel data from the graphics hardware.
/// </summary>
/// <returns>
/// A pointer to the newly created texture or NULL on failure.
/// </returns>
Texture *D3DContext::createStagingTexture(const QSize &size)
{
	if(size.isEmpty())
		return NULL; // Cannot create empty textures

	D3DTexture *tex =
		new D3DTexture(this, GfxStagingFlag, size, DXGI_FORMAT_R8G8B8A8_UNORM);
	if(tex->isValid())
		return tex;
	delete tex;
	return NULL;
}

// This method is not a part of the GraphicsContext interface but it placed
// here as it's related to texture creation.
Texture *D3DContext::createGDITexture(const QSize &size)
{
	if(size.isEmpty())
		return NULL; // Cannot create empty textures
	if(!hasDxgi11())
		return NULL; // GDI compatible textures are a DXGI 1.1 feature
	if(!hasBgraTexSupport())
		return NULL; // GDI compatible textures are always BGRA

	// GDI-compatible textures _MUST_ be in BGRA format and be targetable
	// otherwise we will receive E_INVALIDARG errors
	D3DTexture *tex = new D3DTexture(
		this, GfxTargetableFlag | GfxGDIFlag, size,
		DXGI_FORMAT_B8G8R8A8_UNORM);
	if(tex->isValid())
		return tex;
	delete tex;
	return NULL;
}

// This method is not a part of the GraphicsContext interface but it placed
// here as it's related to texture creation.
Texture *D3DContext::openSharedTexture(HANDLE sharedHandle)
{
	if(sharedHandle == NULL)
		return NULL; // Invalid handle

	// Open shared resource as a texture
	ID3D10Resource *resource = NULL;
	HRESULT res = m_device->OpenSharedResource(
		sharedHandle, __uuidof(ID3D10Resource), (void **)(&resource));
	if(FAILED(res)) {
		gfxLog(LOG_CAT, GfxLog::Warning)
			<< "Failed to open DirectX shared resource. "
			<< "Reason = " << getDXErrorCode(res);
		return NULL;
	}
	ID3D10Texture2D *d3dTex = NULL;
	res = resource->QueryInterface(
		__uuidof(ID3D10Texture2D), (void **)(&d3dTex));
	if(FAILED(res)) {
		gfxLog(LOG_CAT, GfxLog::Warning)
			<< "Failed to convert DirectX shared resource to a texture. "
			<< "Reason = " << getDXErrorCode(res);
		resource->Release();
		return NULL;
	}
	resource->Release();

	D3DTexture *tex = new D3DTexture(this, d3dTex);
	if(tex->isValid())
		return tex;
	delete tex;
	return NULL;
}

Texture *D3DContext::openDX10Texture(ID3D10Texture2D *d3dTex)
{
	if(d3dTex == NULL)
		return NULL; // Invalid handle
	D3DTexture *tex = new D3DTexture(this, d3dTex);
	if(tex->isValid())
		return tex;
	delete tex;
	return NULL;
}

void D3DContext::deleteTexture(Texture *tex)
{
	if(tex == NULL)
		return;
	delete static_cast<D3DTexture *>(tex);
}

/// <summary>
/// Copies the texel data from one texture to another.
/// </summary>
/// <returns>True if the copy command was queued or false on failure.</returns>
bool D3DContext::copyTextureData(
	Texture *dst, Texture *src, const QPoint &dstPos, const QRect &srcRect)
{
	if(dst == NULL || src == NULL)
		return false;
	if(dst->isMapped() || src->isMapped()) {
		gfxLog(LOG_CAT, GfxLog::Warning)
			<< "Cannot copy texture data while mapped";
		return false;
	}
	if(dstPos.x() < 0 || dstPos.y() < 0 ||
		dstPos.x() + srcRect.width() > dst->getWidth() ||
		dstPos.y() + srcRect.height() > dst->getHeight())
	{
		gfxLog(LOG_CAT, GfxLog::Warning)
			<< "Cannot copy texture data as the source rectangle doesn't fit "
			<< "in the destination texture";
		return false;
	}
	if(srcRect.x() < 0 || srcRect.y() < 0 ||
		srcRect.right() > src->getWidth() ||
		srcRect.bottom() > dst->getHeight())
	{
		gfxLog(LOG_CAT, GfxLog::Warning)
			<< "Cannot copy texture data as the source rectangle doesn't fit "
			<< "in the source texture";
		return false;
	}

	D3DTexture *dstTex = static_cast<D3DTexture *>(dst);
	D3DTexture *srcTex = static_cast<D3DTexture *>(src);
	D3D10_BOX box;
	box.left = srcRect.left();
	box.top = srcRect.top();
	box.front = 0;
	box.right = srcRect.right() + 1;
	box.bottom = srcRect.bottom() + 1;
	box.back = 1;
	m_device->CopySubresourceRegion(
		dstTex->getTexture(), D3D10CalcSubresource(0, 0, 0),
		dstPos.x(), dstPos.y(), 0,
		srcTex->getTexture(), D3D10CalcSubresource(0, 0, 0),
		&box);
	return true;
}

//-----------------------------------------------------------------------------
// Render targets

void D3DContext::resizeScreenTarget(const QSize &newSize)
{
	if(!isValid())
		return; // DirectX must be initialized
	if(m_screenTargetSize == newSize)
		return; // No change

	// Don't log as we'll spam the log file when the user resizes the window
	//gfxLog(LOG_CAT) << "Setting screen size to: " << newSize;

	// Unbind the render target if is currently bound
	if(m_currentTarget == GfxScreenTarget) {
		ID3D10RenderTargetView *nullTarget[2] = { NULL, NULL };
		m_device->OMSetRenderTargets(2, nullTarget, NULL);
	}

	// Release the target
	if(m_screenTarget)
		m_screenTarget->Release();
	m_screenTarget = NULL;

	// Actually issue the resize command
	HRESULT res = m_swapChain->ResizeBuffers(
		2, newSize.width(), newSize.height(), DXGI_FORMAT_R8G8B8A8_UNORM, 0);
	if(SUCCEEDED(res))
		m_screenTargetSize = newSize;
	else {
		gfxLog(LOG_CAT, GfxLog::Warning)
			<< "Failed to resize swap chain buffer. "
			<< "Reason = " << getDXErrorCode(res);
	}

	// Recreate render target
	if(!createScreenTarget()) {
		// This is very bad, we probably need to exit the program as well as
		// the rest of the application doesn't take into account when this
		// happens and will most likely lead to more DirectX errors as we'll
		// be rendering to an invalid render target. TODO
		return;
	}

	// Rebind the render target if it was previously bound
	if(m_currentTarget == GfxScreenTarget)
		setRenderTarget(m_currentTarget);
}

void D3DContext::resizeCanvasTarget(const QSize &newSize)
{
	if(!isValid())
		return; // DirectX must be initialized
	if(m_canvasTargetSize == newSize)
		return; // No change

	gfxLog(LOG_CAT) << "Setting canvas texture size to: " << newSize;

	// Unbind the render target if is currently bound
	if(m_currentTarget == GfxCanvas1Target ||
		m_currentTarget == GfxCanvas2Target)
	{
		ID3D10RenderTargetView *nullTarget[2] = { NULL, NULL };
		m_device->OMSetRenderTargets(2, nullTarget, NULL);
	}

	// Release the old textures and render targets
	delete m_canvas1Texture;
	delete m_canvas2Texture;
	m_canvas1Texture = NULL;
	m_canvas2Texture = NULL;

	// Create brand new textures with render targets
	m_canvas1Texture = new D3DTexture(
		this, GfxTargetableFlag, newSize, DXGI_FORMAT_R8G8B8A8_UNORM);
	m_canvas2Texture = new D3DTexture(
		this, GfxTargetableFlag, newSize, DXGI_FORMAT_R8G8B8A8_UNORM);
	if(m_canvas1Texture->getTexture() && m_canvas2Texture->getTexture())
		m_canvasTargetSize = newSize;
	else {
		gfxLog(LOG_CAT, GfxLog::Warning)
			<< "Failed to create two canvas textures.";
	}
	if(m_canvas1Texture->getTargetView() == NULL ||
		m_canvas2Texture->getTargetView() == NULL)
	{
		gfxLog(LOG_CAT, GfxLog::Warning)
			<< "Failed to create two canvas render targets.";
	}

	// Rebind the render target if it was previously bound
	if(m_currentTarget == GfxCanvas1Target ||
		m_currentTarget == GfxCanvas2Target)
	{
		setRenderTarget(m_currentTarget);
	}

	// Determine DXGI version now as it relies on having a valid canvas texture
	hasDxgi11();
}

/// <summary>
/// Resizes the scratch target to the specified size, enlarging its internal
/// texture if required.
///
/// NOTE: `setRenderTarget()` should be called after this method if the calling
/// code intends to render to it.
/// </summary>
void D3DContext::resizeScratchTarget(const QSize &newSize)
{
	if(!isValid())
		return; // DirectX must be initialized

	// Get old size taking into account  NULL pointers
	QSize oldSize(0, 0);
	if(m_scratch1Texture != NULL)
		oldSize = m_scratch1Texture->getSize();

	// Update the scratch texture target size so that the calling code doesn't
	// need to know the actual scratch texture size when calling
	// `setRenderTarget()`
	m_scratchTargetSize = newSize;

	// Do we need to enlarge the actual texture?
	if(newSize.width() <= oldSize.width() &&
		newSize.height() <= oldSize.height())
	{
		// Scratch texture is already large enough
		return;
	}
	// Scratch texture needs to be enlarged

	// Enlarge to the next largest power of two
	QSize size(nextPowTwo(newSize.width()), nextPowTwo(newSize.height()));
	gfxLog(LOG_CAT) << "Setting scratch texture size to: " << size;

	// Recreate scratch textures
	deleteTexture(m_scratch1Texture);
	deleteTexture(m_scratch2Texture);
	m_scratch1Texture =
		static_cast<D3DTexture *>(createTexture(size, false, true));
	m_scratch2Texture =
		static_cast<D3DTexture *>(createTexture(size, false, true));
}

void D3DContext::swapScreenBuffers()
{
	if(!isValid())
		return; // DirectX must be initialized

	m_swapChain->Present(0, 0);
}

Texture *D3DContext::getTargetTexture(GfxRenderTarget target)
{
	switch(target) {
	default:
	case GfxScreenTarget:
		return NULL;
	case GfxCanvas1Target:
		return m_canvas1Texture;
	case GfxCanvas2Target:
		return m_canvas2Texture;
	case GfxScratch1Target:
		return m_scratch1Texture;
	case GfxScratch2Target:
		return m_scratch2Texture;
	case GfxUserTarget:
		return m_userTargets[0];
	}
}

/// <summary>
/// Returns the next available scratch target so that it's possible to chain
/// multiple scratch renders back-to-back.
/// </summary>
GfxRenderTarget D3DContext::getNextScratchTarget()
{
	GfxRenderTarget ret = GfxScratch1Target;
	if(m_scratchNextTarget == 1)
		ret = GfxScratch2Target;
	m_scratchNextTarget ^= 1;
	return ret;
}

/// <summary>
/// Returns the ratio between what the user's requested scratch target size is
/// and what the actual scratch target texture size is. E.g. if the target size
/// is (256, 128) and the actual texture size is (512, 512) then the returned
/// value will be (0.5, 0.25).
/// </summary>
/// <returns></returns>
QPointF D3DContext::getScratchTargetToTextureRatio()
{
	QSize texSize = m_scratchTargetSize;
	if(m_scratch1Texture != NULL)
		texSize = m_scratch1Texture->getSize();
	return QPointF(
		(float)m_scratchTargetSize.width() / (float)texSize.width(),
		(float)m_scratchTargetSize.height() / (float)texSize.height());
}

//-----------------------------------------------------------------------------
// Advanced rendering

Texture *D3DContext::prepareTexture(
	Texture *tex, const QSize &size, GfxFilter filter, bool setFilter,
	QPointF &pxSizeOut, QPointF &botRightOut)
{
	// Even if the input is invalid still try to provide a sane output
	if(!isValid() || tex == NULL || size.width() <= 0 || size.height() <= 0) {
		pxSizeOut = QPointF(1.0f, 1.0f);
		botRightOut = QPointF(1.0f, 1.0f);
		if(setFilter) {
			setTextureFilter((filter == GfxPointFilter)
				? GfxPointFilter : GfxBilinearFilter);
		}
		return tex;
	}

	// Don't crop anything
	const QSize &texSize = tex->getSize();
	QRect cropRect(0, 0, texSize.width(), texSize.height());

	QPointF topLeft;
	return prepareTexture(tex, cropRect, size, filter, setFilter, pxSizeOut,
		topLeft, botRightOut);
}

/// <summary>
/// Prepares the input texture for rendering at the specified size. The
/// returned texture is to be rendered using `GfxPointFilter` filtering if this
/// method was called with `GfxPointFilter` itself or `GfxBilinearFilter` if it
/// was not, set `setFilter` to true to make this method automatically set up
/// the texture filtering mode for you. As the returned texture can potentially
/// be a scratch texture the texture data should be rendered or copied before
/// any other method that uses a scratch texture is called.
///
/// If this method was called with `GfxPointFilter` then it is essentially a
/// no-op. If this method was called with `GfxBilinearFilter` then it will
/// automatically create the least amount of mipmaps necessary to render at the
/// specified size and then return the details of the smallest mipmap. If this
/// method was called with `GfxBicubicFilter` then it will apply bicubic
/// rescaling of the input to the exact specified size so the calling code does
/// not need to worry about how to sample the returned texture (This basically
/// forces another pass on the texture and could potentially be optimized out
/// at a later date at the expense of more shader permutations).
///
/// WARNING: Do not use Texture::getSize() or `size` to determine texel size in
/// later stages! Instead use `pxSizeOut` and `botRightOut` as they take into
/// account scratch texture sharing and the different filter algorithms.
///
/// WARNING: `pxSizeOut` and `botRightOut` are not always the same for the same
/// input as scratch textures can grow at any time! If you're rendering the
/// returned texture using a vertex buffer you must always check the result and
/// update your buffer's UV if it changes between calls.
/// </summary>
Texture *D3DContext::prepareTexture(
	Texture *tex, const QRect &cropRect, const QSize &size,
	GfxFilter filter, bool setFilter, QPointF &pxSizeOut,
	QPointF &topLeftOut, QPointF &botRightOut)
{
	// Even if the input is invalid still try to provide a sane output
	if(!isValid() || tex == NULL || size.width() <= 0 || size.height() <= 0) {
		pxSizeOut = QPointF(1.0f, 1.0f);
		topLeftOut = QPointF(0.0f, 0.0f);
		botRightOut = QPointF(1.0f, 1.0f);
		if(setFilter) {
			setTextureFilter((filter == GfxPointFilter)
				? GfxPointFilter : GfxBilinearFilter);
		}
		return tex;
	}

	// The relative size of the output texture pixel data vs the actual
	// size of the texture buffer. E.g. 0.5 = Half the texture size.
	QPointF relTexSize(1.0f, 1.0f);
	Texture *outTex = tex;

	// Remember original state
	GfxRenderTarget origTarget = m_currentTarget;

	// TODO: Validate crop rectangle

	// We do cropping inefficiently by resampling the entire texture and then
	// doing the actual crop at the end of the method. TODO: We should crop
	// before resampling (Will take a lot of effort though).
	QSizeF cropRelSize( // Relative size of the crop rect vs texture size
		(qreal)cropRect.width() / (qreal)tex->getSize().width(),
		(qreal)cropRect.height() / (qreal)tex->getSize().height());
	QSize invCropSize( // Effective size that the output must be
		ceil((qreal)size.width() / cropRelSize.width()),
		ceil((qreal)size.height() / cropRelSize.height()));

	// Apply our per-method scaling algorithm
	switch(filter) {
	case GfxPointFilter:
		// We don't need to do any actual texture processing for point sampling
		break;
	default:
#if 0
	case GfxBicubicFilter:
#endif // 0
	case GfxBilinearFilter: {
		// Create mipmaps as required
		QSize nextSize = tex->getSize();
		for(;;) {
			if(nextSize.width() <= invCropSize.width() * 2
				&& nextSize.height() <= invCropSize.height() * 2)
			{
				// We can now sample the texture without any distortion, stop
				// creating mipmaps
				break;
			}

			// Calculate the size of the next mipmap. We must integer ceil() to
			// prevent going under 50% size due to floor()ing.
			nextSize = QSize(
				qMax((nextSize.width() + 1) / 2, invCropSize.width()),
				qMax((nextSize.height() + 1) / 2, invCropSize.height()));

			//gfxLog(LOG_CAT)
			//	<< "Creating mipmap of " << nextSize << " for target size "
			//	<< size;

			// Update the vertex buffer
			createTexDecalRect(
				m_mipmapBuf, QRectF(0.0f, 0.0f,
				(qreal)nextSize.width(), (qreal)nextSize.height()),
				relTexSize);

			// Setup render target
			resizeScratchTarget(nextSize);
			GfxRenderTarget target = getNextScratchTarget();
			setRenderTarget(target);
			QMatrix4x4 mat;
			setViewMatrix(mat);
			mat.ortho(
				0.0f, nextSize.width(), nextSize.height(), 0.0f, -1.0f, 1.0f);
			setProjectionMatrix(mat);

			// Render the mipmap
			setShader(GfxTexDecalShader);
			setTopology(GfxTriangleStripTopology);
			setBlending(GfxNoBlending);
			setTexture(outTex);
			setTextureFilter(GfxBilinearFilter);
			drawBuffer(m_mipmapBuf);

			// Update references
			outTex = getTargetTexture(target);
			relTexSize = getScratchTargetToTextureRatio();
		}
		break; }
	}

	// Restore original state
	setRenderTarget(origTarget);

	// Adjust top-left and bottom-right points for cropping
	topLeftOut = QPointF(0.0f, 0.0f);
	botRightOut = relTexSize;
	if(cropRect.topLeft() != QPoint(0, 0) ||
		cropRect.size() != tex->getSize())
	{
		QPointF pxSize(
			relTexSize.x() / (qreal)tex->getSize().width(),
			relTexSize.y() / (qreal)tex->getSize().height());
		topLeftOut = QPointF(
			(qreal)cropRect.left() * pxSize.x(),
			(qreal)cropRect.top() * pxSize.y());
		botRightOut = QPointF(
			(qreal)(cropRect.right() + 1) * pxSize.x(),
			(qreal)(cropRect.bottom() + 1) * pxSize.y());
	}

	pxSizeOut = QPointF(
		(botRightOut.x() - topLeftOut.x()) / (qreal)size.width(),
		(botRightOut.y() - topLeftOut.y()) / (qreal)size.height());
	if(setFilter) {
		setTextureFilter(
			(filter == GfxPointFilter) ? GfxPointFilter : GfxBilinearFilter);
	}

	return outTex;
}

/// <summary>
/// Converts the specified input texture data to a BGRX texture. WARNING: The
/// resulting texture is on the scratch texture, if you want to keep the data
/// you must copy it elsewhere before the scratch texture is used by another
/// method.
/// </summary>
/// <returns>NULL if the texture could not be converted</returns>
Texture *D3DContext::convertToBgrx(
	GfxPixelFormat format, Texture *planeA, Texture *planeB, Texture *planeC)
{
	if(format >= NUM_PIXEL_FORMAT_TYPES)
		return NULL;
	if(format == GfxNoFormat)
		return NULL;
	if(format == GfxRGB24Format || format == GfxRGB32Format ||
		format == GfxARGB32Format)
	{
		// RGB24 is not a valid format, RGB32 and ARGB32 don't need conversion
		return NULL;
	}

	switch(format) {
	default:
		return NULL;
	case GfxYV12Format: // NxM Y, (N/2)x(M/2) V, (N/2)x(M/2) U
	case GfxIYUVFormat: { // NxM Y, (N/2)x(M/2) U, (N/2)x(M/2) V
		if(planeA == NULL || planeB == NULL || planeC == NULL)
			return NULL;
		if(planeB->getWidth() != planeA->getWidth() / 2 ||
			planeB->getHeight() != planeA->getHeight() / 2 ||
			planeC->getWidth() != planeA->getWidth() / 2 ||
			planeC->getHeight() != planeA->getHeight() / 2)
		{
			return NULL;
		}

		// The only difference between IYUV and YV12 is the plane order.
		// Reorder to YV12 always.
		if(format == GfxIYUVFormat) {
			Texture *tmp = planeB;
			planeB = planeC;
			planeC = tmp;
		}

		// Determine output texture size
		QSize outSize(
			(qreal)(planeA->getWidth() * 4), (qreal)planeA->getHeight());

		//--------------------------------------------------------------------

		// Remember original state
		GfxRenderTarget origTarget = m_currentTarget;

		// Update the vertex buffer. NOTE: We reuse the mipmapping buffer
		createTexDecalRect(
			m_mipmapBuf, QRectF(0.0f, 0.0f,
			(qreal)outSize.width(), (qreal)outSize.height()));

		// Setup render target
		resizeScratchTarget(outSize);
		GfxRenderTarget target = getNextScratchTarget();
		setRenderTarget(target);
		QMatrix4x4 mat;
		setViewMatrix(mat);
		mat.ortho(
			0.0f, outSize.width(), outSize.height(), 0.0f, -1.0f, 1.0f);
		setProjectionMatrix(mat);

		// HACK: Reuse RgbNv16 shader cbuffer
		float outTexWidth = 1.0f / outSize.width();
		m_rgbNv16ConstantsLocal[0] = // Inverse 4x Y texel width
			outTexWidth * 4.0f;
		m_rgbNv16ConstantsLocal[1] = // Half Y texel width
			outTexWidth * 0.125f;
		m_rgbNv16ConstantsLocal[2] = // Inverse 4x U/V texel width
			outTexWidth * 8.0f;
		m_rgbNv16ConstantsLocal[3] = // Half U/V texel width
			outTexWidth * 0.0625f;
		if(!m_rgbNv16Constants || !updateDXBuffer(
			m_device, m_rgbNv16Constants, m_rgbNv16ConstantsLocal,
			sizeof(m_rgbNv16ConstantsLocal)))
		{
			// Update failed. Restore original state and return
			setRenderTarget(origTarget);
			return NULL;
		}
		m_rgbNv16ConstantsDirty = true;

		// Render the mipmap
		setShader(GfxYv12RgbShader);
		setTopology(GfxTriangleStripTopology);
		setBlending(GfxNoBlending);
		setTexture(planeA, planeB, planeC);
		setTextureFilter(GfxPointFilter);
		drawBuffer(m_mipmapBuf);

		// Restore original state
		setRenderTarget(origTarget);

		//--------------------------------------------------------------------

		return getTargetTexture(target); }
	case GfxNV12Format: // NxM Y, Nx(M/2) interleaved UV
		return NULL; // TODO
	case GfxUYVYFormat: // UYVY
	case GfxHDYCFormat: // UYVY with BT.709
	case GfxYUY2Format: { // YUYV
		if(planeA == NULL)
			return NULL;

		// Determine output texture size
		QSize outSize(
			(qreal)(planeA->getWidth() * 2), (qreal)planeA->getHeight());

		//--------------------------------------------------------------------

		// Remember original state
		GfxRenderTarget origTarget = m_currentTarget;

		// Update the vertex buffer. NOTE: We reuse the mipmapping buffer
		createTexDecalRect(
			m_mipmapBuf, QRectF(0.0f, 0.0f,
			(qreal)outSize.width(), (qreal)outSize.height()));

		// Setup render target
		resizeScratchTarget(outSize);
		GfxRenderTarget target = getNextScratchTarget();
		setRenderTarget(target);
		QMatrix4x4 mat;
		setViewMatrix(mat);
		mat.ortho(
			0.0f, outSize.width(), outSize.height(), 0.0f, -1.0f, 1.0f);
		setProjectionMatrix(mat);

		// HACK: Reuse RgbNv16 shader cbuffer
		float outTexWidth = 1.0f / outSize.width();
		m_rgbNv16ConstantsLocal[0] = // 4x Y texel width
			outTexWidth * 2.0f;
		m_rgbNv16ConstantsLocal[1] = // 2x Y texel width
			outTexWidth;
		m_rgbNv16ConstantsLocal[2] = 0.0f;
		m_rgbNv16ConstantsLocal[3] = 0.0f;
		if(!m_rgbNv16Constants || !updateDXBuffer(
			m_device, m_rgbNv16Constants, m_rgbNv16ConstantsLocal,
			sizeof(m_rgbNv16ConstantsLocal)))
		{
			// Update failed. Restore original state and return
			setRenderTarget(origTarget);
			return NULL;
		}
		m_rgbNv16ConstantsDirty = true;

		// Render the mipmap
		switch(format) {
		case GfxUYVYFormat: // UYVY
			setShader(GfxUyvyRgbShader);
			break;
		case GfxHDYCFormat: // UYVY with BT.709
			setShader(GfxHdycRgbShader);
			break;
		case GfxYUY2Format: // YUYV
			setShader(GfxYuy2RgbShader);
			break;
		}
		setTopology(GfxTriangleStripTopology);
		setBlending(GfxNoBlending);
		setTexture(planeA);
		setTextureFilter(GfxPointFilter);
		drawBuffer(m_mipmapBuf);

		// Restore original state
		setRenderTarget(origTarget);

		//--------------------------------------------------------------------
		return getTargetTexture(target); }
	}

	// Should never be reached
	return NULL;
}

//-----------------------------------------------------------------------------
// Drawing

void D3DContext::setRenderTarget(GfxRenderTarget target)
{
	if(!isValid())
		return; // DirectX must be initialized

	// WARNING: Do not test if we are already using the requested target as
	// `resizeScreenTarget()` relies on the current behaviour

	m_currentTarget = target;
	ID3D10RenderTargetView *targetView[2] = { NULL, NULL };
	QRect viewRect;
	switch(target) {
	default:
	case GfxScreenTarget:
		m_currentTarget = GfxScreenTarget; // Because of "default"
		targetView[0] = m_screenTarget;
		viewRect = QRect(QPoint(0, 0), m_screenTargetSize);
		break;
	case GfxCanvas1Target:
		if(m_canvas1Texture != NULL)
			targetView[0] = m_canvas1Texture->getTargetView();
		viewRect = QRect(QPoint(0, 0), m_canvasTargetSize);
		break;
	case GfxCanvas2Target:
		if(m_canvas2Texture != NULL)
			targetView[0] = m_canvas2Texture->getTargetView();
		viewRect = QRect(QPoint(0, 0), m_canvasTargetSize);
		break;
	case GfxScratch1Target:
		if(m_scratch1Texture != NULL)
			targetView[0] = m_scratch1Texture->getTargetView();
		viewRect = QRect(QPoint(0, 0), m_scratchTargetSize);
		break;
	case GfxScratch2Target:
		if(m_scratch2Texture != NULL)
			targetView[0] = m_scratch2Texture->getTargetView();
		viewRect = QRect(QPoint(0, 0), m_scratchTargetSize);
		break;
	case GfxUserTarget:
		if(m_userTargets[0] != NULL && m_userTargets[0]->isTargetable()) {
			targetView[0] =
				static_cast<D3DTexture *>(m_userTargets[0])->getTargetView();
		}
		if(m_userTargets[1] != NULL && m_userTargets[1]->isTargetable()) {
			targetView[1] =
				static_cast<D3DTexture *>(m_userTargets[1])->getTargetView();
			// TODO: We don't log a warning if this is not targetable
		}
		viewRect = m_userTargetViewport;
		break;
	}
	if(targetView[0] == NULL) {
		gfxLog(LOG_CAT, GfxLog::Warning)
			<< "Attempted to select a render target that doesn't exist yet";
		return;
	}
	m_device->OMSetRenderTargets(2, targetView, NULL);

	// Setup the viewport as well so that the application doesn't need to worry
	// about it. Note that for scratch targets we set the viewport size to
	// match the requested size instead of the actual scratch texture size.
	D3D10_VIEWPORT vp;
	vp.TopLeftX = viewRect.x();
	vp.TopLeftY = viewRect.y();
	vp.Width = viewRect.width();
	vp.Height = viewRect.height();
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	m_device->RSSetViewports(1, &vp);

	// Camera constants are per target, do buffer update when needed
	m_cameraConstantsDirty = true;
}

void D3DContext::setShader(GfxShader shader)
{
	if(!isValid())
		return; // DirectX must be initialized
	if(m_boundShader == shader)
		return; // Already bound

	switch(shader) {
	default:
	case GfxNoShader:
		m_device->IASetInputLayout(NULL);
		m_device->VSSetShader(NULL);
		m_device->PSSetShader(NULL);
		break;
	case GfxSolidShader:
		m_device->IASetInputLayout(m_solidIL);
		m_device->VSSetShader(m_solidVS);
		m_device->PSSetShader(m_solidPS);
		break;
	case GfxTexDecalShader:
		m_device->IASetInputLayout(m_texDecalIL);
		m_device->VSSetShader(m_texDecalVS);
		m_device->PSSetShader(m_texDecalPS);
		break;
	case GfxTexDecalGbcsShader:
		m_device->IASetInputLayout(m_texDecalIL);
		m_device->VSSetShader(m_texDecalVS);
		m_device->PSSetShader(m_texDecalGbcsPS);
		break;
	case GfxTexDecalRgbShader:
		m_device->IASetInputLayout(m_texDecalIL);
		m_device->VSSetShader(m_texDecalVS);
		m_device->PSSetShader(m_texDecalRgbPS);
		break;
	case GfxResizeLayerShader:
		m_device->IASetInputLayout(m_resizeIL);
		m_device->VSSetShader(m_resizeVS);
		m_device->PSSetShader(m_resizePS);
		break;
	case GfxRgbNv16Shader:
		m_device->IASetInputLayout(m_texDecalIL);
		m_device->VSSetShader(m_texDecalVS);
		m_device->PSSetShader(m_rgbNv16PS);
		break;
	case GfxYv12RgbShader:
		m_device->IASetInputLayout(m_texDecalIL);
		m_device->VSSetShader(m_texDecalVS);
		m_device->PSSetShader(m_yv12RgbPS);
		break;
	case GfxUyvyRgbShader:
		m_device->IASetInputLayout(m_texDecalIL);
		m_device->VSSetShader(m_texDecalVS);
		m_device->PSSetShader(m_UyvyRgbPS);
		break;
	case GfxHdycRgbShader:
		m_device->IASetInputLayout(m_texDecalIL);
		m_device->VSSetShader(m_texDecalVS);
		m_device->PSSetShader(m_HdycRgbPS);
		break;
	case GfxYuy2RgbShader:
		m_device->IASetInputLayout(m_texDecalIL);
		m_device->VSSetShader(m_texDecalVS);
		m_device->PSSetShader(m_Yuy2RgbPS);
		break;
	}
	m_boundShader = shader;
}

void D3DContext::setTopology(GfxTopology topology)
{
	if(!isValid())
		return; // DirectX must be initialized

	switch(topology) {
	default:
	case GfxTriangleListTopology:
		m_device->IASetPrimitiveTopology(
			D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		break;
	case GfxTriangleStripTopology:
		m_device->IASetPrimitiveTopology(
			D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		break;
	}
}

void D3DContext::setBlending(GfxBlending blending)
{
	if(!isValid())
		return; // DirectX must be initialized

	switch(blending) {
	default:
	case GfxNoBlending:
		m_device->OMSetBlendState(m_noBlend, NULL, 0xFFFFFFFF);
		break;
	case GfxAlphaBlending:
		m_device->OMSetBlendState(m_alphaBlend, NULL, 0xFFFFFFFF);
		break;
	case GfxPremultipliedBlending:
		m_device->OMSetBlendState(m_premultiBlend, NULL, 0xFFFFFFFF);
		break;
	}
}

void D3DContext::setTexture(Texture *texA, Texture *texB, Texture *texC)
{
	if(!isValid())
		return; // DirectX must be initialized
	if(texA == NULL)
		return;
	if(texA->isStaging() || (texB && texB->isStaging()) ||
		(texC && texC->isStaging()))
	{
		gfxLog(LOG_CAT, GfxLog::Warning)
			<< "Attempted to bind a staging texture to a shader";
		return;
	}

	int num = 1;
	if(texB != NULL)
		num = 2;
	if(texC != NULL)
		num = 3;
	D3DTexture *textureA = static_cast<D3DTexture *>(texA);
	D3DTexture *textureB = texB ? static_cast<D3DTexture *>(texB) : NULL;
	D3DTexture *textureC = texC ? static_cast<D3DTexture *>(texC) : NULL;
	ID3D10ShaderResourceView *view[3] = {
		textureA->getResourceView(),
		texB ? textureB->getResourceView() : NULL,
		texC ? textureC->getResourceView() : NULL };
	m_device->PSSetShaderResources(0, num, view);

	// Do we need to swizzle the RGB components as we're storing BGRA data in
	// a RGBA texture?
	setSwizzleInTexDecal(textureA->doBgraSwizzle());
}

void D3DContext::setTextureFilter(GfxFilter filter)
{
	if(!isValid())
		return; // DirectX must be initialized

	switch(filter) {
	case GfxPointFilter:
		m_device->PSSetSamplers(0, 1, &m_pointClampSampler);
		break;
	default:
#if 0
	case GfxBicubicFilter:
		gfxLog(LOG_CAT, GfxLog::Warning)
			<< "Unsupported texture filter, using bilinear.";
		// Fall through
#endif // 0
	case GfxBilinearFilter:
		m_device->PSSetSamplers(0, 1, &m_bilinearClampSampler);
		break;
	case GfxResizeLayerFilter:
		m_device->PSSetSamplers(0, 1, &m_resizeSampler);
		break;
	}
}

void D3DContext::clear(const QColor &color)
{
	if(!isValid())
		return; // DirectX must be initialized

	// Get current render target
	ID3D10RenderTargetView *targetView[2] = { NULL, NULL };
	switch(m_currentTarget) {
	default:
	case GfxScreenTarget:
		targetView[0] = m_screenTarget;
		break;
	case GfxCanvas1Target:
		targetView[0] = m_canvas1Texture->getTargetView();
		break;
	case GfxCanvas2Target:
		targetView[0] = m_canvas2Texture->getTargetView();
		break;
	case GfxScratch1Target:
		targetView[0] = m_scratch1Texture->getTargetView();
		break;
	case GfxScratch2Target:
		targetView[0] = m_scratch2Texture->getTargetView();
		break;
	case GfxUserTarget:
		if(m_userTargets[0] != NULL && m_userTargets[0]->isTargetable()) {
			targetView[0] =
				static_cast<D3DTexture *>(m_userTargets[0])->getTargetView();
		}
		if(m_userTargets[1] != NULL && m_userTargets[1]->isTargetable()) {
			targetView[1] =
				static_cast<D3DTexture *>(m_userTargets[1])->getTargetView();
		}
		break;
	}
	if(targetView[0] == NULL && targetView[1] == NULL)
		return;

	float colorF[4];
	colorF[0] = color.redF();
	colorF[1] = color.greenF();
	colorF[2] = color.blueF();
	colorF[3] = color.alphaF();
	if(targetView[0] != NULL)
		m_device->ClearRenderTargetView(targetView[0], colorF);
	if(targetView[1] != NULL)
		m_device->ClearRenderTargetView(targetView[1], colorF);
}

void D3DContext::drawBuffer(
	VertexBuffer *buf, int numVertices, int startVertex)
{
	if(!isValid())
		return; // DirectX must be initialized
	if(buf == NULL)
		return; // Invalid input

	if(numVertices < 0)
		numVertices = buf->getNumVerts();
	if(numVertices == 0)
		return; // Nothing to render

	// Bind the vertex buffer
	D3DVertexBuffer *buffer = static_cast<D3DVertexBuffer *>(buf);
	buffer->bind();

	// Update and bind our camera constants
	updateCameraConstants();
	m_device->VSSetConstantBuffers(0, 1, &m_cameraConstants);

	// Update and bind our pixel shader constants if needed
	if(m_boundShader == GfxResizeLayerShader) {
		updateResizeConstants();
		m_device->PSSetConstantBuffers(0, 1, &m_resizeConstants);
	} else if(m_boundShader == GfxRgbNv16Shader) {
		updateRgbNv16Constants();
		m_device->PSSetConstantBuffers(0, 1, &m_rgbNv16Constants);
	} else if(m_boundShader == GfxYv12RgbShader ||
		m_boundShader == GfxUyvyRgbShader ||
		m_boundShader == GfxHdycRgbShader ||
		m_boundShader == GfxYuy2RgbShader)
	{
		// HACK: Reuse RgbNv16 shader cbuffer
		m_device->PSSetConstantBuffers(0, 1, &m_rgbNv16Constants);
	} else if(m_boundShader == GfxTexDecalShader ||
		m_boundShader == GfxTexDecalGbcsShader ||
		m_boundShader == GfxTexDecalRgbShader)
	{
		updateTexDecalConstants();
		m_device->PSSetConstantBuffers(0, 1, &m_texDecalConstants);
	}

	// Actually send the draw command
	m_device->Draw(numVertices, startVertex);
}
