shader_type canvas_item;
// Expected: icon with a 2-pixel yellow outline (uses TEXTURE_PIXEL_SIZE).
uniform vec4 line_color : source_color = vec4(1.0, 0.85, 0.1, 1.0);
uniform float width = 2.0;
void fragment() {
	vec2 s = TEXTURE_PIXEL_SIZE * width;
	float a = texture(TEXTURE, UV + vec2(s.x, 0.0)).a;
	a = max(a, texture(TEXTURE, UV - vec2(s.x, 0.0)).a);
	a = max(a, texture(TEXTURE, UV + vec2(0.0, s.y)).a);
	a = max(a, texture(TEXTURE, UV - vec2(0.0, s.y)).a);
	vec4 c = texture(TEXTURE, UV);
	COLOR = mix(vec4(line_color.rgb, a * line_color.a), c, c.a);
}
