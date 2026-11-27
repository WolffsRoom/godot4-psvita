shader_type canvas_item;
// Expected: what is behind the tile, blurred (screen texture mip level 2).
uniform sampler2D screen_texture : hint_screen_texture, filter_linear_mipmap;
uniform float lod : hint_range(0.0, 5.0) = 2.0;
void fragment() {
	COLOR = vec4(textureLod(screen_texture, SCREEN_UV, lod).rgb, 1.0);
}
