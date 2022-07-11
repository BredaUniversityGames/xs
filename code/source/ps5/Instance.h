#ifndef __INSTANCE__
#define __INSTANCE__

#ifndef __PSSL__
#define float2	 sce::Vectormath::Simd::Aos::Vector2
#define float4	 sce::Vectormath::Simd::Aos::Vector4
#define ConstantBuffer struct
#endif

#ifdef __PSSL__
ConstantBuffer Instance : b0
#else
ConstantBuffer Instance
#endif
{
	float2 m_position;
};

#ifdef __PSSL__
ConstantBuffer Color : b1
#else
ConstantBuffer Color
#endif
{
	float4 m_color;
};

#endif // __INSTANCE__
