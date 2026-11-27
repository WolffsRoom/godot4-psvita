// Translation stage for the native GXM canvas fragment-shader subset.
// Original Godot source is kept by GXMMaterialStorage. Never identify shaders
// by filenames/game IDs: the same capabilities apply to any Godot project.
//
// Current stage: fragment-only. GXM render targets provide the screen
// samplers, with explicitly downsampled GPU mip levels. Vertex-varying
// programs remain separate work.
//
// Cg contract (v0.4), driven by the R5 hardware probes:
// - every uniform is a parameter of main(): "uniform float4 c" on main()
//   compiled on the Vita (probe 0) while global uniforms are unproven;
// - user and built-in uniforms are passed as float types named
//   "gxm_u_<name>" and copied into locals with the original Godot name, so
//   int/bool uniforms keep their GLSL type and the bridge only ever writes
//   floats (sceGxmSetUniformDataF);
// - values a helper function needs (uniforms, TIME, ...) become
//   "static" globals assigned at the top of main();
// - no identifier shares its name with a Cg semantic (COLOR -> gxm_COLOR);
// - global Godot "const" declarations become "static const": a plain
//   global in Cg is an (unset) uniform.
class GXMCanvasShaderTranslator {
public:
    struct Uniform {
        String type; // Godot type: float, vec2, vec3, vec4, int, ivec2.., bool, uint
        String name;
        int components = 0;
    };

private:
    static bool is_ident_char(char32_t c) {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                (c >= '0' && c <= '9') || c == '_';
    }
    static bool token_at(const String &source, int match, int length) {
        const bool left_ok = match == 0 || !is_ident_char(source[match - 1]);
        const int end = match + length;
        const bool right_ok = end >= source.length() || !is_ident_char(source[end]);
        return left_ok && right_ok;
    }
    static bool has_token(const String &source, const String &name) {
        for (int at = source.find(name); at >= 0; at = source.find(name, at + 1)) {
            if (token_at(source, at, name.length())) return true;
        }
        return false;
    }
    static String replace_token(const String &source, const String &from, const String &to) {
        String result;
        int previous = 0;
        int match = source.find(from);
        while (match >= 0) {
            if (token_at(source, match, from.length())) {
                result += source.substr(previous, match - previous);
                result += to;
                previous = match + from.length();
            }
            match = source.find(from, match + from.length());
        }
        return result + source.substr(previous);
    }

    // Removes // and /* */ comments so that commented-out code never decides
    // what is supported (e.g. "// void vertex()" used to reject a shader).
    static String strip_comments(const String &source) {
        String out;
        int i = 0, copied = 0;
        const int n = source.length();
        while (i < n) {
            if (source[i] == '/' && i + 1 < n && source[i + 1] == '/') {
                out += source.substr(copied, i - copied);
                while (i < n && source[i] != '\n') i++;
                copied = i;
            } else if (source[i] == '/' && i + 1 < n && source[i + 1] == '*') {
                out += source.substr(copied, i - copied);
                i += 2;
                while (i + 1 < n && !(source[i] == '*' && source[i + 1] == '/')) {
                    if (source[i] == '\n') out += "\n";
                    i++;
                }
                i = i + 1 < n ? i + 2 : n;
                out += " ";
                copied = i;
            } else {
                i++;
            }
        }
        return out + source.substr(copied);
    }

    // Splits the argument list of a call whose "(" is at open. Returns the
    // index of the matching ")" or -1.
    static int split_call(const String &source, int open, Vector<String> &args) {
        int depth = 1, close = -1;
        for (int i = open + 1; i < source.length(); ++i) {
            if (source[i] == '(') depth++;
            else if (source[i] == ')' && --depth == 0) { close = i; break; }
        }
        if (close < 0) return -1;
        const String inside = source.substr(open + 1, close - open - 1);
        int first = 0, inner = 0;
        for (int i = 0; i < inside.length(); ++i) {
            if (inside[i] == '(' || inside[i] == '[') inner++;
            else if (inside[i] == ')' || inside[i] == ']') inner--;
            else if (inside[i] == ',' && inner == 0) {
                args.push_back(inside.substr(first, i - first).strip_edges());
                first = i + 1;
            }
        }
        const String last = inside.substr(first).strip_edges();
        if (!last.is_empty() || !args.is_empty()) args.push_back(last);
        return close;
    }

    // Kind of call rewrite applied by rewrite_calls().
    enum CallRewrite {
        REWRITE_CONSTRUCTOR, // floatN(x) -> ((floatN)(x)): Cg has no scalar broadcast constructor
        REWRITE_ATAN2, // atan(y, x) -> atan2(y, x)
        REWRITE_SCREEN, // tex2D/textureLod(screen, uv[, lod]) -> gxm_screen_sample(...)
        REWRITE_LOD, // textureLod(s, uv, lod) -> tex2Dlod(s, float4(uv, 0.0, lod))
    };
    static String rewrite_calls(const String &input, const String &name, CallRewrite kind,
            const String &screen_sampler = String()) {
        String out;
        int copied = 0;
        const String needle = name + "(";
        int at = input.find(needle);
        while (at >= 0) {
            if (at > 0 && is_ident_char(input[at - 1])) {
                at = input.find(needle, at + 1);
                continue;
            }
            Vector<String> args;
            const int open = at + name.length();
            const int close = split_call(input, open, args);
            if (close < 0) break;
            String replacement;
            switch (kind) {
                case REWRITE_CONSTRUCTOR:
                    if (args.size() == 1) {
                        replacement = "((" + name + ")(" + args[0] + "))";
                    }
                    break;
                case REWRITE_ATAN2:
                    if (args.size() == 2) {
                        replacement = "atan2(" + args[0] + ", " + args[1] + ")";
                    }
                    break;
                case REWRITE_SCREEN:
                    if (args.size() >= 2 && args.size() <= 3 && args[0] == screen_sampler) {
                        replacement = "gxm_screen_sample(gxm_screen_0, gxm_screen_1, gxm_screen_2, "
                                      "gxm_screen_3, gxm_screen_4, gxm_screen_5, " +
                                args[1] + ", " + (args.size() == 3 ? args[2] : String("0.0")) + ")";
                    }
                    break;
                case REWRITE_LOD:
                    if (args.size() == 3) {
                        replacement = "tex2Dlod(" + args[0] + ", float4(" + args[1] + ", 0.0, " + args[2] + "))";
                    }
                    break;
            }
            if (!replacement.is_empty()) {
                // Arguments may hold nested calls of the same kind.
                if (kind == REWRITE_CONSTRUCTOR || kind == REWRITE_ATAN2) {
                    const String nested = rewrite_calls(replacement.substr(1), name, kind, screen_sampler);
                    replacement = replacement.substr(0, 1) + nested;
                }
                out += input.substr(copied, at - copied) + replacement;
                copied = close + 1;
                at = input.find(needle, close + 1);
            } else {
                at = input.find(needle, at + needle.length());
            }
        }
        return out + input.substr(copied);
    }

    // Finds "void <name>()" and returns its body range [start '{', end '}'].
    static bool find_function(const String &source, const String &name, int &r_decl, int &r_open, int &r_close) {
        int at = source.find(name);
        while (at >= 0) {
            if (token_at(source, at, name.length())) {
                int p = at + name.length();
                while (p < source.length() && (source[p] == ' ' || source[p] == '\t')) p++;
                if (p < source.length() && source[p] == '(') {
                    int decl = at - 1;
                    while (decl >= 0 && (source[decl] == ' ' || source[decl] == '\t')) decl--;
                    if (decl >= 3 && source.substr(decl - 3, 4) == "void") {
                        const int open = source.find("{", p);
                        if (open < 0) return false;
                        int nest = 0;
                        for (int i = open; i < source.length(); i++) {
                            if (source[i] == '{') nest++;
                            if (source[i] == '}' && --nest == 0) {
                                r_decl = decl - 3;
                                r_open = open;
                                r_close = i;
                                return true;
                            }
                        }
                        return false;
                    }
                }
            }
            at = source.find(name, at + name.length());
        }
        return false;
    }

    // True when COLOR is used other than as "COLOR = ..." at statement start.
    static bool reads_color(const String &body) {
        for (int at = body.find("COLOR"); at >= 0; at = body.find("COLOR", at + 1)) {
            if (!token_at(body, at, 5)) continue;
            int before = at - 1;
            while (before >= 0 && (body[before] == ' ' || body[before] == '\t' || body[before] == '\n' || body[before] == '\r')) before--;
            int after = at + 5;
            while (after < body.length() && (body[after] == ' ' || body[after] == '\t')) after++;
            const bool statement_start = before < 0 || body[before] == ';' || body[before] == '{' || body[before] == '}';
            const bool plain_assign = after < body.length() && body[after] == '=' &&
                    !(after + 1 < body.length() && body[after + 1] == '=');
            if (!(statement_start && plain_assign)) return true;
        }
        return false;
    }
    static String compact_blank_lines(const String &source) {
        String out;
        const PackedStringArray lines = source.split("\n");
        bool previous_blank = true;
        for (int i = 0; i < lines.size(); i++) {
            const bool blank = lines[i].strip_edges().is_empty();
            if (blank && previous_blank) continue;
            out += lines[i] + "\n";
            previous_blank = blank;
        }
        return out;
    }
    static String cg_type(const String &godot_type) {
        if (godot_type == "vec2") return "float2";
        if (godot_type == "vec3") return "float3";
        if (godot_type == "vec4") return "float4";
        if (godot_type == "ivec2" || godot_type == "uvec2") return "int2";
        if (godot_type == "ivec3" || godot_type == "uvec3") return "int3";
        if (godot_type == "ivec4" || godot_type == "uvec4") return "int4";
        if (godot_type == "uint") return "int";
        if (godot_type == "bvec2") return "bool2";
        if (godot_type == "bvec3") return "bool3";
        if (godot_type == "bvec4") return "bool4";
        return godot_type; // float, int, bool
    }
    static int type_components(const String &godot_type) {
        if (godot_type == "float" || godot_type == "int" || godot_type == "uint" || godot_type == "bool") return 1;
        if (godot_type.ends_with("vec2")) return 2;
        if (godot_type.ends_with("vec3")) return 3;
        if (godot_type.ends_with("vec4")) return 4;
        return 0;
    }
    static String float_type(int components) {
        return components == 1 ? String("float") : "float" + itos(components);
    }
    static bool is_precision(const String &word) {
        return word == "lowp" || word == "mediump" || word == "highp";
    }

public:
    static constexpr int NAME_MAX_BYTES = 47; // VitaGXMCanvasUniform::name minus NUL

    static String uniform_cg_name(const String &name) { return "gxm_u_" + name; }

    // Plain (non-sampler) uniforms in declaration order. Uniforms the
    // translator cannot express (arrays, matrices) are reported as invalid.
    static Vector<Uniform> uniform_declarations(const String &raw_source, String *r_unsupported = nullptr) {
        Vector<Uniform> uniforms;
        const String source = strip_comments(raw_source);
        const PackedStringArray lines = source.split("\n");
        for (const String &original : lines) {
            String line = original.strip_edges();
            if (!line.begins_with("uniform ")) continue;
            const int hint = line.find(":");
            if (hint >= 0) line = line.substr(0, hint);
            const int def = line.find("=");
            if (def >= 0) line = line.substr(0, def);
            const int semicolon = line.find(";");
            if (semicolon >= 0) line = line.substr(0, semicolon);
            const PackedStringArray all_words = line.substr(8).strip_edges().split(" ", false);
            Vector<String> words;
            for (const String &w : all_words) {
                if (!is_precision(w)) words.push_back(w);
            }
            if (words.size() < 2) continue;
            if (words[0].begins_with("sampler")) continue;
            Uniform u;
            u.type = words[0];
            u.name = words[1].strip_edges();
            u.components = type_components(u.type);
            if (!u.components || u.name.contains("[") || words.size() > 2) {
                if (r_unsupported) *r_unsupported = "uniform type not supported by the GXM translator: " + line;
                continue;
            }
            uniforms.push_back(u);
        }
        return uniforms;
    }

    static PackedStringArray additional_sampler_names(const String &raw_source) {
        PackedStringArray samplers;
        const PackedStringArray lines = strip_comments(raw_source).split("\n");
        for (const String &original : lines) {
            const String line = original.strip_edges();
            if (!line.begins_with("uniform ") || !line.contains("sampler2D ") ||
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

    static bool uses_screen_texture(const String &raw_source) {
        return strip_comments(raw_source).contains("hint_screen_texture");
    }

    static String to_cg(const String &godot_source_raw, String *reason = nullptr) {
        String godot_source = strip_comments(godot_source_raw);
        // The 2D GXM renderer has no Light2D pass: light() can never run, so
        // its built-ins must not make the whole shader unsupported.
        int ldecl, lopen, lclose;
        if (find_function(godot_source, "light", ldecl, lopen, lclose)) {
            godot_source = godot_source.substr(0, ldecl) + godot_source.substr(lclose + 1);
        }
        if (!godot_source.contains("shader_type canvas_item")) {
            if (reason) *reason = "not a canvas_item shader";
            return String();
        }
        int vdecl, vopen, vclose;
        if (find_function(godot_source, "vertex", vdecl, vopen, vclose) || godot_source.contains("varying ")) {
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
        static const char *unsupported_tokens[] = {
            "mat2", "mat3", "mat4", "samplerCube", "sampler2DArray", "sampler3D", "usampler2D", "isampler2D",
            "textureSize", "texelFetch", "textureProj", "textureGrad",
            "NORMAL", "NORMAL_MAP", "NORMAL_MAP_DEPTH", "NORMAL_TEXTURE", "SPECULAR_SHININESS",
            "SPECULAR_SHININESS_TEXTURE", "POINT_COORD", "VERTEX", "SHADOW_VERTEX", "LIGHT_VERTEX",
            "AT_LIGHT_PASS", "SCREEN_MATRIX", "CANVAS_MATRIX", "MODEL_MATRIX", "INSTANCE_CUSTOM",
            "VERTEX_ID", "INSTANCE_ID", "REGION_RECT",
        };
        for (const char *token : unsupported_tokens) {
            if (has_token(godot_source, token)) {
                if (reason) *reason = String("unsupported GLSL feature: ") + token;
                return String();
            }
        }
        if (godot_source.contains("instance uniform") || godot_source.contains("global uniform")) {
            if (reason) *reason = "instance/global uniforms are not supported";
            return String();
        }
        const PackedStringArray extra_samplers = additional_sampler_names(godot_source);
        if (extra_samplers.size() > 4) {
            if (reason) *reason = "more than four custom texture samplers";
            return String();
        }
        String screen_sampler;
        const PackedStringArray screen_lines = godot_source.split("\n");
        for (const String &original_line : screen_lines) {
            const String line = original_line.strip_edges();
            if (!line.begins_with("uniform ") || !line.contains("hint_screen_texture")) continue;
            const int begin = line.find("sampler2D") + 9;
            const int finish = line.find(":", begin);
            if (begin < 9 || finish <= begin) {
                if (reason) *reason = "unreadable screen sampler declaration";
                return String();
            }
            const String name = line.substr(begin, finish - begin).strip_edges();
            if (!screen_sampler.is_empty() && name != screen_sampler) {
                if (reason) *reason = "multiple independent screen samplers";
                return String();
            }
            screen_sampler = name;
        }
        String unsupported_uniform;
        const Vector<Uniform> uniforms = uniform_declarations(godot_source, &unsupported_uniform);
        if (!unsupported_uniform.is_empty()) {
            if (reason) *reason = unsupported_uniform;
            return String();
        }
        for (const Uniform &u : uniforms) {
            if (uniform_cg_name(u.name).utf8().length() > NAME_MAX_BYTES) {
                if (reason) *reason = "uniform name too long for the GXM bridge: " + u.name;
                return String();
            }
        }

        // Drop Godot directives and uniform declarations; keep helpers,
        // structs and constants. Precision qualifiers have no Cg meaning.
        String source;
        const PackedStringArray lines = godot_source.split("\n");
        for (int i = 0; i < lines.size(); i++) {
            const String trimmed = lines[i].strip_edges();
            if (trimmed.begins_with("shader_type ") || trimmed.begins_with("render_mode ") ||
                    trimmed.begins_with("uniform ") || trimmed.begins_with("group_uniforms")) {
                source += "\n";
                continue;
            }
            source += lines[i] + "\n";
        }
        source = replace_token(source, "lowp", "");
        source = replace_token(source, "mediump", "");
        source = replace_token(source, "highp", "");

        int fdecl, fopen, fclose;
        if (!find_function(source, "fragment", fdecl, fopen, fclose)) {
            if (reason) *reason = "fragment entry point missing";
            return String();
        }
        String globals = source.substr(0, fdecl) + "\n" + source.substr(fclose + 1);
        String body = source.substr(fopen + 1, fclose - fopen - 1);

        // Godot built-ins that are uniforms on the Cg side.
        struct Builtin {
            const char *name;
            const char *cg_type;
        };
        static const Builtin builtin_uniforms[] = {
            { "TIME", "float" },
            { "SCREEN_PIXEL_SIZE", "float2" },
            { "TEXTURE_PIXEL_SIZE", "float2" },
        };
        String statics, prologue, entry_parameters = "float2 gxm_uv : TEXCOORD0, uniform float4 gxm_tint";
        for (const Builtin &b : builtin_uniforms) {
            const bool in_body = has_token(body, b.name);
            const bool in_globals = has_token(globals, b.name);
            if (!in_body && !in_globals) continue;
            entry_parameters += String(", uniform ") + b.cg_type + " " + uniform_cg_name(b.name);
            if (in_globals) {
                statics += String("static ") + b.cg_type + " " + b.name + ";\n";
                prologue += String(b.name) + " = " + uniform_cg_name(b.name) + ";\n";
            } else {
                prologue += String(b.cg_type) + " " + b.name + " = " + uniform_cg_name(b.name) + ";\n";
            }
        }
        for (const Uniform &u : uniforms) {
            const String cg = cg_type(u.type);
            const String carrier = float_type(u.components);
            entry_parameters += ", uniform " + carrier + " " + uniform_cg_name(u.name);
            String value = uniform_cg_name(u.name);
            if (u.type == "bool") value = "(" + value + " > 0.5)";
            else if (cg != carrier) value = "((" + cg + ")(" + value + "))";
            if (has_token(globals, u.name)) {
                statics += "static " + cg + " " + u.name + ";\n";
                prologue += u.name + " = " + value + ";\n";
            } else {
                prologue += cg + " " + u.name + " = " + value + ";\n";
            }
        }
        // Samplers are bound by slot; helpers have no access to them.
        PackedStringArray all_samplers = extra_samplers;
        if (!screen_sampler.is_empty()) all_samplers.push_back(screen_sampler);
        for (const String &sampler : all_samplers) {
            if (has_token(globals, sampler)) {
                if (reason) *reason = "texture sampling inside helper functions is not supported";
                return String();
            }
        }
        // Godot's COLOR input already holds the texture sample. Sample it
        // unless the body only ever assigns COLOR as a whole (procedural
        // shaders then need no sampler, as in the proven R5 probes).
        const bool uses_main_texture = has_token(body, "TEXTURE") || reads_color(body);
        if (uses_main_texture) entry_parameters += ", uniform sampler2D TEXTURE : TEXUNIT0";
        for (int i = 0; i < extra_samplers.size(); ++i) {
            entry_parameters += ", uniform sampler2D " + extra_samplers[i] + " : TEXUNIT" + itos(7 + i);
        }
        if (!screen_sampler.is_empty()) {
            for (int i = 0; i < 6; ++i) {
                entry_parameters += ", uniform sampler2D gxm_screen_" + itos(i) + " : TEXUNIT" + itos(i + 1);
            }
        }
        const bool uses_fragcoord = has_token(body, "FRAGCOORD") || has_token(body, "SCREEN_UV");
        if (uses_fragcoord) {
            entry_parameters += ", float4 gxm_wpos : WPOS";
            if (!has_token(body, "SCREEN_PIXEL_SIZE") && !has_token(globals, "SCREEN_PIXEL_SIZE")) {
                entry_parameters += ", uniform float2 " + uniform_cg_name("SCREEN_PIXEL_SIZE");
            }
        }

        // Fragment-local built-ins. COLOR starts as texture * modulate,
        // exactly as in Godot's canvas.glsl, and is the final output.
        String locals = "float2 UV = gxm_uv;\n";
        if (uses_fragcoord) {
            locals += "float4 FRAGCOORD = float4(gxm_wpos.xy, gxm_wpos.z, 1.0);\n";
            locals += "float2 SCREEN_UV = gxm_wpos.xy * " + uniform_cg_name("SCREEN_PIXEL_SIZE") + ";\n";
        }
        locals += String("float4 gxm_COLOR = ") +
                (uses_main_texture ? "tex2D(TEXTURE, UV) * gxm_tint" : "gxm_tint") + ";\n";
        body = replace_token(body, "COLOR", "gxm_COLOR");

        // Global consts must not become uniforms.
        {
            String fixed;
            const PackedStringArray glines = globals.split("\n");
            for (int i = 0; i < glines.size(); i++) {
                const String t = glines[i].strip_edges();
                if (t.begins_with("const ")) fixed += "static " + t + "\n";
                else fixed += glines[i] + "\n";
            }
            globals = fixed;
        }

        String program = globals + "\nfloat4 main(" + entry_parameters + ") : COLOR {\n" +
                prologue + locals + body + "\nreturn gxm_COLOR;\n}\n";

        // GLSL -> Cg vocabulary.
        static const char *renames[][2] = {
            { "vec2", "float2" }, { "vec3", "float3" }, { "vec4", "float4" },
            { "ivec2", "int2" }, { "ivec3", "int3" }, { "ivec4", "int4" },
            { "uvec2", "int2" }, { "uvec3", "int3" }, { "uvec4", "int4" }, { "uint", "int" },
            { "bvec2", "bool2" }, { "bvec3", "bool3" }, { "bvec4", "bool4" },
            { "mix", "lerp" }, { "fract", "frac" }, { "inversesqrt", "rsqrt" },
            { "dFdx", "ddx" }, { "dFdy", "ddy" }, { "mod", "gxm_mod" },
        };
        for (const auto &r : renames) program = replace_token(program, r[0], r[1]);
        program = rewrite_calls(program, "atan", REWRITE_ATAN2);
        if (!screen_sampler.is_empty()) {
            program = rewrite_calls(program, "textureLod", REWRITE_SCREEN, screen_sampler);
            program = rewrite_calls(program, "texture", REWRITE_SCREEN, screen_sampler);
        }
        program = rewrite_calls(program, "textureLod", REWRITE_LOD);
        program = replace_token(program, "texture", "tex2D");
        static const char *constructors[] = {
            "float2", "float3", "float4", "int2", "int3", "int4", "bool2", "bool3", "bool4",
        };
        for (const char *c : constructors) program = rewrite_calls(program, c, REWRITE_CONSTRUCTOR);

        // Helpers emitted only when used.
        String prelude;
        if (program.find("gxm_mod(") >= 0) {
            // GLSL mod() floors; Cg fmod() truncates (differs for negatives).
            static const char *types[] = { "float", "float2", "float3", "float4" };
            for (const char *t : types) {
                prelude += String(t) + " gxm_mod(" + t + " x, " + t + " y) { return x - y * floor(x / y); }\n";
            }
            for (int i = 1; i < 4; i++) {
                prelude += String(types[i]) + " gxm_mod(" + types[i] + " x, float y) { return x - y * floor(x / y); }\n";
            }
        }
        static const char *constants[][2] = {
            { "PI", "3.1415926535897932384626433833" },
            { "TAU", "6.2831853071795864769252867666" },
            { "E", "2.7182818284590452353602874714" },
        };
        for (const auto &c : constants) {
            if (has_token(program, c[0])) {
                prelude += String("static const float ") + c[0] + " = " + c[1] + ";\n";
            }
        }
        if (!screen_sampler.is_empty()) {
            prelude += "float4 gxm_screen_sample(sampler2D s0, sampler2D s1, sampler2D s2, "
                       "sampler2D s3, sampler2D s4, sampler2D s5, float2 uv, float level) {\n"
                       "level = clamp(level, 0.0, 5.0);\n"
                       "if (level < 1.0) return lerp(tex2D(s0, uv), tex2D(s1, uv), level);\n"
                       "if (level < 2.0) return lerp(tex2D(s1, uv), tex2D(s2, uv), level - 1.0);\n"
                       "if (level < 3.0) return lerp(tex2D(s2, uv), tex2D(s3, uv), level - 2.0);\n"
                       "if (level < 4.0) return lerp(tex2D(s3, uv), tex2D(s4, uv), level - 3.0);\n"
                       "if (level < 5.0) return lerp(tex2D(s4, uv), tex2D(s5, uv), level - 4.0);\n"
                       "return tex2D(s5, uv);\n}\n";
        }
        if (reason) *reason = "native GXM canvas Cg prepared";
        return compact_blank_lines(prelude + statics + program);
    }
};
