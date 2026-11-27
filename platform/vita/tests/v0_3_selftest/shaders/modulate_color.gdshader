shader_type canvas_item;
// Expected: the tile's modulate (blue) applied, inverted inside the circle.
// COLOR starts as texture * modulate, as in Godot.
void fragment() {
	vec2 p = UV - vec2(0.5);
	if (length(p) < 0.35) {
		COLOR.rgb = vec3(1.0) - COLOR.rgb;
	}
}
