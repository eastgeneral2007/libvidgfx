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
	// .r = Inverse 4x Y texel width (= 1 / Output texture width * 4)
	// .g = Half Y texel width (= 1 / Output texture width / 8)
	// .b = Inverse 4x U/V texel width (= 1 / Output texture width * 8)
	// .a = Half U/V texel width (= 1 / Output texture width / 16)
	float4 texOffsets;
};

Texture2D yPlaneTexture;
Texture2D vPlaneTexture;
Texture2D uPlaneTexture;
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

// As we used packed RGBA textures instead of just a texture with a single
// component we must adjust our sample position using all this ugly maths.
// TODO: If we can get rid of this then YUV conversion will be much faster and
// we will be able to use hardware samplers for bilinear filtering.
float packedSample(
	Texture2D tex, float2 uv, float invFourTexelWidth, float halfTexelWidth)
{
	// While our textures are packed the way we render the triangles results in
	// the UV coordinate remaining the same. All we need to do is figure out
	// which texel component we should return. We determine this as a number in
	// the range [0..3] where R=0, G=1, B=2, A=3
	float subtex =
		fmod(uv.x - halfTexelWidth, invFourTexelWidth) / invFourTexelWidth;
	subtex = floor(subtex * 4.0f);

	// Actually sample the texture
	float4 pix = tex.Sample(
		texSampler, uv);

	// Return the appropriate texel component by masking out the others
	return dot(pix, float4(
		step(0.5f, 1.0f - abs(subtex       )),
		step(0.5f, 1.0f - abs(subtex - 1.0f)),
		step(0.5f, 1.0f - abs(subtex - 2.0f)),
		step(0.5f, 1.0f - abs(subtex - 3.0f))));
}

float4 main(PSInput input) : SV_TARGET
{
	// Get YUV components from textures
	float3 yuv = float3(
		packedSample(yPlaneTexture, input.uv, texOffsets.r, texOffsets.g),
		packedSample(uPlaneTexture, input.uv, texOffsets.b, texOffsets.a),
		packedSample(vPlaneTexture, input.uv, texOffsets.b, texOffsets.a));

	// Do YUV->RGB conversion
	yuv -= float3(0.0625f, 0.5f, 0.5f);
	yuv = mul(yuv, yuvCoef); // `yuv` now contains RGB
	yuv = saturate(yuv);

	// Return RGBA
	return float4(yuv, 1.0f);
}
