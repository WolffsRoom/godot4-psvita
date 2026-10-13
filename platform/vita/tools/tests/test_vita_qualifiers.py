#!/usr/bin/env python3
"""Host regression for the actual Vita qualifier rewrite, using a small String adapter.
Requires c++ on PATH. It does not emulate the shader compiler or GPU.
"""
from pathlib import Path
import re
import subprocess
import tempfile

engine = Path(__file__).resolve().parents[4]
source = (engine / 'drivers/gles3/shader_gles3.cpp').read_text()
helper = source.split('static String _vita_replace_qualifier', 1)[1].split('static String _vita_downemit', 1)[0]
block = source.split('\t// Convert file-scope varying declarations', 1)[1]
block = block[block.index('\n\t{'):block.index('\n\t// Map the sampler types')]
block = block.replace('lines.write[i]', 'lines[i]')
adapter = r'''
#include <string>
#include <vector>
#include <iostream>
#include <iterator>
template<class T> struct Vector : std::vector<T> { bool is_empty() const { return this->empty(); } };
struct String {
 std::string s;
 String() = default; String(const char *v):s(v){} String(std::string v):s(v){}
 int length() const {return s.size();} bool is_empty() const {return s.empty();}
 char32_t operator[](int i) const {return s[i];}
 int find(const String &v, int p=0) const {auto n=s.find(v.s,p);return n==std::string::npos?-1:int(n);}
 String substr(int p, int n=-1) const {return s.substr(p,n<0?std::string::npos:n);}
 String strip_edges() const {auto a=s.find_first_not_of(" \t\r\n"); if(a==std::string::npos)return {};return s.substr(a,s.find_last_not_of(" \t\r\n")-a+1);}
 bool begins_with(const char*v) const {return s.rfind(v,0)==0;}
 bool ends_with(const char*v) const {std::string t=v;return s.size()>=t.size()&&s.compare(s.size()-t.size(),t.size(),t)==0;}
 Vector<String> split(const char*v) const {Vector<String> r;size_t p=0,n;std::string t=v;while((n=s.find(t,p))!=std::string::npos){r.push_back(s.substr(p,n-p));p=n+t.size();}r.push_back(s.substr(p));return r;}
 String operator+(const String&v)const{return s+v.s;} String&operator+=(const String&v){s+=v.s;return *this;}
};
'''
program = adapter + 'static String _vita_replace_qualifier' + helper
program += '\nString rewrite(String c, bool p_fragment) {' + block + '\nreturn c; }\n'
program += 'int main(){std::string s((std::istreambuf_iterator<char>(std::cin)),{});std::cout<<rewrite(s,true).s;}'
real = (engine / 'drivers/gles3/shaders/s4ao_inc.glsl').read_text()
real = re.sub(r'/\*.*?\*/|//[^\n]*', '', real, flags=re.S)
fixture = real + '\nin vec2 uv_interp;\nvoid helper(in vec2 input_uv, out vec2 output_uv) {\noutput_uv = input_uv;\n}\n'
with tempfile.TemporaryDirectory() as temp:
    cpp, exe = Path(temp)/'test.cpp', Path(temp)/'test'
    cpp.write_text(program)
    subprocess.run(['c++','-std=c++17',str(cpp),'-o',str(exe)],check=True)
    result = subprocess.check_output([str(exe)],input=fixture,text=True)
    assert 'varying vec2 uv_interp;' in result
    assert 'void helper(in vec2 input_uv, out vec2 output_uv)' in result
    nested = '#if A\nvoid f() {\n#if B\nif (true) {\n#else\nif (false) {\n#endif\n}\n}\n#else\nvoid f() {\n}\n#endif\nin vec2 uv_interp;\n'
    assert 'varying vec2 uv_interp;' in subprocess.check_output([str(exe)],input=nested,text=True)
    # Prove this fixture detects the old bug, rather than merely accepting
    # a declaration that the old implementation already handled.
    old_block = re.sub(r'\n\t\t\tif \(t\.begins_with\("#if"\)\).*?(?=\n\t\t\tif \(depth == 0)', '', block, flags=re.S)
    assert old_block != block
    cpp.write_text(program.replace(block, old_block))
    subprocess.run(['c++','-std=c++17',str(cpp),'-o',str(exe)],check=True)
    old_result = subprocess.check_output([str(exe)],input=fixture,text=True)
    assert 'in vec2 uv_interp;' in old_result
    assert 'varying vec2 uv_interp;' not in old_result
print('PASS: real SSAO branches, nested alternatives, and function parameter qualifiers')
