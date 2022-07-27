#pragma once

#ifndef __PSSL__
#define float4x4	 sce::Vectormath::Simd::Aos::Matrix4
#endif

struct Camera
{
	float x;
	float y;

	float res_x;
	float res_y;

	float4x4 u_worldviewproj;
};