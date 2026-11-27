shader_type canvas_item;
// Expected: icon dissolving in and out following the logo's red channel
// (second sampler bound by the material).
uniform sampler2D mask_texture : repeat_enable;
void fragment() {
	vec4 c = texture(TEXTURE, UV);
	float n = texture(mask_texture, UV).r;
	float progress = sin(TIME) * 0.5 + 0.5;
	c.a *= step(progress, n + 0.02);
	COLOR = c;
}
