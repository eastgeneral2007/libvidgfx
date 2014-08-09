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

// Designed for use with the "texDecal-vs.hlsl" vertex shader

cbuffer RgbNv16
{
	float4 texOffsets;
};

Texture2D texTexture;
SamplerState texSampler;

struct PSInput
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD0;
};

struct PSOutput
{
	float4 yyyy : SV_TARGET0;
	float4 uvuv : SV_TARGET1;
};

//-----------------------------------------------------------------------------
// RGB->YUV coefficients

// BT.601 (Y [16 .. 235], U/V [16 .. 240]) with linear, full-range RGB input
static const float4x4 yuvCoef = {
	0.256788f, -0.148223f,  0.439216f, 0.0f,
	0.504129f, -0.290993f, -0.367788f, 0.0f,
	0.097906f,  0.439216f, -0.071427f, 0.0f,
	0.0625f,    0.5f,       0.5f,      1.0f};

// BT.709 (Y [16 .. 235], U/V [16 .. 240]) with linear, full-range RGB input
//static const float4x4 yuvCoef = {
//	0.1826f, -0.1006f,  0.4392f, 0.0f,
//	0.6142f, -0.3386f, -0.3989f, 0.0f,
//	0.0620f,  0.4392f, -0.0403f, 0.0f,
//	0.0625f,  0.5f,     0.50f,   1.0f};

//-----------------------------------------------------------------------------

PSOutput main(PSInput input)
{
	PSOutput output;

	// Sample the appropriate input texture texels that will be packed into our
	// output pixel
	float4 a = texTexture.Sample(
		texSampler, float2(input.uv.x + texOffsets.r, input.uv.y));
	float4 b = texTexture.Sample(
		texSampler, float2(input.uv.x + texOffsets.g, input.uv.y));
	float4 c = texTexture.Sample(
		texSampler, float2(input.uv.x + texOffsets.b, input.uv.y));
	float4 d = texTexture.Sample(
		texSampler, float2(input.uv.x + texOffsets.a, input.uv.y));

	// Do RGB->YUV conversion on all samples
	// TODO: We don't need B or D chroma if using MPEG-2 style subsampling
	a = mul(float4(a.rgb, 1.0f), yuvCoef);
	b = mul(float4(b.rgb, 1.0f), yuvCoef);
	c = mul(float4(c.rgb, 1.0f), yuvCoef);
	d = mul(float4(d.rgb, 1.0f), yuvCoef);

	// Pack luminance into the first render target
	output.yyyy = float4(a.r, b.r, c.r, d.r);

	//-------------------------------------------------------------------------
	// Subsample the chroma horizontally and pack into the second render target

	// MPEG-1 style (Between samples)
	//float4 e = lerp(a, b, 0.5f);
	//float4 f = lerp(c, d, 0.5f);
	//output.uvuv = float4(e.g, e.b, f.g, f.b);

	// MPEG-2 style (Left aligned)
	output.uvuv = float4(a.g, a.b, c.g, c.b);

	//-------------------------------------------------------------------------

	return output;
}
