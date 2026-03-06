#version 450 core

// Input game resolution
layout(location = 0) uniform vec2 u_input_size;
// Output screen resolution
layout(location = 1) uniform vec2 u_output_size;
// Rendered game texture
layout(location = 2) uniform sampler2D u_texture;

in vec2 v_uv;
out vec4 frag_color;

// ---- CRT Parameters ----
// Scanline thinness: 0.5=fused, 0.7=default, 1.0=thin
#define THIN 0.70
// Horizontal blur: -3.0=pixely, -2.5=default, -2.0=smooth
#define BLUR -2.5
// Phosphor mask: 0.25=heavy, 0.5=default, 1.0=no mask
#define MASK 0.5
// Screen warp amounts (x=horizontal influence, y=vertical)
#define WARP_X (1.0 / 48.0)
#define WARP_Y (1.0 / 24.0)

// ---- Fetch from game render texture ----
vec3 crt_fetch(vec2 uv) {
	return texture(u_texture, uv).rgb;
}

// ---- Aperture grille lite phosphor mask ----
// Brighter variant: one channel is dark per column of 3 pixels
vec3 crt_mask(vec2 pos, float dark) {
	vec3 m = vec3(1.0, 1.0, 1.0);
	float x = fract(pos.x * (1.0 / 3.0));
	if      (x < (1.0 / 3.0)) m.r = dark;
	else if (x < (2.0 / 3.0)) m.g = dark;
	else                        m.b = dark;
	return m;
}

// ---- Tonal curve parameter generation ----
// Computes exposure-matching parameters to counteract darkening from
// scanlines and mask. Returns (contrast, a, b, contrast+saturation).
vec4 crt_tone(float contrast, float saturation, float thin, float mask) {
	// Grille lite adjustment
	mask = 0.5 + mask * 0.5;
	float midOut = 0.18 / ((1.5 - thin) * (0.5 * mask + 0.5));
	float pMidIn = pow(0.18, contrast);
	return vec4(
		contrast,
		((-pMidIn) + midOut) / ((1.0 - pMidIn) * midOut),
		((-pMidIn) * midOut + pMidIn) / (midOut * (-pMidIn) + midOut),
		contrast + saturation);
}

void main() {
	// Pixel coordinates in output space (origin bottom-left)
	vec2 fragCoord = v_uv * u_output_size;

	vec2 rcpIn  = 1.0 / u_input_size;
	vec2 rcpOut = 1.0 / u_output_size;

	// Convert to [-1, 1] range and apply barrel warp
	vec2 pos = fragCoord * (2.0 * rcpOut) - vec2(1.0);
	pos *= vec2(
		1.0 + (pos.y * pos.y) * WARP_X,
		1.0 + (pos.x * pos.x) * WARP_Y);

	// Vignette: fade to black at warped screen edges
	float vin = 1.0 -
		(1.0 - clamp(pos.x * pos.x, 0.0, 1.0)) *
		(1.0 - clamp(pos.y * pos.y, 0.0, 1.0));
	vin = clamp((-vin) * u_input_size.y + u_input_size.y, 0.0, 1.0);

	// Move into input pixel space [0, inputSize]
	pos = pos * (u_input_size * 0.5) + (u_input_size * 0.5);

	// Snap to scanline and texel centers
	float y0 = floor(pos.y - 0.5) + 0.5;
	float x0 = floor(pos.x - 1.5) + 0.5;

	// 8-tap fetch: 4 texels from 2 nearest scanlines
	vec2 p = vec2(x0 * rcpIn.x, y0 * rcpIn.y);
	vec3 a0 = crt_fetch(p);              p.x += rcpIn.x;
	vec3 a1 = crt_fetch(p);              p.x += rcpIn.x;
	vec3 a2 = crt_fetch(p);              p.x += rcpIn.x;
	vec3 a3 = crt_fetch(p);              p.y += rcpIn.y;
	vec3 b3 = crt_fetch(p);              p.x -= rcpIn.x;
	vec3 b2 = crt_fetch(p);              p.x -= rcpIn.x;
	vec3 b1 = crt_fetch(p);              p.x -= rcpIn.x;
	vec3 b0 = crt_fetch(p);

	// Vertical scanline weights (cosine window)
	float off = pos.y - y0;
	const float pi2 = 6.28318530717958;
	float sA = cos(min(0.5,  off        * THIN) * pi2) * 0.5 + 0.5;
	float sB = cos(min(0.5, (1.0 - off) * THIN) * pi2) * 0.5 + 0.5;

	// Horizontal gaussian blur weights
	float d0 = pos.x - x0, d1 = d0 - 1.0, d2 = d0 - 2.0, d3 = d0 - 3.0;
	float w0 = exp2(BLUR * d0 * d0);
	float w1 = exp2(BLUR * d1 * d1);
	float w2 = exp2(BLUR * d2 * d2);
	float w3 = exp2(BLUR * d3 * d3);
	// Normalise and apply vignette
	float wT = vin / (w0 + w1 + w2 + w3);
	sA *= wT;
	sB *= wT;

	// Combine scanlines with horizontal blur
	vec3 color =
		(a0 * w0 + a1 * w1 + a2 * w2 + a3 * w3) * sA +
		(b0 * w0 + b1 * w1 + b2 * w2 + b3 * w3) * sB;

	// Phosphor mask
	color *= crt_mask(fragCoord, MASK);

	// Auto-exposure + contrast + saturation (tonal curve)
	vec4 tone = crt_tone(1.0, 0.0, THIN, MASK);
	float peak = max(1.0 / (256.0 * 65536.0), max(color.r, max(color.g, color.b)));
	vec3 ratio = color / peak;
	peak = pow(peak, tone.x);
	peak = peak / (peak * tone.y + tone.z);
	ratio = pow(ratio, vec3(tone.w));

	frag_color = vec4(ratio * peak, 1.0);
}
