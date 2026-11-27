shader_type canvas_item;
// Expected: rings moving outward over 4 angular sectors.
void fragment() {
	vec2 p = UV * 2.0 - 1.0;
	float a = atan(p.y, p.x) / PI;
	float r = length(p);
	float rings = mod(r * 8.0 - TIME, 1.0);
	COLOR = vec4(vec3(fract(a * 2.0) * rings), 1.0);
}
