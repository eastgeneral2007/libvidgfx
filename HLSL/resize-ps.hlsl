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

cbuffer Resize
{
	float4 texRect;
};

Texture2D texTexture;
SamplerState texSampler;

struct PSInput
{
	float4 pos : SV_POSITION;
	float4 wPos : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET
{
	// Calculate texture coordinates. As X and Y should always be (0, 0) we can
	// simplify the formula slightly.
	float2 uv;
	//uv.x = (input.wPos.x - texRect.x) / texRect.z;
	//uv.y = (input.wPos.y - texRect.y) / texRect.w;
	uv.x = input.wPos.x / texRect.z;
	uv.y = input.wPos.y / texRect.w;

	// Sample canvas texture and convert to inverted luminance
	float4 col = texTexture.Sample(texSampler, uv)
		; // HACK: Fix auto-indenting of the next line
	float lum = 1.0f - dot(col.rgb, float3(0.2126, 0.7152, 0.0722));

	// Rescale luminance so that it only uses values between the ranges of
	// [0.0, 0.3] and [0.7, 1.0]
	lum = (lum - 0.5f) * 0.6f;
	lum += sign(lum) * 0.2f + 0.5f;

	return float4(lum, lum, lum, 1.0f);
}
