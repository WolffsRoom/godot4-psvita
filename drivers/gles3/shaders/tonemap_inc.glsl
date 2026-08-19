layout(std140) uniform TonemapData { //ubo:0
	float exposure;
	int tonemapper;
	int pad;
	int pad2;
	vec4 tonemapper_params;
	float brightness;
	float contrast;
	float saturation;
	int pad3;
};

// This approximation expects non-negative input; negative input is undefined behavior.
vec3 linear_to_srgb(vec3 color) {
	//const vec3 a = vec3(0.055f);
	//return mix((vec3(1.0f) + a) * pow(color.rgb, vec3(1.0f / 2.4f)) - a, 12.92f * color.rgb, lessThan(color.rgb, vec3(0.0031308f)));
	// Approximation from http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
	return max(vec3(1.055) * pow(color, vec3(0.416666667)) - vec3(0.055), vec3(0.0));
}

// This approximation expects non-negative input; negative input behaves poorly.
vec3 srgb_to_linear(vec3 color) {
	// Approximation from http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
	return color * (color * (color * 0.305306011 + 0.682171111) + 0.012522878);
}

#ifdef APPLY_TONEMAPPING

// VITA bring-up note: the full tonemapper set (ACES/AgX with mat3 math + the
// allenwp curve) makes post/scene/sky large and complex enough to trip a
// memory-corruption bug in VitaGL's GLSL->Cg translator (shader gets duplicated
// -> "identifier 'x' already declared" -> heap/stack smash -> crash). Until the
// translator is fixed (or we move to the PVR backend), collapse every tonemapper
// to a light Reinhard, which keeps the shader small and uses no matrix math.
// Visually acceptable for first bring-up; restore the full path once stable.
#define TONEMAPPER_LINEAR 0
#define TONEMAPPER_REINHARD 1
#define TONEMAPPER_FILMIC 2
#define TONEMAPPER_ACES 3
#define TONEMAPPER_AGX 4

vec3 apply_tonemapping(vec3 color) { // inputs are LINEAR
	if (tonemapper == TONEMAPPER_LINEAR) {
		return color;
	}
	// Ensure color values are positive (negative lights lead to undesired behavior).
	color = max(vec3(0.0), color);
	// Reinhard extended formula (small, matrix-free) for all non-linear tonemappers.
	float white_squared = tonemapper_params.x;
	vec3 white_squared_color = white_squared * color;
	return (white_squared_color + color * color) / (white_squared_color + white_squared);
}

#endif // APPLY_TONEMAPPING
