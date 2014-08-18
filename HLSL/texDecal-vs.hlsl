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

cbuffer Camera
{
	matrix viewMat;
	matrix projMat;
};

struct VSInput
{
	float3 pos : POSITION;
	float2 uv : TEXCOORD0;
};

struct PSInput
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD0;
};

PSInput main(VSInput input)
{
	PSInput output;

	// Transform to viewport space
	output.pos = float4(input.pos, 1.0f);
	output.pos = mul(output.pos, viewMat);
	output.pos = mul(output.pos, projMat);

	// Forward UV
	output.uv = input.uv;

	return output;
}
