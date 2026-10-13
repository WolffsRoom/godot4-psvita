#pragma once

// CanvasData is flattened into plain uniforms by _vita_downemit(). Binding
// its original UBO does not populate these uniforms. Upload after glUseProgram
// for every canvas batch, including program switches and new canvas passes.
// The caller supplies GL declarations and the CPU StateBuffer-compatible type.
// Keep this helper shared with the standalone on-device transport probe.
template <typename State>
inline void vita_upload_canvas_uniforms(GLuint program, const State &state) {
	glUniformMatrix4fv(glGetUniformLocation(program, "canvas_transform"), 1, GL_FALSE, state.canvas_transform);
	glUniformMatrix4fv(glGetUniformLocation(program, "screen_transform"), 1, GL_FALSE, state.screen_transform);
	glUniformMatrix4fv(glGetUniformLocation(program, "canvas_normal_transform"), 1, GL_FALSE, state.canvas_normal_transform);
	glUniform4fv(glGetUniformLocation(program, "canvas_modulation"), 1, state.canvas_modulate);
	glUniform2fv(glGetUniformLocation(program, "screen_pixel_size"), 1, state.screen_pixel_size);
	glUniform1f(glGetUniformLocation(program, "time"), state.time);
	glUniform1i(glGetUniformLocation(program, "use_pixel_snap"), state.use_pixel_snap ? 1 : 0);
	glUniform4fv(glGetUniformLocation(program, "sdf_to_tex"), 1, state.sdf_to_tex);
	glUniform2fv(glGetUniformLocation(program, "screen_to_sdf"), 1, state.screen_to_sdf);
	glUniform2fv(glGetUniformLocation(program, "sdf_to_screen"), 1, state.sdf_to_screen);
	// The down-emitter maps uint to signed int for VitaGL/Cg.
	glUniform1i(glGetUniformLocation(program, "directional_light_count"), state.directional_light_count);
	glUniform1f(glGetUniformLocation(program, "tex_to_sdf"), state.tex_to_sdf);
}
