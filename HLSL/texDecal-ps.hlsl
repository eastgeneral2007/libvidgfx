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

cbuffer TexDecal
{
	float4 modCol;
	uint4 flags; // x: 0 = Don't swizzle, 1+ = Swizzle RGB
	float4 gbcs; // r: Gamma g: Brightness b: Contrast a: Saturation
};

Texture2D texTexture;
SamplerState texSampler;

struct PSInput
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET
{
	//return float4(input.uv, 0.0f, 1.0f);

	// Fetch texture texel data and swizzle it if we're storing BGRA data in a
	// RGBA texture
	float4 texCol = texTexture.Sample(
		texSampler, input.uv);
	texCol.rgb = flags.x ? texCol.bgr : texCol.rgb;

	return texCol * modCol;
}
