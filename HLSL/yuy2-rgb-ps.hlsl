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

// Designed for use with the "texDecal-vs.hlsl" vertex shader

cbuffer RgbNv16
{
	// .r = 4x Y texel width (= 1 / Output texture width * 2)
	// .g = 2x Y texel width (= 1 / Output texture width)
	float4 texOffsets;
};

Texture2D texTexture;
SamplerState texSampler; // Designed for nearest-neighbour

struct PSInput
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD0;
};

//-----------------------------------------------------------------------------
// YUV->RGB coefficients

// BT.601 (Y [16 .. 235], U/V [16 .. 240]) with linear, full-range RGB output.
// Input YUV must be first subtracted by (0.0625, 0.5, 0.5).
static const float3x3 yuvCoef = {
	1.164f,  1.164f, 1.164f,
	0.000f, -0.392f, 2.017f,
	1.596f, -0.813f, 0.000f};

// BT.709 (Y [16 .. 235], U/V [16 .. 240]) with linear, full-range RGB output.
// Input YUV must be first subtracted by (0.0625, 0.5, 0.5).
//static const float3x3 yuvCoef = {
//	1.164f,  1.164f, 1.164f,
//	0.000f, -0.213f, 2.112f,
//	1.793f, -0.533f, 0.000f};

//-----------------------------------------------------------------------------

float4 main(PSInput input) : SV_TARGET
{
	// Sample our texture. Even though our textures are packed the way we
	// render the triangle makes the UV coordinate remain the same.
	float4 pix = texTexture.Sample(
		texSampler, input.uv);

	// Unpack the YUV components
	float3 yuv = float3(
		(fmod(input.uv.x, texOffsets.r) < texOffsets.g) ? pix.r : pix.b,
		pix.g,
		pix.a);

	// Do YUV->RGB conversion
	yuv -= float3(0.0625f, 0.5f, 0.5f);
	yuv = mul(yuv, yuvCoef); // `yuv` now contains RGB
	yuv = saturate(yuv);

	// Return RGBA
	return float4(yuv, 1.0f);
}
