// Translation stage for the native GXM canvas fragment-shader subset.
// Original Godot source is kept by GXMMaterialStorage. Never identify shaders
// by filenames/game IDs: the same capabilities apply to any Godot project.
//
// Current stage: fragment-only. GXM render targets provide the screen
// samplers, with explicitly downsampled GPU mip levels. Arbitrary user
// texture uniforms and vertex-varying programs remain separate work.
class GXMCanvasShaderTranslator {
    static bool is_ident_char(char32_t c) {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                (c >= '0' && c <= '9') || c == '_';
    }
    static String replace_token(const String &source, const String &from, const String &to) {
        String result;
        int previous = 0;
        int match = source.find(from);
        while (match >= 0) {
            const bool left_ok = match == 0 || !is_ident_char(source[match - 1]);
            const int end = match + from.length();
            const bool right_ok = end >= source.length() || !is_ident_char(source[end]);
            if (left_ok && right_ok) {
                result += source.substr(previous, match - previous);
                result += to;
                previous = end;
            }
            match = source.find(from, match + from.length());
        }
        return result + source.substr(previous);
    }
    // Rewrite GLSL texture(sampler, uv[, lod]) to a native GXM screen-mip
    // lookup only for samplers declared hint_screen_texture. Calls to TEXTURE
    // remain tex2D, and user samplers are never silently repurposed.
    static String rewrite_screen_sampling(const String &input, const String &sampler) {
        if (sampler.is_empty()) return input;
        String out;
        int copied = 0;
        int at = input.find("tex2D(");
        while (at >= 0) {
            int depth = 1, close = -1;
            for (int i = at + 6; i < input.length(); ++i) {
                if (input[i] == '(') depth++;
                else if (input[i] == ')' && --depth == 0) { close = i; break; }
            }
            if (close < 0) break;
            const String inside = input.substr(at + 6, close - at - 6);
            Vector<String> args;
            int first = 0, inner_depth = 0;
            for (int i = 0; i < inside.length(); ++i) {
                if (inside[i] == '(') inner_depth++;
                else if (inside[i] == ')') inner_depth--;
                else if (inside[i] == ',' && inner_depth == 0) {
                    args.push_back(inside.substr(first, i - first).strip_edges());
                    first = i + 1;
                }
            }
            args.push_back(inside.substr(first).strip_edges());
            if (args.size() >= 2 && args.size() <= 3 && args[0] == sampler) {
                out += input.substr(copied, at - copied);
                out += "gxm_screen_sample(" + args[1] + ", " +
                        (args.size() == 3 ? args[2] : String("0.0")) + ")";
                copied = close + 1;
            }
            at = input.find("tex2D(", close + 1);
        }
        return out + input.substr(copied);
    }
public:
    static PackedStringArray additional_sampler_names(const String &source) {
        PackedStringArray samplers;
        const PackedStringArray lines = source.split("\n");
        for (const String &original : lines) {
            const String line = original.strip_edges();
            if (!line.begins_with("uniform sampler2D ") ||
                    line.contains("hint_screen_texture")) continue;
            const int begin = line.find("sampler2D ") + 10;
            const int terminator = line.find(":", begin);
            const int semicolon = line.find(";", begin);
            const int stop = terminator >= 0 ? terminator : semicolon;
            if (stop <= begin) continue;
            const String name = line.substr(begin, stop - begin).strip_edges();
            if (!name.is_empty()) samplers.push_back(name);
        }
        return samplers;
    }
    static String to_cg(const String &godot_source, String *reason = nullptr) {
        if (!godot_source.contains("shader_type canvas_item")) {
            if (reason) *reason = "not a canvas_item shader";
            return String();
        }
        if (godot_source.contains("textureLod(")) {
            if (reason) *reason = "textureLod expression is not yet lowered";
            return String();
        }
        if (godot_source.contains("void vertex()") || godot_source.contains("varying ")) {
            if (reason) *reason = "requires vertex-varying stage";
            return String();
        }
        // The Godot GLSL/Cg conversion is still a subset. In particular,
        // Cg on Vita may attempt pathological compile-time unrolling of
        // shader loops and crash inside SceShaccCg rather than return an
        // error. Never hand these programs to the hardware compiler until
        // loop lowering into bounded GXM-compatible code is implemented.
        int for_count = 0, pos = godot_source.find("for (");
        while (pos >= 0) {
            for_count++;
            pos = godot_source.find("for (", pos + 5);
        }
        if (for_count > 1 && godot_source.length() > 4500) {
            if (reason) *reason = "compiler safety: large nested/iterated loops require Cg lowering";
            return String();
        }
        if (godot_source.contains("mat2(") || godot_source.contains("mat3(") ||
                godot_source.contains("mat4(")) {
            if (reason) *reason = "matrix constructors require GLSL-to-Cg lowering";
            return String();
        }
        const PackedStringArray extra_samplers = additional_sampler_names(godot_source);
        if (extra_samplers.size() > 4) {
            if (reason) *reason = "more than four custom texture samplers";
            return String();
        }
        if (godot_source.contains("uniform samplerCube")) {
            if (reason) *reason = "cubemap sampler needs 3D shader pipeline";
            return String();
        }
        String screen_sampler;
        const PackedStringArray screen_lines = godot_source.split("\n");
        for (const String &original_line : screen_lines) {
            const String line = original_line.strip_edges();
            if (!line.begins_with("uniform sampler")) continue;
            if (!line.contains("hint_screen_texture")) continue;
            const int begin = line.find("sampler2D") + 9;
            const int finish = line.find(":", begin);
            if (finish <= begin) return String();
            const String name = line.substr(begin, finish - begin).strip_edges();
            if (!screen_sampler.is_empty() && name != screen_sampler) {
                if (reason) *reason = "multiple independent screen samplers";
                return String();
            }
            screen_sampler = name;
        }
        const int frag = godot_source.find("void fragment()");
        if (frag < 0) {
            if (reason) *reason = "fragment entry point missing";
            return String();
        }
        // Preserve helper functions and local declarations, excluding Godot
        // directives. Vita Cg accepts uniforms reliably as entry-point
        // parameters (R5 probe 0), while global uniforms made even the
        // minimal translated shaders fail with "fatal internal error".
        String source;
        PackedStringArray uniform_parameters;
        const PackedStringArray lines = godot_source.split("\n");
        for (int i = 0; i < lines.size(); i++) {
            String line = lines[i];
            const String trimmed = line.strip_edges();
            if (trimmed.begins_with("shader_type ") || trimmed.begins_with("render_mode ")) {
                continue;
            }
            if (trimmed.begins_with("uniform sampler")) {
                continue; // Engine binds screen and project samplers to GXM slots.
            }
            if (trimmed.begins_with("uniform ")) {
                // Godot defaults are held by MaterialStorage/ShaderMaterial;
                // retain only the Cg type/name and place it on main().
                const int hint = line.find(":");
                if (hint >= 0) line = line.substr(0, hint);
                const int def = line.find("=");
                if (def >= 0) line = line.substr(0, def);
                line = line.strip_edges();
                if (line.ends_with(";")) line = line.left(line.length() - 1);
                uniform_parameters.push_back(line);
                continue;
            }
            source += line + "\n";
        }
        source = replace_token(source, "vec2", "float2");
        source = replace_token(source, "vec3", "float3");
        source = replace_token(source, "vec4", "float4");
        source = replace_token(source, "ivec2", "int2");
        source = replace_token(source, "ivec3", "int3");
        source = replace_token(source, "ivec4", "int4");
        source = replace_token(source, "mix", "lerp");
        source = replace_token(source, "fract", "frac");
        source = replace_token(source, "texture", "tex2D");
        // Used only in shaders whose source does not request explicit LOD.
        source = replace_token(source, "mod", "fmod");
        source = rewrite_screen_sampling(source, screen_sampler);
        const String fragment_name = "void fragment()";
        const int f = source.find(fragment_name);
        if (f < 0) return String();
        const int start = source.find("{", f + fragment_name.length());
        if (start < 0) return String();
        int nest = 0, end = -1;
        for (int i = start; i < source.length(); i++) {
            if (source[i] == '{') nest++;
            if (source[i] == '}' && --nest == 0) { end = i; break; }
        }
        if (end < 0) {
            if (reason) *reason = "unclosed fragment block";
            return String();
        }
        String entry_parameters = "float2 gxm_uv : TEXCOORD0, uniform float4 gxm_tint";
        if (godot_source.contains("TIME")) entry_parameters += ", uniform float TIME";
        if (godot_source.contains("SCREEN_PIXEL_SIZE")) entry_parameters += ", uniform float2 SCREEN_PIXEL_SIZE";
        for (const String &parameter : uniform_parameters) {
            String cg_parameter = replace_token(parameter, "vec2", "float2");
            cg_parameter = replace_token(cg_parameter, "vec3", "float3");
            cg_parameter = replace_token(cg_parameter, "vec4", "float4");
            cg_parameter = replace_token(cg_parameter, "ivec2", "int2");
            cg_parameter = replace_token(cg_parameter, "ivec3", "int3");
            cg_parameter = replace_token(cg_parameter, "ivec4", "int4");
            entry_parameters += ", " + cg_parameter;
        }
        const bool uses_main_texture = godot_source.contains("TEXTURE");
        if (uses_main_texture) {
            entry_parameters += ", uniform sampler2D TEXTURE : TEXUNIT0";
        }
        for (int i = 0; i < extra_samplers.size(); ++i) {
            entry_parameters += ", uniform sampler2D " + extra_samplers[i] +
                    " : TEXUNIT" + itos(7 + i);
        }
        const String entry = "float4 main(" + entry_parameters + ") : COLOR";
        String screen_functions;
        if (!screen_sampler.is_empty()) {
            for (int i = 0; i < 6; ++i) {
                screen_functions += "uniform sampler2D gxm_screen_" + itos(i) +
                        " : TEXUNIT" + itos(i + 1) + ";\n";
            }
            screen_functions += "float4 gxm_screen_sample(float2 uv, float level) {\n"
                    "level = clamp(level, 0.0, 5.0);\n";
            for (int i = 0; i < 5; ++i) {
                screen_functions += "if (level < " + itos(i + 1) + ".0) return lerp("
                        "tex2D(gxm_screen_" + itos(i) + ", uv), "
                        "tex2D(gxm_screen_" + itos(i + 1) + ", uv), level - " + itos(i) + ".0);\n";
            }
            screen_functions += "return tex2D(gxm_screen_5, uv);\n}\n";
        }
        source = source.substr(0, f) + entry +
                source.substr(f + fragment_name.length(), start - (f + fragment_name.length()) + 1) +
                "\nfloat2 UV = gxm_uv;\nfloat2 SCREEN_UV = gxm_uv;\n"
                "float4 COLOR = " + (uses_main_texture ? String("tex2D(TEXTURE, UV)") : String("float4(1.0, 1.0, 1.0, 1.0)")) + ";\n" +
                source.substr(start + 1, end - start - 1) +
                "\nreturn COLOR * gxm_tint;\n}\n" + source.substr(end + 1);
        if (reason) *reason = "native GXM canvas Cg prepared";
        return screen_functions + source;
    }
};
