#!/usr/bin/env python3
"""Host regression for the GXM canvas_item -> Cg translator.

Compiles platform/vita/gxm_shader_translator_vita.inc against a small String
adapter, translates a set of Godot canvas shaders and checks the Cg contract
(uniforms on main(), no COLOR identifier, modulate semantics, ...).

When NVIDIA's cgc is on PATH (Debian/Ubuntu: nvidia-cg-toolkit) every
translated program is also compiled with the fp40 profile. That is only a
syntax/type check with a related compiler: it does not prove that the Vita's
SceShaccCg accepts the program or that the GPU output is right.
"""
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve()
TRANSLATOR = HERE.parents[2] / 'gxm_shader_translator_vita.inc'
SELFTEST_SHADERS = HERE.parents[2] / 'tests' / 'v0_3_selftest' / 'shaders'

ADAPTER = r'''
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
template <class T> struct Vector : std::vector<T> {
    bool is_empty() const { return this->empty(); }
    int size() const { return (int)std::vector<T>::size(); }
};
struct CharString { std::string s; int length() const { return (int)s.size(); } const char *get_data() const { return s.c_str(); } };
struct String {
    std::string s;
    String() = default;
    String(const char *v) : s(v) {}
    String(std::string v) : s(std::move(v)) {}
    int length() const { return (int)s.size(); }
    bool is_empty() const { return s.empty(); }
    char32_t operator[](int i) const { return (unsigned char)s[i]; }
    int find(const String &v, int p = 0) const { if (p < 0) p = 0; auto n = s.find(v.s, p); return n == std::string::npos ? -1 : (int)n; }
    String substr(int p, int n = -1) const { if (p >= (int)s.size()) return String(); return s.substr(p, n < 0 ? std::string::npos : (size_t)n); }
    String left(int n) const { return s.substr(0, n); }
    String strip_edges() const { auto a = s.find_first_not_of(" \t\r\n"); if (a == std::string::npos) return String(); return s.substr(a, s.find_last_not_of(" \t\r\n") - a + 1); }
    bool begins_with(const String &v) const { return s.rfind(v.s, 0) == 0; }
    bool ends_with(const String &v) const { return s.size() >= v.s.size() && s.compare(s.size() - v.s.size(), v.s.size(), v.s) == 0; }
    bool contains(const String &v) const { return s.find(v.s) != std::string::npos; }
    Vector<String> split(const String &sep, bool allow_empty = true) const {
        Vector<String> out; size_t start = 0;
        while (true) { size_t at = s.find(sep.s, start); std::string part = s.substr(start, at == std::string::npos ? std::string::npos : at - start);
            if (allow_empty || !part.empty()) out.push_back(String(part)); if (at == std::string::npos) break; start = at + sep.s.size(); }
        return out;
    }
    CharString utf8() const { return CharString{ s }; }
    String operator+(const String &o) const { return s + o.s; }
    String &operator+=(const String &o) { s += o.s; return *this; }
    bool operator==(const String &o) const { return s == o.s; }
    bool operator!=(const String &o) const { return s != o.s; }
};
inline String operator+(const char *a, const String &b) { return String(a) + b; }
typedef Vector<String> PackedStringArray;
inline String itos(int v) { return std::to_string(v); }
'''

MAIN = r'''
#include <fstream>
#include <sstream>
int main(int argc, char **argv) {
    std::stringstream in; in << std::cin.rdbuf();
    String reason;
    String cg = GXMCanvasShaderTranslator::to_cg(String(in.str()), &reason);
    std::cout << "REASON:" << reason.s << "\n" << cg.s;
    return 0;
}
'''

SHADERS = {
    'uv_gradient': ('''shader_type canvas_item;
void fragment() {
    COLOR = vec4(UV.x, UV.y, 0.2, 1.0);
}''', True),
    'uniform_tint': ('''shader_type canvas_item;
uniform vec4 tint : source_color = vec4(1.0, 0.5, 0.5, 1.0);
uniform float amount : hint_range(0.0, 1.0) = 0.5;
void fragment() {
    vec4 c = texture(TEXTURE, UV);
    COLOR = mix(c, c * tint, amount);
}''', True),
    'time_wave_helper': ('''shader_type canvas_item;
// void vertex() { } (commented out: must not reject the shader)
uniform float speed = 2.0;
uniform int bands = 4;
uniform bool invert = false;
const float BASE = 0.25;
float wave(float x) {
    return sin(x * float(bands) + TIME * speed) * 0.5 + 0.5;
}
void fragment() {
    float w = wave(UV.x * TAU);
    if (invert) w = 1.0 - w;
    COLOR.rgb = vec3(w) * (1.0 - BASE) + BASE;
}''', True),
    'mod_atan_fract': ('''shader_type canvas_item;
void fragment() {
    vec2 p = UV * 2.0 - 1.0;
    float a = atan(p.y, p.x) / PI;
    float r = length(p);
    float rings = mod(r * 8.0 - TIME, 1.0);
    COLOR = vec4(vec3(fract(a * 4.0) * rings), 1.0);
}''', True),
    'outline_texture_pixel_size': ('''shader_type canvas_item;
uniform lowp vec4 line_color : source_color = vec4(0.0, 0.0, 0.0, 1.0);
uniform float width = 1.0;
void fragment() {
    vec2 s = TEXTURE_PIXEL_SIZE * width;
    float a = texture(TEXTURE, UV + vec2(s.x, 0.0)).a;
    a = max(a, texture(TEXTURE, UV - vec2(s.x, 0.0)).a);
    a = max(a, texture(TEXTURE, UV + vec2(0.0, s.y)).a);
    a = max(a, texture(TEXTURE, UV - vec2(0.0, s.y)).a);
    vec4 c = texture(TEXTURE, UV);
    COLOR = mix(vec4(line_color.rgb, a * line_color.a), c, c.a);
}''', True),
    'screen_blur_lod': ('''shader_type canvas_item;
uniform sampler2D screen_texture : hint_screen_texture, filter_linear_mipmap;
uniform float lod : hint_range(0.0, 5.0) = 2.0;
void fragment() {
    COLOR = textureLod(screen_texture, SCREEN_UV, lod);
}''', True),
    'extra_sampler_dissolve': ('''shader_type canvas_item;
uniform sampler2D noise : repeat_enable;
uniform float progress : hint_range(0.0, 1.0) = 0.0;
void fragment() {
    vec4 c = texture(TEXTURE, UV);
    float n = texture(noise, UV).r;
    c.a *= step(progress, n);
    COLOR = c;
}''', True),
    'light_function_ignored': ('''shader_type canvas_item;
void fragment() {
    COLOR = texture(TEXTURE, UV);
}
void light() {
    LIGHT = vec4(NORMAL, 1.0) * LIGHT_COLOR;
}''', True),
    'rejects_vertex': ('''shader_type canvas_item;
varying vec2 p;
void vertex() { p = VERTEX; }
void fragment() { COLOR = vec4(p, 0.0, 1.0); }''', False),
    'rejects_matrix': ('''shader_type canvas_item;
void fragment() { mat2 m = mat2(1.0); COLOR = vec4(m * UV, 0.0, 1.0); }''', False),
    'rejects_sampler_in_helper': ('''shader_type canvas_item;
uniform sampler2D noise;
float n(vec2 p) { return texture(noise, p).r; }
void fragment() { COLOR = vec4(n(UV)); }''', False),
}


def main():
    if not shutil.which('c++'):
        print('SKIP: c++ not found')
        return 0
    cgc = shutil.which('cgc')
    failures = []
    # The device self test draws these; they must all translate.
    for path in sorted(SELFTEST_SHADERS.glob('*.gdshader')):
        SHADERS['selftest/' + path.stem] = (path.read_text(), True)
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        src = tmp / 'translator_test.cpp'
        src.write_text(ADAPTER + TRANSLATOR.read_text() + MAIN)
        exe = tmp / 'translator_test'
        subprocess.run(['c++', '-std=c++17', '-O1', '-o', str(exe), str(src)], check=True)
        for name, (shader, supported) in SHADERS.items():
            out = subprocess.run([str(exe)], input=shader, capture_output=True, text=True, check=True).stdout
            reason, _, cg = out.partition('\n')
            reason = reason[len('REASON:'):]
            if not supported:
                if cg.strip():
                    failures.append(f'{name}: expected rejection, got Cg')
                else:
                    print(f'ok   {name}: rejected ({reason})')
                continue
            if not cg.strip():
                failures.append(f'{name}: rejected ({reason})')
                continue
            checks = [
                ('main() returns gxm_COLOR', 'return gxm_COLOR;' in cg),
                ('no COLOR identifier besides the semantic', cg.replace(') : COLOR', '').replace('gxm_COLOR', '').find('COLOR') < 0),
                ('no global uniform declarations', not any(l.startswith('uniform ') for l in cg.splitlines())),
                ('no GLSL vocabulary left', not any(t in cg for t in ('vec2', 'vec3', 'vec4', 'mix(', 'fract(', 'texture(', 'textureLod(', 'lowp'))),
                ('modulate applied before user code', 'gxm_tint;' in cg and 'gxm_COLOR * gxm_tint' not in cg),
            ]
            for label, ok in checks:
                if not ok:
                    failures.append(f'{name}: {label}')
            (tmp / f"{name.replace('/', '_')}.cg").write_text(cg)
            if cgc:
                r = subprocess.run([cgc, '-profile', 'fp40', '-entry', 'main', '-quiet', str(tmp / f"{name.replace('/', '_')}.cg")],
                                   capture_output=True, text=True)
                if r.returncode != 0:
                    failures.append(f'{name}: cgc fp40 failed\n{r.stdout}{r.stderr}\n--- Cg ---\n{cg}')
                    continue
            print(f'ok   {name}: {len(cg.encode())} bytes' + (' (cgc fp40 ok)' if cgc else ''))
    if not cgc:
        print('note: cgc not found, Cg syntax not checked')
    for f in failures:
        print('FAIL', f)
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
