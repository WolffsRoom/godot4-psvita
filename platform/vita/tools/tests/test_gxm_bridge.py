#!/usr/bin/env python3
"""Check bridge submission and GPU buffer lifetime with a host fake of vita2d."""
from pathlib import Path
import subprocess
import tempfile

platform = Path(__file__).resolve().parents[2]
header = r'''
#include <stddef.h>
#include <stdint.h>
typedef struct { float x,y,z; uint32_t color; } vita2d_color_vertex;
typedef struct { float x,y,z,u,v; } vita2d_texture_vertex;
typedef struct { unsigned char data[64]; } vita2d_texture;
#define SCE_GXM_TEXTURE_FILTER_LINEAR 1
#define SCE_GXM_TEXTURE_FILTER_POINT 0
vita2d_texture *vita2d_create_empty_texture(unsigned int,unsigned int);
void vita2d_free_texture(vita2d_texture *);
void *vita2d_texture_get_datap(const vita2d_texture *);
unsigned int vita2d_texture_get_stride(const vita2d_texture *);
void vita2d_texture_set_filters(vita2d_texture *,int,int);
void vita2d_draw_array_textured(const vita2d_texture *,int,const vita2d_texture_vertex *,size_t,unsigned int);
#define SCE_GXM_PRIMITIVE_TRIANGLES 3
int vita2d_init(void);
void vita2d_start_drawing(void), vita2d_disable_clipping(void), vita2d_clear_screen(void);
void vita2d_set_clear_color(unsigned int);
void *vita2d_pool_memalign(unsigned int,unsigned int);
void vita2d_draw_array(int,const vita2d_color_vertex *,size_t);
void vita2d_set_clip_rectangle(int,int,int,int);
void vita2d_enable_clipping(void),vita2d_end_drawing(void),vita2d_wait_rendering_done(void);
void vita2d_swap_buffers(void),vita2d_pool_reset(void),vita2d_fini(void);
'''
harness = r'''
#include "vita2d.h"
#include "gxm_bridge.h"
#include <assert.h>
#include <stdlib.h>
static int init_result, started, ended, waited, swapped, reset, submitted, final;
static int freed, textured;
static union { vita2d_color_vertex color[6]; vita2d_texture_vertex tex[6]; } vertices;
#define pool vertices.color
static vita2d_texture *allocated;
vita2d_texture *vita2d_create_empty_texture(unsigned int w,unsigned int h){assert(w==3&&h==2);allocated=calloc(1,sizeof(*allocated));return allocated;}
void vita2d_free_texture(vita2d_texture *t){assert(waited);freed++;free(t);}
void *vita2d_texture_get_datap(const vita2d_texture *t){return (void *)t->data;}
unsigned int vita2d_texture_get_stride(const vita2d_texture *t){(void)t;return 32;}
void vita2d_texture_set_filters(vita2d_texture *t,int a,int b){assert(t==allocated&&a==1&&b==1);}
void vita2d_draw_array_textured(const vita2d_texture *t,int mode,const vita2d_texture_vertex *v,size_t n,unsigned int color){
 assert(t==allocated&&mode==3&&n==6&&color==0x804040ff);
 assert(v[0].u==0.25f&&v[0].v==0.0f&&v[2].u==0.5f&&v[2].v==1.0f);
 assert(v[5].x==80&&v[5].y==200);textured++;
}
int vita2d_init(void){return init_result;}
void vita2d_start_drawing(void){started++;}
void vita2d_disable_clipping(void){}
void vita2d_clear_screen(void){}
void vita2d_set_clear_color(unsigned int c){assert(c==0xff000000);}
void *vita2d_pool_memalign(unsigned int n,unsigned int a){assert(n<=sizeof(vertices)&&a==8);return &vertices;}
void vita2d_draw_array(int mode,const vita2d_color_vertex *p,size_t n){
 assert(mode==3&&n==6&&p==pool);submitted++;
 assert(p[0].x==80&&p[0].y==120&&p[1].x==240&&p[1].y==120);
 assert(p[2].x==240&&p[2].y==200&&p[3].x==80&&p[3].y==120);
 assert(p[4].x==240&&p[4].y==200&&p[5].x==80&&p[5].y==200);
 for(int i=0;i<6;i++)assert(p[i].color==0xff4040ff&&p[i].z==0.5f);
}
void vita2d_set_clip_rectangle(int x,int y,int a,int b){assert(x==10&&y==20&&a==40&&b==60);}
void vita2d_enable_clipping(void){}
void vita2d_end_drawing(void){ended++;}
void vita2d_wait_rendering_done(void){assert(ended);waited++;}
void vita2d_swap_buffers(void){assert(waited);swapped++;}
void vita2d_pool_reset(void){assert(waited);reset++;}
void vita2d_fini(void){final++;}
int main(void){
 float xy[]={80,120,240,120,240,200,80,200};
 assert(!vita_gxm_init());vita_gxm_begin(0xff000000);assert(!started);
 init_result=1;assert(vita_gxm_init());
 vita_gxm_quad(xy,0xff4040ff);assert(!submitted);
 vita_gxm_begin(0xff000000);vita_gxm_begin(0xff000000);assert(started==1);
 vita_gxm_clip(1,10,20,30,40);vita_gxm_quad(xy,0xff4040ff);assert(submitted==1);
 unsigned char rgba[24];for(int i=0;i<24;i++)rgba[i]=i;
 assert(!vita_gxm_texture_create(0,2,rgba));
 assert(!vita_gxm_texture_create(4097,2,rgba));
 void *texture=vita_gxm_texture_create(3,2,rgba);assert(texture);
 for(int i=0;i<12;i++){assert(allocated->data[i]==i);assert(allocated->data[32+i]==12+i);}
 for(int i=12;i<32;i++)assert(allocated->data[i]==0);
 float uv[]={0.25f,0,0.5f,0,0.5f,1,0.25f,1};
 vita_gxm_texture_quad(texture,xy,uv,0x804040ff,1);assert(textured==1);
 vita_gxm_texture_free(texture);assert(!freed); // GPU still references it.
 vita_gxm_end(1);assert(ended==1&&waited==1&&swapped==1&&reset==1);
 assert(freed==1);
 vita_gxm_end(1);assert(ended==1);
 vita_gxm_finish();vita_gxm_finish();assert(final==1);
}
'''
with tempfile.TemporaryDirectory() as temp:
    root = Path(temp)
    (root/'vita2d.h').write_text(header)
    (root/'test.c').write_text(harness)
    exe=root/'test'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-I'+temp,'-I'+str(platform),str(root/'test.c'),str(platform/'loader/gxm_bridge.c'),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS: init, geometry/color, clipping, NPOT row pitch, UV/tint/filter, deferred texture free, GPU buffer lifetime')
