shader_type canvas_item;
// Expected: icon tinted pink. Neither uniform is set by the material, so this
// checks that shader defaults reach the GPU.
uniform vec4 tint : source_color = vec4(1.0, 0.4, 0.6, 1.0);
uniform float amount : hint_range(0.0, 1.0) = 0.75;
void fragment() {
	vec4 c = texture(TEXTURE, UV);
	COLOR = mix(c, c * tint, amount);
}
