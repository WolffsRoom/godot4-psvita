shader_type canvas_item;
// Expected: red grows to the right, green grows downward, blue 0.2.
void fragment() {
	COLOR = vec4(UV.x, UV.y, 0.2, 1.0);
}
