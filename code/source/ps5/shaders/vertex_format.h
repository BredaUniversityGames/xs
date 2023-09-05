#pragma once

struct vs_output
{
	float4 position		: S_POSITION;
	float2 texture		: TEXCOORD0;
	float3 mul_color	: COLOR0;
	float3 add_color	: COLOR1;
};