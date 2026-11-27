shader_type canvas_item;
// Expected: 4 vertical bands scrolling sideways. Helper function reads
// uniforms and TIME; int/bool uniforms and a global const.
uniform float speed = 2.0;
uniform int bands = 4;
uniform bool invert = false;
const float BASE = 0.25;
float wave(float x) {
	return sin(x * float(bands) + TIME * speed) * 0.5 + 0.5;
}
void fragment() {
	float w = wave(UV.x * TAU);
	if (invert) {
		w = 1.0 - w;
	}
	COLOR = vec4(vec3(w) * (1.0 - BASE) + BASE, 1.0);
}
