#include <stdint.h>
#include "../gxm_bridge.h"
typedef struct { char *symbol; uintptr_t func; } so_default_dynlib;

extern int FT_Activate_Size();
extern int FT_Add_Default_Modules();
extern int FT_Done_Face();
extern int FT_Done_FreeType();
extern int FT_Done_Glyph();
extern int FT_Done_Library();
extern int FT_Done_MM_Var();
extern int FT_Done_Size();
extern int FT_Error_String();
extern int FT_Face_GetCharVariantIndex();
extern int FT_Get_Advance();
extern int FT_Get_Char_Index();
extern int FT_Get_Color_Glyph_ClipBox();
extern int FT_Get_Color_Glyph_Layer();
extern int FT_Get_Color_Glyph_Paint();
extern int FT_Get_Colorline_Stops();
extern int FT_Get_First_Char();
extern int FT_Get_Glyph();
extern int FT_Get_Glyph_Name();
extern int FT_Get_Kerning();
extern int FT_Get_MM_Var();
extern int FT_Get_Name_Index();
extern int FT_Get_Next_Char();
extern int FT_Get_Paint();
extern int FT_Get_Paint_Layers();
extern int FT_Get_Sfnt_Table();
extern int FT_Get_Var_Design_Coordinates();
extern int FT_Glyph_Stroke();
extern int FT_Glyph_To_Bitmap();
extern int FT_Init_FreeType();
extern int FT_Load_Glyph();
extern int FT_Load_Sfnt_Table();
extern int FT_MulFix();
extern int FT_New_Face();
extern int FT_New_Library();
extern int FT_New_Memory_Face();
extern int FT_New_Size();
extern int FT_Open_Face();
extern int FT_Outline_Decompose();
extern int FT_Outline_Embolden();
extern int FT_Outline_Get_Orientation();
extern int FT_Outline_Transform();
extern int FT_Outline_Translate();
extern int FT_Palette_Data_Get();
extern int FT_Palette_Select();
extern int FT_Property_Set();
extern int FT_Reference_Face();
extern int FT_Reference_Library();
extern int FT_Render_Glyph();
extern int FT_Request_Size();
extern int FT_Select_Charmap();
extern int FT_Select_Size();
extern int FT_Set_Char_Size();
extern int FT_Set_Default_Properties();
extern int FT_Set_Transform();
extern int FT_Set_Var_Design_Coordinates();
extern int FT_Sfnt_Table_Info();
extern int FT_Stroker_Done();
extern int FT_Stroker_New();
extern int FT_Stroker_Set();
extern int ZSTD_CCtx_refPrefix();
extern int ZSTD_CCtx_setParameter();
extern int ZSTD_CCtx_setPledgedSrcSize();
extern int ZSTD_DCtx_refPrefix();
extern int ZSTD_DCtx_setParameter();
extern int ZSTD_compress2();
extern int ZSTD_compressBound();
extern int ZSTD_compressCCtx();
extern int ZSTD_createCCtx();
extern int ZSTD_createDCtx();
extern int ZSTD_decompress();
extern int ZSTD_decompressDCtx();
extern int ZSTD_freeCCtx();
extern int ZSTD_freeDCtx();
extern int ZSTD_getErrorCode();
extern int ZSTD_getErrorString();
extern int ZSTD_getFrameContentSize();
extern int ZSTD_isError();
extern int _ZNKSt12__basic_fileIcE7is_openEv();
extern int _ZNKSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE12find_last_ofEPKcj();
extern int _ZNKSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE4findERKS4_j();
extern int _ZNKSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE5rfindEcj();
extern int _ZNKSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE6substrEjj();
extern int _ZNKSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE7compareEPKc();
extern int _ZNKSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE7compareERKS4_();
extern int _ZNKSt7__cxx1115basic_stringbufIcSt11char_traitsIcESaIcEE3strEv();
extern int _ZNKSt9type_infoeqERKS_();
extern int _ZNSo9_M_insertIdEERSoT_();
extern int _ZNSo9_M_insertImEERSoT_();
extern int _ZNSo9_M_insertIxEERSoT_();
extern int _ZNSolsEPFRSoS_E();
extern int _ZNSolsEf();
extern int _ZNSolsEi();
extern int _ZNSt13basic_filebufIcSt11char_traitsIcEEC1Ev();
extern int _ZNSt14basic_ofstreamIcSt11char_traitsIcEE4openEPKcSt13_Ios_Openmode();
extern int _ZNSt14basic_ofstreamIcSt11char_traitsIcEE5closeEv();
extern int _ZNSt14basic_ofstreamIcSt11char_traitsIcEED1Ev();
extern int _ZNSt18condition_variable10notify_allEv();
extern int _ZNSt18condition_variable10notify_oneEv();
extern int _ZNSt18condition_variable4waitERSt11unique_lockISt5mutexE();
extern int _ZNSt18condition_variableC1Ev();
extern int _ZNSt18condition_variableD1Ev();
extern int _ZNSt6localeC1Ev();
extern int _ZNSt6thread15_M_start_threadESt10unique_ptrINS_6_StateESt14default_deleteIS1_EEPFvvE();
extern int _ZNSt6thread20hardware_concurrencyEv();
extern int _ZNSt6thread4joinEv();
extern int _ZNSt6thread6_StateD2Ev();
extern int _ZNSt6thread6detachEv();
extern int _ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv();
extern int _ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_replaceEjjPKcj();
extern int _ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE6appendEPKc();
extern int _ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE6appendEPKcj();
extern int _ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE6appendERKS4_();
extern int _ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE6insertEjPKc();
extern int _ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE6insertEjRKS4_();
extern int _ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE6resizeEj();
extern int _ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE7_S_copyEPcPKcj();
extern int _ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE7reserveEj();
extern int _ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE8pop_backEv();
extern int _ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE9_M_appendEPKcj();
extern int _ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE9_M_assignERKS4_();
extern int _ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE9_M_createERjj();
extern int _ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE9push_backEc();
extern int _ZNSt7__cxx1115basic_stringbufIcSt11char_traitsIcESaIcEE3strERKNS_12basic_stringIcS2_S3_EE();
extern int _ZNSt7__cxx1118basic_stringstreamIcSt11char_traitsIcESaIcEED1Ev();
extern int _ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEED1Ev();
extern int _ZNSt8__detail15_List_node_base11_M_transferEPS0_S1_();
extern int _ZNSt8__detail15_List_node_base7_M_hookEPS0_();
extern int _ZNSt8__detail15_List_node_base9_M_unhookEv();
extern int _ZNSt8ios_baseC2Ev();
extern int _ZNSt9basic_iosIcSt11char_traitsIcEE4initEPSt15basic_streambufIcS1_E();
extern int _ZSt14__once_functor();
extern int _ZSt16__get_once_mutexv();
extern int _ZSt17__throw_bad_allocv();
extern int _ZSt18_Rb_tree_decrementPSt18_Rb_tree_node_base();
extern int _ZSt19__throw_logic_errorPKc();
extern int _ZSt20__throw_length_errorPKc();
extern int _ZSt20__throw_system_errori();
extern int _ZSt24__throw_out_of_range_fmtPKcz();
extern int _ZSt25__throw_bad_function_callv();
extern int _ZSt27__set_once_functor_lock_ptrPSt11unique_lockISt5mutexE();
extern int _ZSt28__throw_bad_array_new_lengthv();
extern int _ZSt29_Rb_tree_insert_and_rebalancebPSt18_Rb_tree_node_baseS0_RS_();
extern int _ZSt4endlIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_();
extern int _ZSt9terminatev();
extern int _ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc();
extern int _ZStlsIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_St8_SetfillIS3_E();
extern int _ZStlsIcSt11char_traitsIcESaIcEERSt13basic_ostreamIT_T0_ES7_RKNSt7__cxx1112basic_stringIS4_S5_T1_EE();
extern int _ZTHN12MessageQueue16thread_singletonE();
extern int _ZTHN14ResourceLoader12load_nestingE();
extern int _ZTHN16GDScriptLanguage11_call_stackE();
extern int _ZTHN16GDScriptLanguage16_call_stack_sizeE();
extern int _ZTHN16GDScriptLanguage21_debug_parse_err_lineE();
extern int _ZTHN4Node28current_process_thread_groupE();
extern int _ZTINSt6thread6_StateE();
extern int _ZTTNSt7__cxx1118basic_stringstreamIcSt11char_traitsIcESaIcEEE();
extern int _ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE();
extern int _ZTTSt14basic_ofstreamIcSt11char_traitsIcEE();
extern int _ZTVN10__cxxabiv117__class_type_infoE();
extern int _ZTVN10__cxxabiv120__si_class_type_infoE();
extern int _ZTVN10__cxxabiv121__vmi_class_type_infoE();
extern int _ZTVNSt7__cxx1115basic_stringbufIcSt11char_traitsIcESaIcEEE();
extern int _ZTVNSt7__cxx1118basic_stringstreamIcSt11char_traitsIcESaIcEEE();
extern int _ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE();
extern int _ZTVSt14basic_ofstreamIcSt11char_traitsIcEE();
extern int _ZTVSt15basic_streambufIcSt11char_traitsIcEE();
extern int _ZTVSt9basic_iosIcSt11char_traitsIcEE();
extern int _ZdaPv();
extern int _ZdaPvj();
extern int _ZdlPv();
extern int _ZdlPvj();
extern int _Znaj();
extern int _Znwj();
extern int _ZnwjRKSt9nothrow_t();
extern int __aeabi_atexit();
extern int __aeabi_d2lz();
extern int __aeabi_d2ulz();
extern int __aeabi_f2lz();
extern int __aeabi_f2ulz();
extern int __aeabi_idiv();
extern int __aeabi_idivmod();
extern int __aeabi_l2d();
extern int __aeabi_l2f();
extern int __aeabi_ldivmod();
extern int __aeabi_uidiv();
extern int __aeabi_uidivmod();
extern int __aeabi_ul2d();
extern int __aeabi_ul2f();
extern int __aeabi_uldivmod();
extern int __ctzdi2();
extern int __cxa_guard_acquire();
extern int __cxa_guard_release();
extern int __cxa_pure_virtual();
extern int __cxa_thread_atexit();
extern int __dynamic_cast();
extern int __emutls_get_address();
extern int __errno();
extern int __once_proxy();
extern int __popcountdi2();
extern int __popcountsi2();
extern int _ctype_();
extern int _impure_ptr();
extern int _sceAppMgrGetBudgetInfo();
extern int _timezone();
extern int _tzname();
extern int abort();
extern int accept();
extern int access();
extern int acos();
extern int acosf();
extern int acosh();
extern int asin();
extern int asinf();
extern int asinh();
extern int atan();
extern int atan2();
extern int atan2f();
extern int atanf();
extern int atanh();
extern int atexit();
extern int atof();
extern int atoi();
extern int bind();
extern int calloc();
extern int cbrtf();
extern int ceil();
extern int ceilf();
extern int chdir();
extern int chmod();
extern int clock();
extern int clock_gettime();
extern int close();
extern int closedir();
extern int compress();
extern int compressBound();
extern int connect();
extern int cos();
extern int cosf();
extern int cosh();
extern int crc32();
extern int deflate();
extern int deflateBound();
extern int deflateEnd();
extern int deflateInit2_();
extern int dlclose();
extern int dlerror();
extern int dlopen();
extern int dlsym();
extern int exit();
extern int exp();
extern int exp2();
extern int exp2f();
extern int expf();
extern int fchmod();
extern int fclose();
extern int fcntl();
extern int fdopen();
extern int feof();
extern int ferror();
extern int fflush();
extern int fgetpos();
extern int fgets();
extern int fileno();
extern int floor();
extern int floorf();
extern int fmax();
extern int fmaxf();
extern int fmin();
extern int fmod();
extern int fmodf();
extern int fopen();
extern int fork();
extern int fprintf();
extern int fputc();
extern int fputs();
extern int fread();
extern int free();
extern int freeaddrinfo();
extern int freopen();
extern int fscanf();
extern int fseek();
extern int fseeko();
extern int fsetpos();
extern int fstat();
extern int ftell();
extern int ftello();
extern int ftruncate();
extern int fwrite();
extern int get_crc_table();
extern int getaddrinfo();
extern int getc();
extern int getcwd();
extern int getenv();
extern int getpid();
extern int getsockname();
extern int gettimeofday();
extern int glActiveTexture();
extern int glAttachShader();
extern int glBindBuffer();
extern int glBindBufferBase();
extern int glBindBufferRange();
extern int glBindFramebuffer();
extern int glBindRenderbuffer();
extern int glBindTexture();
extern int glBindVertexArray();
extern int glBlendEquation();
extern int glBlendFunc();
extern int glBlendFuncSeparate();
extern int glBlitFramebuffer();
extern int glBufferData();
extern int glBufferSubData();
extern int glCheckFramebufferStatus();
extern int glClear();
extern int glClearColor();
extern int glClearDepthf();
extern int glClearStencil();
extern int glColorMask();
extern int glCompileShader();
extern int glCompressedTexImage2D();
extern int glCreateProgram();
extern int glCreateShader();
extern int glCullFace();
extern int glDeleteBuffers();
extern int glDeleteFramebuffers();
extern int glDeleteProgram();
extern int glDeleteQueries();
extern int glDeleteRenderbuffers();
extern int glDeleteShader();
extern int glDeleteTextures();
extern int glDeleteVertexArrays();
extern int glDepthFunc();
extern int glDepthMask();
extern int glDisable();
extern int glDisableVertexAttribArray();
extern int glDrawArrays();
extern int glDrawArraysInstanced();
extern int glDrawElements();
extern int glDrawElementsInstanced();
extern int glEnable();
extern int glEnableVertexAttribArray();
extern int glFinish();
extern int glFramebufferRenderbuffer();
extern int glFramebufferTexture2D();
extern int glFrontFace();
extern int glGenBuffers();
extern int glGenFramebuffers();
extern int glGenQueries();
extern int glGenRenderbuffers();
extern int glGenTextures();
extern int glGenVertexArrays();
extern int glGenerateMipmap();
extern int glGetFloatv();
extern int glGetIntegerv();
extern int glGetProgramBinary();
extern int glGetProgramInfoLog();
extern int glGetProgramiv();
extern int glGetShaderInfoLog();
extern int glGetShaderiv();
extern int glGetString();
extern int glGetStringi();
extern int glGetUniformBlockIndex();
extern int glGetUniformLocation();
extern int glLinkProgram();
extern int glMapBufferRange();
extern int glPixelStorei();
extern int glProgramBinary();
extern int glReadBuffer();
extern int glReadPixels();
extern int glRenderbufferStorage();
extern int glScissor();
extern int glShaderSource();
extern int glStencilFunc();
extern int glStencilMask();
extern int glStencilOp();
extern int glTexImage2D();
extern int glTexParameterf();
extern int glTexParameteri();
extern int glUniform1f();
extern int glUniform1i();
extern int glUniform1iv();
extern int glUniform2f();
extern int glUniform2fv();
extern int glUniform2iv();
extern int glUniform3fv();
extern int glUniform4f();
extern int glUniform4fv();
extern int glUniformBlockBinding();
extern int glUniformMatrix3fv();
extern int glUniformMatrix4fv();
extern int glUnmapBuffer();
extern int glUseProgram();
extern int glVertexAttrib4f();
extern int glVertexAttribDivisor();
extern int glVertexAttribPointer();
extern int glViewport();
extern int gmtime_r();
extern int hypotf();
extern int ilogb();
extern int inflate();
extern int inflateEnd();
extern int inflateInit2_();
extern int inflateReset();
extern int isalpha();
extern int isatty();
extern int isspace();
extern int isxdigit();
extern int kill();
extern int ldexp();
extern int ldexpf();
extern int listen();
extern int localtime_r();
extern int log();
extern int log10();
extern int log10f();
extern int log2();
extern int log2f();
extern int logf();
extern int longjmp();
extern int lrintf();
extern int lstat();
extern int malloc();
extern int memalign();
extern int memchr();
extern int memcmp();
extern int memcpy();
extern int memmove();
extern int memset();
extern int mkdir();
extern int mkstemp();
extern int modf();
extern int nanosleep();
extern int nearbyint();
extern int nearbyintf();
extern int nextafter();
extern int nl_langinfo();
extern int ogg_page_bos();
extern int ogg_page_granulepos();
extern int ogg_page_serialno();
extern int ogg_stream_check();
extern int ogg_stream_clear();
extern int ogg_stream_init();
extern int ogg_stream_packetout();
extern int ogg_stream_pagein();
extern int ogg_stream_reset();
extern int ogg_sync_buffer();
extern int ogg_sync_check();
extern int ogg_sync_clear();
extern int ogg_sync_init();
extern int ogg_sync_pageout();
extern int ogg_sync_reset();
extern int ogg_sync_wrote();
extern int open();
extern int opendir();
extern int pcre2_code_free_32();
extern int pcre2_compile_32();
extern int pcre2_compile_context_create_32();
extern int pcre2_compile_context_free_32();
extern int pcre2_general_context_create_32();
extern int pcre2_general_context_free_32();
extern int pcre2_get_error_message_32();
extern int pcre2_get_ovector_count_32();
extern int pcre2_get_ovector_pointer_32();
extern int pcre2_match_32();
extern int pcre2_match_context_create_32();
extern int pcre2_match_context_free_32();
extern int pcre2_match_data_create_from_pattern_32();
extern int pcre2_match_data_free_32();
extern int pcre2_pattern_info_32();
extern int pcre2_substitute_32();
extern int pipe();
extern int png_create_info_struct();
extern int png_create_read_struct();
extern int png_create_write_struct();
extern int png_destroy_read_struct();
extern int png_destroy_write_struct();
extern int png_error();
extern int png_get_IHDR();
extern int png_get_bit_depth();
extern int png_get_channels();
extern int png_get_io_ptr();
extern int png_get_rowbytes();
extern int png_get_valid();
extern int png_image_begin_read_from_file();
extern int png_image_begin_read_from_memory();
extern int png_image_finish_read();
extern int png_image_free();
extern int png_image_write_to_memory();
extern int png_read_end();
extern int png_read_image();
extern int png_read_info();
extern int png_read_update_info();
extern int png_set_IHDR();
extern int png_set_add_alpha();
extern int png_set_expand_gray_1_2_4_to_8();
extern int png_set_gray_to_rgb();
extern int png_set_interlace_handling();
extern int png_set_longjmp_fn();
extern int png_set_palette_to_rgb();
extern int png_set_read_fn();
extern int png_set_strip_16();
extern int png_set_tRNS_to_alpha();
extern int png_set_write_fn();
extern int png_write_end();
extern int png_write_image();
extern int png_write_info();
extern int poll();
extern int pow();
extern int powf();
extern int printf();
extern int pthread_cancel();
extern int pthread_create();
extern int pthread_join();
extern int pthread_mutex_destroy();
extern int pthread_mutex_init();
extern int pthread_mutex_lock();
extern int pthread_mutex_trylock();
extern int pthread_mutex_unlock();
extern int pthread_once();
extern int pthread_rwlock_rdlock();
extern int pthread_rwlock_unlock();
extern int pthread_rwlock_wrlock();
extern int pthread_self();
extern int qsort();
extern int raise();
extern int rand();
extern int read();
extern int readdir();
extern int realloc();
extern int recv();
extern int recvfrom();
extern int remainder();
extern int rename();
extern int rewind();
extern int rint();
extern int rintf();
extern int rmdir();
extern int round();
extern int roundf();
extern int sceAudioOutOpenPort();
extern int sceAudioOutOutput();
extern int sceAudioOutReleasePort();
extern int sceClibPrintf();
extern int sceCtrlPeekBufferPositive();
extern int sceCtrlSetSamplingMode();
extern int sceIoClose();
extern int sceIoOpen();
extern int sceIoRemove();
extern int sceIoWrite();
extern int sceKernelExitProcess();
extern int sceKernelGetRandomNumber();
extern int sceMotionGetState();
extern int sceMotionStartSampling();
extern int scePowerSetArmClockFrequency();
extern int scePowerSetBusClockFrequency();
extern int scePowerSetGpuClockFrequency();
extern int scePowerSetGpuXbarClockFrequency();
extern int sceTouchGetPanelInfo();
extern int sceTouchPeek();
extern int sceTouchSetSamplingState();
extern int send();
extern int sendto();
extern int setenv();
extern int setjmp();
extern int setlocale();
extern int setsid();
extern int setsockopt();
extern int signal();
extern int sin();
extern int sinf();
extern int sinh();
extern int snprintf();
extern int socket();
extern int sprintf();
extern int sqrt();
extern int sqrtf();
extern int srand();
extern int sscanf();
extern int stat();
extern int statvfs();
extern int strcasecmp();
extern int strcat();
extern int strchr();
extern int strcmp();
extern int strcpy();
extern int strdup();
extern int strerror();
extern int strftime();
extern int strlen();
extern int strncat();
extern int strncmp();
extern int strncpy();
extern int strnlen();
extern int strrchr();
extern int strstr();
extern int strtod();
extern int strtok_r();
extern int strtol();
extern int strtoul();
extern int tan();
extern int tanf();
extern int tanh();
extern int tanhf();
extern int th_comment_clear();
extern int th_comment_init();
extern int th_decode_alloc();
extern int th_decode_ctl();
extern int th_decode_free();
extern int th_decode_headerin();
extern int th_decode_packetin();
extern int th_decode_ycbcr_out();
extern int th_granule_frame();
extern int th_granule_time();
extern int th_info_clear();
extern int th_info_init();
extern int th_packet_iskeyframe();
extern int th_setup_free();
extern int time();
extern int tolower();
extern int tzset();
extern int uncompress();
extern int unlink();
extern int unsetenv();
extern int vfprintf();
extern int vglInitWithCustomThreshold();
extern int vglSetParamBufferSize();
extern int vglSetSemanticBindingMode();
extern int vglSwapBuffers();
extern int vglUseTripleBuffering();
extern int vorbis_block_clear();
extern int vorbis_block_init();
extern int vorbis_comment_clear();
extern int vorbis_comment_init();
extern int vorbis_dsp_clear();
extern int vorbis_granule_time();
extern int vorbis_info_clear();
extern int vorbis_info_init();
extern int vorbis_synthesis();
extern int vorbis_synthesis_blockin();
extern int vorbis_synthesis_headerin();
extern int vorbis_synthesis_idheader();
extern int vorbis_synthesis_init();
extern int vorbis_synthesis_pcmout();
extern int vorbis_synthesis_read();
extern int vorbis_synthesis_restart();
extern int vorbis_synthesis_trackonly();
extern int vprintf();
extern int vsnprintf();
extern int wcscmp();
extern int wcscpy();
extern int write();

so_default_dynlib default_dynlib[] = {
  { "vita_gxm_init", (uintptr_t)&vita_gxm_init },
  { "vita_gxm_texture_create", (uintptr_t)&vita_gxm_texture_create },
  { "vita_gxm_texture_free", (uintptr_t)&vita_gxm_texture_free },
  { "vita_gxm_texture_quad", (uintptr_t)&vita_gxm_texture_quad },
  { "vita_gxm_begin", (uintptr_t)&vita_gxm_begin },
  { "vita_gxm_quad", (uintptr_t)&vita_gxm_quad },
  { "vita_gxm_clip", (uintptr_t)&vita_gxm_clip },
  { "vita_gxm_end", (uintptr_t)&vita_gxm_end },
  { "vita_gxm_finish", (uintptr_t)&vita_gxm_finish },
  { "FT_Activate_Size", (uintptr_t)&FT_Activate_Size },
  { "FT_Add_Default_Modules", (uintptr_t)&FT_Add_Default_Modules },
  { "FT_Done_Face", (uintptr_t)&FT_Done_Face },
  { "FT_Done_FreeType", (uintptr_t)&FT_Done_FreeType },
  { "FT_Done_Glyph", (uintptr_t)&FT_Done_Glyph },
  { "FT_Done_Library", (uintptr_t)&FT_Done_Library },
  { "FT_Done_MM_Var", (uintptr_t)&FT_Done_MM_Var },
  { "FT_Done_Size", (uintptr_t)&FT_Done_Size },
  { "FT_Error_String", (uintptr_t)&FT_Error_String },
  { "FT_Face_GetCharVariantIndex", (uintptr_t)&FT_Face_GetCharVariantIndex },
  { "FT_Get_Advance", (uintptr_t)&FT_Get_Advance },
  { "FT_Get_Char_Index", (uintptr_t)&FT_Get_Char_Index },
  { "FT_Get_Color_Glyph_ClipBox", (uintptr_t)&FT_Get_Color_Glyph_ClipBox },
  { "FT_Get_Color_Glyph_Layer", (uintptr_t)&FT_Get_Color_Glyph_Layer },
  { "FT_Get_Color_Glyph_Paint", (uintptr_t)&FT_Get_Color_Glyph_Paint },
  { "FT_Get_Colorline_Stops", (uintptr_t)&FT_Get_Colorline_Stops },
  { "FT_Get_First_Char", (uintptr_t)&FT_Get_First_Char },
  { "FT_Get_Glyph", (uintptr_t)&FT_Get_Glyph },
  { "FT_Get_Glyph_Name", (uintptr_t)&FT_Get_Glyph_Name },
  { "FT_Get_Kerning", (uintptr_t)&FT_Get_Kerning },
  { "FT_Get_MM_Var", (uintptr_t)&FT_Get_MM_Var },
  { "FT_Get_Name_Index", (uintptr_t)&FT_Get_Name_Index },
  { "FT_Get_Next_Char", (uintptr_t)&FT_Get_Next_Char },
  { "FT_Get_Paint", (uintptr_t)&FT_Get_Paint },
  { "FT_Get_Paint_Layers", (uintptr_t)&FT_Get_Paint_Layers },
  { "FT_Get_Sfnt_Table", (uintptr_t)&FT_Get_Sfnt_Table },
  { "FT_Get_Var_Design_Coordinates", (uintptr_t)&FT_Get_Var_Design_Coordinates },
  { "FT_Glyph_Stroke", (uintptr_t)&FT_Glyph_Stroke },
  { "FT_Glyph_To_Bitmap", (uintptr_t)&FT_Glyph_To_Bitmap },
  { "FT_Init_FreeType", (uintptr_t)&FT_Init_FreeType },
  { "FT_Load_Glyph", (uintptr_t)&FT_Load_Glyph },
  { "FT_Load_Sfnt_Table", (uintptr_t)&FT_Load_Sfnt_Table },
  { "FT_MulFix", (uintptr_t)&FT_MulFix },
  { "FT_New_Face", (uintptr_t)&FT_New_Face },
  { "FT_New_Library", (uintptr_t)&FT_New_Library },
  { "FT_New_Memory_Face", (uintptr_t)&FT_New_Memory_Face },
  { "FT_New_Size", (uintptr_t)&FT_New_Size },
  { "FT_Open_Face", (uintptr_t)&FT_Open_Face },
  { "FT_Outline_Decompose", (uintptr_t)&FT_Outline_Decompose },
  { "FT_Outline_Embolden", (uintptr_t)&FT_Outline_Embolden },
  { "FT_Outline_Get_Orientation", (uintptr_t)&FT_Outline_Get_Orientation },
  { "FT_Outline_Transform", (uintptr_t)&FT_Outline_Transform },
  { "FT_Outline_Translate", (uintptr_t)&FT_Outline_Translate },
  { "FT_Palette_Data_Get", (uintptr_t)&FT_Palette_Data_Get },
  { "FT_Palette_Select", (uintptr_t)&FT_Palette_Select },
  { "FT_Property_Set", (uintptr_t)&FT_Property_Set },
  { "FT_Reference_Face", (uintptr_t)&FT_Reference_Face },
  { "FT_Reference_Library", (uintptr_t)&FT_Reference_Library },
  { "FT_Render_Glyph", (uintptr_t)&FT_Render_Glyph },
  { "FT_Request_Size", (uintptr_t)&FT_Request_Size },
  { "FT_Select_Charmap", (uintptr_t)&FT_Select_Charmap },
  { "FT_Select_Size", (uintptr_t)&FT_Select_Size },
  { "FT_Set_Char_Size", (uintptr_t)&FT_Set_Char_Size },
  { "FT_Set_Default_Properties", (uintptr_t)&FT_Set_Default_Properties },
  { "FT_Set_Transform", (uintptr_t)&FT_Set_Transform },
  { "FT_Set_Var_Design_Coordinates", (uintptr_t)&FT_Set_Var_Design_Coordinates },
  { "FT_Sfnt_Table_Info", (uintptr_t)&FT_Sfnt_Table_Info },
  { "FT_Stroker_Done", (uintptr_t)&FT_Stroker_Done },
  { "FT_Stroker_New", (uintptr_t)&FT_Stroker_New },
  { "FT_Stroker_Set", (uintptr_t)&FT_Stroker_Set },
  { "ZSTD_CCtx_refPrefix", (uintptr_t)&ZSTD_CCtx_refPrefix },
  { "ZSTD_CCtx_setParameter", (uintptr_t)&ZSTD_CCtx_setParameter },
  { "ZSTD_CCtx_setPledgedSrcSize", (uintptr_t)&ZSTD_CCtx_setPledgedSrcSize },
  { "ZSTD_DCtx_refPrefix", (uintptr_t)&ZSTD_DCtx_refPrefix },
  { "ZSTD_DCtx_setParameter", (uintptr_t)&ZSTD_DCtx_setParameter },
  { "ZSTD_compress2", (uintptr_t)&ZSTD_compress2 },
  { "ZSTD_compressBound", (uintptr_t)&ZSTD_compressBound },
  { "ZSTD_compressCCtx", (uintptr_t)&ZSTD_compressCCtx },
  { "ZSTD_createCCtx", (uintptr_t)&ZSTD_createCCtx },
  { "ZSTD_createDCtx", (uintptr_t)&ZSTD_createDCtx },
  { "ZSTD_decompress", (uintptr_t)&ZSTD_decompress },
  { "ZSTD_decompressDCtx", (uintptr_t)&ZSTD_decompressDCtx },
  { "ZSTD_freeCCtx", (uintptr_t)&ZSTD_freeCCtx },
  { "ZSTD_freeDCtx", (uintptr_t)&ZSTD_freeDCtx },
  { "ZSTD_getErrorCode", (uintptr_t)&ZSTD_getErrorCode },
  { "ZSTD_getErrorString", (uintptr_t)&ZSTD_getErrorString },
  { "ZSTD_getFrameContentSize", (uintptr_t)&ZSTD_getFrameContentSize },
  { "ZSTD_isError", (uintptr_t)&ZSTD_isError },
  { "_ZNKSt12__basic_fileIcE7is_openEv", (uintptr_t)&_ZNKSt12__basic_fileIcE7is_openEv },
  { "_ZNKSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE12find_last_ofEPKcj", (uintptr_t)&_ZNKSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE12find_last_ofEPKcj },
  { "_ZNKSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE4findERKS4_j", (uintptr_t)&_ZNKSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE4findERKS4_j },
  { "_ZNKSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE5rfindEcj", (uintptr_t)&_ZNKSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE5rfindEcj },
  { "_ZNKSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE6substrEjj", (uintptr_t)&_ZNKSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE6substrEjj },
  { "_ZNKSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE7compareEPKc", (uintptr_t)&_ZNKSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE7compareEPKc },
  { "_ZNKSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE7compareERKS4_", (uintptr_t)&_ZNKSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE7compareERKS4_ },
  { "_ZNKSt7__cxx1115basic_stringbufIcSt11char_traitsIcESaIcEE3strEv", (uintptr_t)&_ZNKSt7__cxx1115basic_stringbufIcSt11char_traitsIcESaIcEE3strEv },
  { "_ZNKSt9type_infoeqERKS_", (uintptr_t)&_ZNKSt9type_infoeqERKS_ },
  { "_ZNSo9_M_insertIdEERSoT_", (uintptr_t)&_ZNSo9_M_insertIdEERSoT_ },
  { "_ZNSo9_M_insertImEERSoT_", (uintptr_t)&_ZNSo9_M_insertImEERSoT_ },
  { "_ZNSo9_M_insertIxEERSoT_", (uintptr_t)&_ZNSo9_M_insertIxEERSoT_ },
  { "_ZNSolsEPFRSoS_E", (uintptr_t)&_ZNSolsEPFRSoS_E },
  { "_ZNSolsEf", (uintptr_t)&_ZNSolsEf },
  { "_ZNSolsEi", (uintptr_t)&_ZNSolsEi },
  { "_ZNSt13basic_filebufIcSt11char_traitsIcEEC1Ev", (uintptr_t)&_ZNSt13basic_filebufIcSt11char_traitsIcEEC1Ev },
  { "_ZNSt14basic_ofstreamIcSt11char_traitsIcEE4openEPKcSt13_Ios_Openmode", (uintptr_t)&_ZNSt14basic_ofstreamIcSt11char_traitsIcEE4openEPKcSt13_Ios_Openmode },
  { "_ZNSt14basic_ofstreamIcSt11char_traitsIcEE5closeEv", (uintptr_t)&_ZNSt14basic_ofstreamIcSt11char_traitsIcEE5closeEv },
  { "_ZNSt14basic_ofstreamIcSt11char_traitsIcEED1Ev", (uintptr_t)&_ZNSt14basic_ofstreamIcSt11char_traitsIcEED1Ev },
  { "_ZNSt18condition_variable10notify_allEv", (uintptr_t)&_ZNSt18condition_variable10notify_allEv },
  { "_ZNSt18condition_variable10notify_oneEv", (uintptr_t)&_ZNSt18condition_variable10notify_oneEv },
  { "_ZNSt18condition_variable4waitERSt11unique_lockISt5mutexE", (uintptr_t)&_ZNSt18condition_variable4waitERSt11unique_lockISt5mutexE },
  { "_ZNSt18condition_variableC1Ev", (uintptr_t)&_ZNSt18condition_variableC1Ev },
  { "_ZNSt18condition_variableD1Ev", (uintptr_t)&_ZNSt18condition_variableD1Ev },
  { "_ZNSt6localeC1Ev", (uintptr_t)&_ZNSt6localeC1Ev },
  { "_ZNSt6thread15_M_start_threadESt10unique_ptrINS_6_StateESt14default_deleteIS1_EEPFvvE", (uintptr_t)&_ZNSt6thread15_M_start_threadESt10unique_ptrINS_6_StateESt14default_deleteIS1_EEPFvvE },
  { "_ZNSt6thread20hardware_concurrencyEv", (uintptr_t)&_ZNSt6thread20hardware_concurrencyEv },
  { "_ZNSt6thread4joinEv", (uintptr_t)&_ZNSt6thread4joinEv },
  { "_ZNSt6thread6_StateD2Ev", (uintptr_t)&_ZNSt6thread6_StateD2Ev },
  { "_ZNSt6thread6detachEv", (uintptr_t)&_ZNSt6thread6detachEv },
  { "_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv", (uintptr_t)&_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv },
  { "_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_replaceEjjPKcj", (uintptr_t)&_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_replaceEjjPKcj },
  { "_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE6appendEPKc", (uintptr_t)&_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE6appendEPKc },
  { "_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE6appendEPKcj", (uintptr_t)&_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE6appendEPKcj },
  { "_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE6appendERKS4_", (uintptr_t)&_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE6appendERKS4_ },
  { "_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE6insertEjPKc", (uintptr_t)&_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE6insertEjPKc },
  { "_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE6insertEjRKS4_", (uintptr_t)&_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE6insertEjRKS4_ },
  { "_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE6resizeEj", (uintptr_t)&_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE6resizeEj },
  { "_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE7_S_copyEPcPKcj", (uintptr_t)&_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE7_S_copyEPcPKcj },
  { "_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE7reserveEj", (uintptr_t)&_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE7reserveEj },
  { "_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE8pop_backEv", (uintptr_t)&_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE8pop_backEv },
  { "_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE9_M_appendEPKcj", (uintptr_t)&_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE9_M_appendEPKcj },
  { "_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE9_M_assignERKS4_", (uintptr_t)&_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE9_M_assignERKS4_ },
  { "_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE9_M_createERjj", (uintptr_t)&_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE9_M_createERjj },
  { "_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE9push_backEc", (uintptr_t)&_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE9push_backEc },
  { "_ZNSt7__cxx1115basic_stringbufIcSt11char_traitsIcESaIcEE3strERKNS_12basic_stringIcS2_S3_EE", (uintptr_t)&_ZNSt7__cxx1115basic_stringbufIcSt11char_traitsIcESaIcEE3strERKNS_12basic_stringIcS2_S3_EE },
  { "_ZNSt7__cxx1118basic_stringstreamIcSt11char_traitsIcESaIcEED1Ev", (uintptr_t)&_ZNSt7__cxx1118basic_stringstreamIcSt11char_traitsIcESaIcEED1Ev },
  { "_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEED1Ev", (uintptr_t)&_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEED1Ev },
  { "_ZNSt8__detail15_List_node_base11_M_transferEPS0_S1_", (uintptr_t)&_ZNSt8__detail15_List_node_base11_M_transferEPS0_S1_ },
  { "_ZNSt8__detail15_List_node_base7_M_hookEPS0_", (uintptr_t)&_ZNSt8__detail15_List_node_base7_M_hookEPS0_ },
  { "_ZNSt8__detail15_List_node_base9_M_unhookEv", (uintptr_t)&_ZNSt8__detail15_List_node_base9_M_unhookEv },
  { "_ZNSt8ios_baseC2Ev", (uintptr_t)&_ZNSt8ios_baseC2Ev },
  { "_ZNSt9basic_iosIcSt11char_traitsIcEE4initEPSt15basic_streambufIcS1_E", (uintptr_t)&_ZNSt9basic_iosIcSt11char_traitsIcEE4initEPSt15basic_streambufIcS1_E },
  { "_ZSt14__once_functor", (uintptr_t)&_ZSt14__once_functor },
  { "_ZSt16__get_once_mutexv", (uintptr_t)&_ZSt16__get_once_mutexv },
  { "_ZSt17__throw_bad_allocv", (uintptr_t)&_ZSt17__throw_bad_allocv },
  { "_ZSt18_Rb_tree_decrementPSt18_Rb_tree_node_base", (uintptr_t)&_ZSt18_Rb_tree_decrementPSt18_Rb_tree_node_base },
  { "_ZSt19__throw_logic_errorPKc", (uintptr_t)&_ZSt19__throw_logic_errorPKc },
  { "_ZSt20__throw_length_errorPKc", (uintptr_t)&_ZSt20__throw_length_errorPKc },
  { "_ZSt20__throw_system_errori", (uintptr_t)&_ZSt20__throw_system_errori },
  { "_ZSt24__throw_out_of_range_fmtPKcz", (uintptr_t)&_ZSt24__throw_out_of_range_fmtPKcz },
  { "_ZSt25__throw_bad_function_callv", (uintptr_t)&_ZSt25__throw_bad_function_callv },
  { "_ZSt27__set_once_functor_lock_ptrPSt11unique_lockISt5mutexE", (uintptr_t)&_ZSt27__set_once_functor_lock_ptrPSt11unique_lockISt5mutexE },
  { "_ZSt28__throw_bad_array_new_lengthv", (uintptr_t)&_ZSt28__throw_bad_array_new_lengthv },
  { "_ZSt29_Rb_tree_insert_and_rebalancebPSt18_Rb_tree_node_baseS0_RS_", (uintptr_t)&_ZSt29_Rb_tree_insert_and_rebalancebPSt18_Rb_tree_node_baseS0_RS_ },
  { "_ZSt4endlIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_", (uintptr_t)&_ZSt4endlIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_ },
  { "_ZSt9terminatev", (uintptr_t)&_ZSt9terminatev },
  { "_ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc", (uintptr_t)&_ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc },
  { "_ZStlsIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_St8_SetfillIS3_E", (uintptr_t)&_ZStlsIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_St8_SetfillIS3_E },
  { "_ZStlsIcSt11char_traitsIcESaIcEERSt13basic_ostreamIT_T0_ES7_RKNSt7__cxx1112basic_stringIS4_S5_T1_EE", (uintptr_t)&_ZStlsIcSt11char_traitsIcESaIcEERSt13basic_ostreamIT_T0_ES7_RKNSt7__cxx1112basic_stringIS4_S5_T1_EE },
  { "_ZTHN12MessageQueue16thread_singletonE", (uintptr_t)&_ZTHN12MessageQueue16thread_singletonE },
  { "_ZTHN14ResourceLoader12load_nestingE", (uintptr_t)&_ZTHN14ResourceLoader12load_nestingE },
  { "_ZTHN16GDScriptLanguage11_call_stackE", (uintptr_t)&_ZTHN16GDScriptLanguage11_call_stackE },
  { "_ZTHN16GDScriptLanguage16_call_stack_sizeE", (uintptr_t)&_ZTHN16GDScriptLanguage16_call_stack_sizeE },
  { "_ZTHN16GDScriptLanguage21_debug_parse_err_lineE", (uintptr_t)&_ZTHN16GDScriptLanguage21_debug_parse_err_lineE },
  { "_ZTHN4Node28current_process_thread_groupE", (uintptr_t)&_ZTHN4Node28current_process_thread_groupE },
  { "_ZTINSt6thread6_StateE", (uintptr_t)&_ZTINSt6thread6_StateE },
  { "_ZTTNSt7__cxx1118basic_stringstreamIcSt11char_traitsIcESaIcEEE", (uintptr_t)&_ZTTNSt7__cxx1118basic_stringstreamIcSt11char_traitsIcESaIcEEE },
  { "_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE", (uintptr_t)&_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE },
  { "_ZTTSt14basic_ofstreamIcSt11char_traitsIcEE", (uintptr_t)&_ZTTSt14basic_ofstreamIcSt11char_traitsIcEE },
  { "_ZTVN10__cxxabiv117__class_type_infoE", (uintptr_t)&_ZTVN10__cxxabiv117__class_type_infoE },
  { "_ZTVN10__cxxabiv120__si_class_type_infoE", (uintptr_t)&_ZTVN10__cxxabiv120__si_class_type_infoE },
  { "_ZTVN10__cxxabiv121__vmi_class_type_infoE", (uintptr_t)&_ZTVN10__cxxabiv121__vmi_class_type_infoE },
  { "_ZTVNSt7__cxx1115basic_stringbufIcSt11char_traitsIcESaIcEEE", (uintptr_t)&_ZTVNSt7__cxx1115basic_stringbufIcSt11char_traitsIcESaIcEEE },
  { "_ZTVNSt7__cxx1118basic_stringstreamIcSt11char_traitsIcESaIcEEE", (uintptr_t)&_ZTVNSt7__cxx1118basic_stringstreamIcSt11char_traitsIcESaIcEEE },
  { "_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE", (uintptr_t)&_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE },
  { "_ZTVSt14basic_ofstreamIcSt11char_traitsIcEE", (uintptr_t)&_ZTVSt14basic_ofstreamIcSt11char_traitsIcEE },
  { "_ZTVSt15basic_streambufIcSt11char_traitsIcEE", (uintptr_t)&_ZTVSt15basic_streambufIcSt11char_traitsIcEE },
  { "_ZTVSt9basic_iosIcSt11char_traitsIcEE", (uintptr_t)&_ZTVSt9basic_iosIcSt11char_traitsIcEE },
  { "_ZdaPv", (uintptr_t)&_ZdaPv },
  { "_ZdaPvj", (uintptr_t)&_ZdaPvj },
  { "_ZdlPv", (uintptr_t)&_ZdlPv },
  { "_ZdlPvj", (uintptr_t)&_ZdlPvj },
  { "_Znaj", (uintptr_t)&_Znaj },
  { "_Znwj", (uintptr_t)&_Znwj },
  { "_ZnwjRKSt9nothrow_t", (uintptr_t)&_ZnwjRKSt9nothrow_t },
  { "__aeabi_atexit", (uintptr_t)&__aeabi_atexit },
  { "__aeabi_d2lz", (uintptr_t)&__aeabi_d2lz },
  { "__aeabi_d2ulz", (uintptr_t)&__aeabi_d2ulz },
  { "__aeabi_f2lz", (uintptr_t)&__aeabi_f2lz },
  { "__aeabi_f2ulz", (uintptr_t)&__aeabi_f2ulz },
  { "__aeabi_idiv", (uintptr_t)&__aeabi_idiv },
  { "__aeabi_idivmod", (uintptr_t)&__aeabi_idivmod },
  { "__aeabi_l2d", (uintptr_t)&__aeabi_l2d },
  { "__aeabi_l2f", (uintptr_t)&__aeabi_l2f },
  { "__aeabi_ldivmod", (uintptr_t)&__aeabi_ldivmod },
  { "__aeabi_uidiv", (uintptr_t)&__aeabi_uidiv },
  { "__aeabi_uidivmod", (uintptr_t)&__aeabi_uidivmod },
  { "__aeabi_ul2d", (uintptr_t)&__aeabi_ul2d },
  { "__aeabi_ul2f", (uintptr_t)&__aeabi_ul2f },
  { "__aeabi_uldivmod", (uintptr_t)&__aeabi_uldivmod },
  { "__ctzdi2", (uintptr_t)&__ctzdi2 },
  { "__cxa_guard_acquire", (uintptr_t)&__cxa_guard_acquire },
  { "__cxa_guard_release", (uintptr_t)&__cxa_guard_release },
  { "__cxa_pure_virtual", (uintptr_t)&__cxa_pure_virtual },
  { "__cxa_thread_atexit", (uintptr_t)&__cxa_thread_atexit },
  { "__dynamic_cast", (uintptr_t)&__dynamic_cast },
  { "__emutls_get_address", (uintptr_t)&__emutls_get_address },
  { "__errno", (uintptr_t)&__errno },
  { "__once_proxy", (uintptr_t)&__once_proxy },
  { "__popcountdi2", (uintptr_t)&__popcountdi2 },
  { "__popcountsi2", (uintptr_t)&__popcountsi2 },
  { "_ctype_", (uintptr_t)&_ctype_ },
  { "_impure_ptr", (uintptr_t)&_impure_ptr },
  { "_sceAppMgrGetBudgetInfo", (uintptr_t)&_sceAppMgrGetBudgetInfo },
  { "_timezone", (uintptr_t)&_timezone },
  { "_tzname", (uintptr_t)&_tzname },
  { "abort", (uintptr_t)&abort },
  { "accept", (uintptr_t)&accept },
  { "access", (uintptr_t)&access },
  { "acos", (uintptr_t)&acos },
  { "acosf", (uintptr_t)&acosf },
  { "acosh", (uintptr_t)&acosh },
  { "asin", (uintptr_t)&asin },
  { "asinf", (uintptr_t)&asinf },
  { "asinh", (uintptr_t)&asinh },
  { "atan", (uintptr_t)&atan },
  { "atan2", (uintptr_t)&atan2 },
  { "atan2f", (uintptr_t)&atan2f },
  { "atanf", (uintptr_t)&atanf },
  { "atanh", (uintptr_t)&atanh },
  { "atexit", (uintptr_t)&atexit },
  { "atof", (uintptr_t)&atof },
  { "atoi", (uintptr_t)&atoi },
  { "bind", (uintptr_t)&bind },
  { "calloc", (uintptr_t)&calloc },
  { "cbrtf", (uintptr_t)&cbrtf },
  { "ceil", (uintptr_t)&ceil },
  { "ceilf", (uintptr_t)&ceilf },
  { "chdir", (uintptr_t)&chdir },
  { "chmod", (uintptr_t)&chmod },
  { "clock", (uintptr_t)&clock },
  { "clock_gettime", (uintptr_t)&clock_gettime },
  { "close", (uintptr_t)&close },
  { "closedir", (uintptr_t)&closedir },
  { "compress", (uintptr_t)&compress },
  { "compressBound", (uintptr_t)&compressBound },
  { "connect", (uintptr_t)&connect },
  { "cos", (uintptr_t)&cos },
  { "cosf", (uintptr_t)&cosf },
  { "cosh", (uintptr_t)&cosh },
  { "crc32", (uintptr_t)&crc32 },
  { "deflate", (uintptr_t)&deflate },
  { "deflateBound", (uintptr_t)&deflateBound },
  { "deflateEnd", (uintptr_t)&deflateEnd },
  { "deflateInit2_", (uintptr_t)&deflateInit2_ },
  { "dlclose", (uintptr_t)&dlclose },
  { "dlerror", (uintptr_t)&dlerror },
  { "dlopen", (uintptr_t)&dlopen },
  { "dlsym", (uintptr_t)&dlsym },
  { "exit", (uintptr_t)&exit },
  { "exp", (uintptr_t)&exp },
  { "exp2", (uintptr_t)&exp2 },
  { "exp2f", (uintptr_t)&exp2f },
  { "expf", (uintptr_t)&expf },
  { "fchmod", (uintptr_t)&fchmod },
  { "fclose", (uintptr_t)&fclose },
  { "fcntl", (uintptr_t)&fcntl },
  { "fdopen", (uintptr_t)&fdopen },
  { "feof", (uintptr_t)&feof },
  { "ferror", (uintptr_t)&ferror },
  { "fflush", (uintptr_t)&fflush },
  { "fgetpos", (uintptr_t)&fgetpos },
  { "fgets", (uintptr_t)&fgets },
  { "fileno", (uintptr_t)&fileno },
  { "floor", (uintptr_t)&floor },
  { "floorf", (uintptr_t)&floorf },
  { "fmax", (uintptr_t)&fmax },
  { "fmaxf", (uintptr_t)&fmaxf },
  { "fmin", (uintptr_t)&fmin },
  { "fmod", (uintptr_t)&fmod },
  { "fmodf", (uintptr_t)&fmodf },
  { "fopen", (uintptr_t)&fopen },
  { "fork", (uintptr_t)&fork },
  { "fprintf", (uintptr_t)&fprintf },
  { "fputc", (uintptr_t)&fputc },
  { "fputs", (uintptr_t)&fputs },
  { "fread", (uintptr_t)&fread },
  { "free", (uintptr_t)&free },
  { "freeaddrinfo", (uintptr_t)&freeaddrinfo },
  { "freopen", (uintptr_t)&freopen },
  { "fscanf", (uintptr_t)&fscanf },
  { "fseek", (uintptr_t)&fseek },
  { "fseeko", (uintptr_t)&fseeko },
  { "fsetpos", (uintptr_t)&fsetpos },
  { "fstat", (uintptr_t)&fstat },
  { "ftell", (uintptr_t)&ftell },
  { "ftello", (uintptr_t)&ftello },
  { "ftruncate", (uintptr_t)&ftruncate },
  { "fwrite", (uintptr_t)&fwrite },
  { "get_crc_table", (uintptr_t)&get_crc_table },
  { "getaddrinfo", (uintptr_t)&getaddrinfo },
  { "getc", (uintptr_t)&getc },
  { "getcwd", (uintptr_t)&getcwd },
  { "getenv", (uintptr_t)&getenv },
  { "getpid", (uintptr_t)&getpid },
  { "getsockname", (uintptr_t)&getsockname },
  { "gettimeofday", (uintptr_t)&gettimeofday },
  { "glActiveTexture", (uintptr_t)&glActiveTexture },
  { "glAttachShader", (uintptr_t)&glAttachShader },
  { "glBindBuffer", (uintptr_t)&glBindBuffer },
  { "glBindBufferBase", (uintptr_t)&glBindBufferBase },
  { "glBindBufferRange", (uintptr_t)&glBindBufferRange },
  { "glBindFramebuffer", (uintptr_t)&glBindFramebuffer },
  { "glBindRenderbuffer", (uintptr_t)&glBindRenderbuffer },
  { "glBindTexture", (uintptr_t)&glBindTexture },
  { "glBindVertexArray", (uintptr_t)&glBindVertexArray },
  { "glBlendEquation", (uintptr_t)&glBlendEquation },
  { "glBlendFunc", (uintptr_t)&glBlendFunc },
  { "glBlendFuncSeparate", (uintptr_t)&glBlendFuncSeparate },
  { "glBlitFramebuffer", (uintptr_t)&glBlitFramebuffer },
  { "glBufferData", (uintptr_t)&glBufferData },
  { "glBufferSubData", (uintptr_t)&glBufferSubData },
  { "glCheckFramebufferStatus", (uintptr_t)&glCheckFramebufferStatus },
  { "glClear", (uintptr_t)&glClear },
  { "glClearColor", (uintptr_t)&glClearColor },
  { "glClearDepthf", (uintptr_t)&glClearDepthf },
  { "glClearStencil", (uintptr_t)&glClearStencil },
  { "glColorMask", (uintptr_t)&glColorMask },
  { "glCompileShader", (uintptr_t)&glCompileShader },
  { "glCompressedTexImage2D", (uintptr_t)&glCompressedTexImage2D },
  { "glCreateProgram", (uintptr_t)&glCreateProgram },
  { "glCreateShader", (uintptr_t)&glCreateShader },
  { "glCullFace", (uintptr_t)&glCullFace },
  { "glDeleteBuffers", (uintptr_t)&glDeleteBuffers },
  { "glDeleteFramebuffers", (uintptr_t)&glDeleteFramebuffers },
  { "glDeleteProgram", (uintptr_t)&glDeleteProgram },
  { "glDeleteQueries", (uintptr_t)&glDeleteQueries },
  { "glDeleteRenderbuffers", (uintptr_t)&glDeleteRenderbuffers },
  { "glDeleteShader", (uintptr_t)&glDeleteShader },
  { "glDeleteTextures", (uintptr_t)&glDeleteTextures },
  { "glDeleteVertexArrays", (uintptr_t)&glDeleteVertexArrays },
  { "glDepthFunc", (uintptr_t)&glDepthFunc },
  { "glDepthMask", (uintptr_t)&glDepthMask },
  { "glDisable", (uintptr_t)&glDisable },
  { "glDisableVertexAttribArray", (uintptr_t)&glDisableVertexAttribArray },
  { "glDrawArrays", (uintptr_t)&glDrawArrays },
  { "glDrawArraysInstanced", (uintptr_t)&glDrawArraysInstanced },
  { "glDrawElements", (uintptr_t)&glDrawElements },
  { "glDrawElementsInstanced", (uintptr_t)&glDrawElementsInstanced },
  { "glEnable", (uintptr_t)&glEnable },
  { "glEnableVertexAttribArray", (uintptr_t)&glEnableVertexAttribArray },
  { "glFinish", (uintptr_t)&glFinish },
  { "glFramebufferRenderbuffer", (uintptr_t)&glFramebufferRenderbuffer },
  { "glFramebufferTexture2D", (uintptr_t)&glFramebufferTexture2D },
  { "glFrontFace", (uintptr_t)&glFrontFace },
  { "glGenBuffers", (uintptr_t)&glGenBuffers },
  { "glGenFramebuffers", (uintptr_t)&glGenFramebuffers },
  { "glGenQueries", (uintptr_t)&glGenQueries },
  { "glGenRenderbuffers", (uintptr_t)&glGenRenderbuffers },
  { "glGenTextures", (uintptr_t)&glGenTextures },
  { "glGenVertexArrays", (uintptr_t)&glGenVertexArrays },
  { "glGenerateMipmap", (uintptr_t)&glGenerateMipmap },
  { "glGetFloatv", (uintptr_t)&glGetFloatv },
  { "glGetIntegerv", (uintptr_t)&glGetIntegerv },
  { "glGetProgramBinary", (uintptr_t)&glGetProgramBinary },
  { "glGetProgramInfoLog", (uintptr_t)&glGetProgramInfoLog },
  { "glGetProgramiv", (uintptr_t)&glGetProgramiv },
  { "glGetShaderInfoLog", (uintptr_t)&glGetShaderInfoLog },
  { "glGetShaderiv", (uintptr_t)&glGetShaderiv },
  { "glGetString", (uintptr_t)&glGetString },
  { "glGetStringi", (uintptr_t)&glGetStringi },
  { "glGetUniformBlockIndex", (uintptr_t)&glGetUniformBlockIndex },
  { "glGetUniformLocation", (uintptr_t)&glGetUniformLocation },
  { "glLinkProgram", (uintptr_t)&glLinkProgram },
  { "glMapBufferRange", (uintptr_t)&glMapBufferRange },
  { "glPixelStorei", (uintptr_t)&glPixelStorei },
  { "glProgramBinary", (uintptr_t)&glProgramBinary },
  { "glReadBuffer", (uintptr_t)&glReadBuffer },
  { "glReadPixels", (uintptr_t)&glReadPixels },
  { "glRenderbufferStorage", (uintptr_t)&glRenderbufferStorage },
  { "glScissor", (uintptr_t)&glScissor },
  { "glShaderSource", (uintptr_t)&glShaderSource },
  { "glStencilFunc", (uintptr_t)&glStencilFunc },
  { "glStencilMask", (uintptr_t)&glStencilMask },
  { "glStencilOp", (uintptr_t)&glStencilOp },
  { "glTexImage2D", (uintptr_t)&glTexImage2D },
  { "glTexParameterf", (uintptr_t)&glTexParameterf },
  { "glTexParameteri", (uintptr_t)&glTexParameteri },
  { "glUniform1f", (uintptr_t)&glUniform1f },
  { "glUniform1i", (uintptr_t)&glUniform1i },
  { "glUniform1iv", (uintptr_t)&glUniform1iv },
  { "glUniform2f", (uintptr_t)&glUniform2f },
  { "glUniform2fv", (uintptr_t)&glUniform2fv },
  { "glUniform2iv", (uintptr_t)&glUniform2iv },
  { "glUniform3fv", (uintptr_t)&glUniform3fv },
  { "glUniform4f", (uintptr_t)&glUniform4f },
  { "glUniform4fv", (uintptr_t)&glUniform4fv },
  { "glUniformBlockBinding", (uintptr_t)&glUniformBlockBinding },
  { "glUniformMatrix3fv", (uintptr_t)&glUniformMatrix3fv },
  { "glUniformMatrix4fv", (uintptr_t)&glUniformMatrix4fv },
  { "glUnmapBuffer", (uintptr_t)&glUnmapBuffer },
  { "glUseProgram", (uintptr_t)&glUseProgram },
  { "glVertexAttrib4f", (uintptr_t)&glVertexAttrib4f },
  { "glVertexAttribDivisor", (uintptr_t)&glVertexAttribDivisor },
  { "glVertexAttribPointer", (uintptr_t)&glVertexAttribPointer },
  { "glViewport", (uintptr_t)&glViewport },
  { "gmtime_r", (uintptr_t)&gmtime_r },
  { "hypotf", (uintptr_t)&hypotf },
  { "ilogb", (uintptr_t)&ilogb },
  { "inflate", (uintptr_t)&inflate },
  { "inflateEnd", (uintptr_t)&inflateEnd },
  { "inflateInit2_", (uintptr_t)&inflateInit2_ },
  { "inflateReset", (uintptr_t)&inflateReset },
  { "isalpha", (uintptr_t)&isalpha },
  { "isatty", (uintptr_t)&isatty },
  { "isspace", (uintptr_t)&isspace },
  { "isxdigit", (uintptr_t)&isxdigit },
  { "kill", (uintptr_t)&kill },
  { "ldexp", (uintptr_t)&ldexp },
  { "ldexpf", (uintptr_t)&ldexpf },
  { "listen", (uintptr_t)&listen },
  { "localtime_r", (uintptr_t)&localtime_r },
  { "log", (uintptr_t)&log },
  { "log10", (uintptr_t)&log10 },
  { "log10f", (uintptr_t)&log10f },
  { "log2", (uintptr_t)&log2 },
  { "log2f", (uintptr_t)&log2f },
  { "logf", (uintptr_t)&logf },
  { "longjmp", (uintptr_t)&longjmp },
  { "lrintf", (uintptr_t)&lrintf },
  { "lstat", (uintptr_t)&lstat },
  { "malloc", (uintptr_t)&malloc },
  { "memalign", (uintptr_t)&memalign },
  { "memchr", (uintptr_t)&memchr },
  { "memcmp", (uintptr_t)&memcmp },
  { "memcpy", (uintptr_t)&memcpy },
  { "memmove", (uintptr_t)&memmove },
  { "memset", (uintptr_t)&memset },
  { "mkdir", (uintptr_t)&mkdir },
  { "mkstemp", (uintptr_t)&mkstemp },
  { "modf", (uintptr_t)&modf },
  { "nanosleep", (uintptr_t)&nanosleep },
  { "nearbyint", (uintptr_t)&nearbyint },
  { "nearbyintf", (uintptr_t)&nearbyintf },
  { "nextafter", (uintptr_t)&nextafter },
  { "nl_langinfo", (uintptr_t)&nl_langinfo },
  { "ogg_page_bos", (uintptr_t)&ogg_page_bos },
  { "ogg_page_granulepos", (uintptr_t)&ogg_page_granulepos },
  { "ogg_page_serialno", (uintptr_t)&ogg_page_serialno },
  { "ogg_stream_check", (uintptr_t)&ogg_stream_check },
  { "ogg_stream_clear", (uintptr_t)&ogg_stream_clear },
  { "ogg_stream_init", (uintptr_t)&ogg_stream_init },
  { "ogg_stream_packetout", (uintptr_t)&ogg_stream_packetout },
  { "ogg_stream_pagein", (uintptr_t)&ogg_stream_pagein },
  { "ogg_stream_reset", (uintptr_t)&ogg_stream_reset },
  { "ogg_sync_buffer", (uintptr_t)&ogg_sync_buffer },
  { "ogg_sync_check", (uintptr_t)&ogg_sync_check },
  { "ogg_sync_clear", (uintptr_t)&ogg_sync_clear },
  { "ogg_sync_init", (uintptr_t)&ogg_sync_init },
  { "ogg_sync_pageout", (uintptr_t)&ogg_sync_pageout },
  { "ogg_sync_reset", (uintptr_t)&ogg_sync_reset },
  { "ogg_sync_wrote", (uintptr_t)&ogg_sync_wrote },
  { "open", (uintptr_t)&open },
  { "opendir", (uintptr_t)&opendir },
  { "pcre2_code_free_32", (uintptr_t)&pcre2_code_free_32 },
  { "pcre2_compile_32", (uintptr_t)&pcre2_compile_32 },
  { "pcre2_compile_context_create_32", (uintptr_t)&pcre2_compile_context_create_32 },
  { "pcre2_compile_context_free_32", (uintptr_t)&pcre2_compile_context_free_32 },
  { "pcre2_general_context_create_32", (uintptr_t)&pcre2_general_context_create_32 },
  { "pcre2_general_context_free_32", (uintptr_t)&pcre2_general_context_free_32 },
  { "pcre2_get_error_message_32", (uintptr_t)&pcre2_get_error_message_32 },
  { "pcre2_get_ovector_count_32", (uintptr_t)&pcre2_get_ovector_count_32 },
  { "pcre2_get_ovector_pointer_32", (uintptr_t)&pcre2_get_ovector_pointer_32 },
  { "pcre2_match_32", (uintptr_t)&pcre2_match_32 },
  { "pcre2_match_context_create_32", (uintptr_t)&pcre2_match_context_create_32 },
  { "pcre2_match_context_free_32", (uintptr_t)&pcre2_match_context_free_32 },
  { "pcre2_match_data_create_from_pattern_32", (uintptr_t)&pcre2_match_data_create_from_pattern_32 },
  { "pcre2_match_data_free_32", (uintptr_t)&pcre2_match_data_free_32 },
  { "pcre2_pattern_info_32", (uintptr_t)&pcre2_pattern_info_32 },
  { "pcre2_substitute_32", (uintptr_t)&pcre2_substitute_32 },
  { "pipe", (uintptr_t)&pipe },
  { "png_create_info_struct", (uintptr_t)&png_create_info_struct },
  { "png_create_read_struct", (uintptr_t)&png_create_read_struct },
  { "png_create_write_struct", (uintptr_t)&png_create_write_struct },
  { "png_destroy_read_struct", (uintptr_t)&png_destroy_read_struct },
  { "png_destroy_write_struct", (uintptr_t)&png_destroy_write_struct },
  { "png_error", (uintptr_t)&png_error },
  { "png_get_IHDR", (uintptr_t)&png_get_IHDR },
  { "png_get_bit_depth", (uintptr_t)&png_get_bit_depth },
  { "png_get_channels", (uintptr_t)&png_get_channels },
  { "png_get_io_ptr", (uintptr_t)&png_get_io_ptr },
  { "png_get_rowbytes", (uintptr_t)&png_get_rowbytes },
  { "png_get_valid", (uintptr_t)&png_get_valid },
  { "png_image_begin_read_from_file", (uintptr_t)&png_image_begin_read_from_file },
  { "png_image_begin_read_from_memory", (uintptr_t)&png_image_begin_read_from_memory },
  { "png_image_finish_read", (uintptr_t)&png_image_finish_read },
  { "png_image_free", (uintptr_t)&png_image_free },
  { "png_image_write_to_memory", (uintptr_t)&png_image_write_to_memory },
  { "png_read_end", (uintptr_t)&png_read_end },
  { "png_read_image", (uintptr_t)&png_read_image },
  { "png_read_info", (uintptr_t)&png_read_info },
  { "png_read_update_info", (uintptr_t)&png_read_update_info },
  { "png_set_IHDR", (uintptr_t)&png_set_IHDR },
  { "png_set_add_alpha", (uintptr_t)&png_set_add_alpha },
  { "png_set_expand_gray_1_2_4_to_8", (uintptr_t)&png_set_expand_gray_1_2_4_to_8 },
  { "png_set_gray_to_rgb", (uintptr_t)&png_set_gray_to_rgb },
  { "png_set_interlace_handling", (uintptr_t)&png_set_interlace_handling },
  { "png_set_longjmp_fn", (uintptr_t)&png_set_longjmp_fn },
  { "png_set_palette_to_rgb", (uintptr_t)&png_set_palette_to_rgb },
  { "png_set_read_fn", (uintptr_t)&png_set_read_fn },
  { "png_set_strip_16", (uintptr_t)&png_set_strip_16 },
  { "png_set_tRNS_to_alpha", (uintptr_t)&png_set_tRNS_to_alpha },
  { "png_set_write_fn", (uintptr_t)&png_set_write_fn },
  { "png_write_end", (uintptr_t)&png_write_end },
  { "png_write_image", (uintptr_t)&png_write_image },
  { "png_write_info", (uintptr_t)&png_write_info },
  { "poll", (uintptr_t)&poll },
  { "pow", (uintptr_t)&pow },
  { "powf", (uintptr_t)&powf },
  { "printf", (uintptr_t)&printf },
  { "pthread_cancel", (uintptr_t)&pthread_cancel },
  { "pthread_create", (uintptr_t)&pthread_create },
  { "pthread_join", (uintptr_t)&pthread_join },
  { "pthread_mutex_destroy", (uintptr_t)&pthread_mutex_destroy },
  { "pthread_mutex_init", (uintptr_t)&pthread_mutex_init },
  { "pthread_mutex_lock", (uintptr_t)&pthread_mutex_lock },
  { "pthread_mutex_trylock", (uintptr_t)&pthread_mutex_trylock },
  { "pthread_mutex_unlock", (uintptr_t)&pthread_mutex_unlock },
  { "pthread_once", (uintptr_t)&pthread_once },
  { "pthread_rwlock_rdlock", (uintptr_t)&pthread_rwlock_rdlock },
  { "pthread_rwlock_unlock", (uintptr_t)&pthread_rwlock_unlock },
  { "pthread_rwlock_wrlock", (uintptr_t)&pthread_rwlock_wrlock },
  { "pthread_self", (uintptr_t)&pthread_self },
  { "qsort", (uintptr_t)&qsort },
  { "raise", (uintptr_t)&raise },
  { "rand", (uintptr_t)&rand },
  { "read", (uintptr_t)&read },
  { "readdir", (uintptr_t)&readdir },
  { "realloc", (uintptr_t)&realloc },
  { "recv", (uintptr_t)&recv },
  { "recvfrom", (uintptr_t)&recvfrom },
  { "remainder", (uintptr_t)&remainder },
  { "rename", (uintptr_t)&rename },
  { "rewind", (uintptr_t)&rewind },
  { "rint", (uintptr_t)&rint },
  { "rintf", (uintptr_t)&rintf },
  { "rmdir", (uintptr_t)&rmdir },
  { "round", (uintptr_t)&round },
  { "roundf", (uintptr_t)&roundf },
  { "sceAudioOutOpenPort", (uintptr_t)&sceAudioOutOpenPort },
  { "sceAudioOutOutput", (uintptr_t)&sceAudioOutOutput },
  { "sceAudioOutReleasePort", (uintptr_t)&sceAudioOutReleasePort },
  { "sceClibPrintf", (uintptr_t)&sceClibPrintf },
  { "sceCtrlPeekBufferPositive", (uintptr_t)&sceCtrlPeekBufferPositive },
  { "sceCtrlSetSamplingMode", (uintptr_t)&sceCtrlSetSamplingMode },
  { "sceIoClose", (uintptr_t)&sceIoClose },
  { "sceIoOpen", (uintptr_t)&sceIoOpen },
  { "sceIoRemove", (uintptr_t)&sceIoRemove },
  { "sceIoWrite", (uintptr_t)&sceIoWrite },
  { "sceKernelExitProcess", (uintptr_t)&sceKernelExitProcess },
  { "sceKernelGetRandomNumber", (uintptr_t)&sceKernelGetRandomNumber },
  { "sceMotionGetState", (uintptr_t)&sceMotionGetState },
  { "sceMotionStartSampling", (uintptr_t)&sceMotionStartSampling },
  { "scePowerSetArmClockFrequency", (uintptr_t)&scePowerSetArmClockFrequency },
  { "scePowerSetBusClockFrequency", (uintptr_t)&scePowerSetBusClockFrequency },
  { "scePowerSetGpuClockFrequency", (uintptr_t)&scePowerSetGpuClockFrequency },
  { "scePowerSetGpuXbarClockFrequency", (uintptr_t)&scePowerSetGpuXbarClockFrequency },
  { "sceTouchGetPanelInfo", (uintptr_t)&sceTouchGetPanelInfo },
  { "sceTouchPeek", (uintptr_t)&sceTouchPeek },
  { "sceTouchSetSamplingState", (uintptr_t)&sceTouchSetSamplingState },
  { "send", (uintptr_t)&send },
  { "sendto", (uintptr_t)&sendto },
  { "setenv", (uintptr_t)&setenv },
  { "setjmp", (uintptr_t)&setjmp },
  { "setlocale", (uintptr_t)&setlocale },
  { "setsid", (uintptr_t)&setsid },
  { "setsockopt", (uintptr_t)&setsockopt },
  { "signal", (uintptr_t)&signal },
  { "sin", (uintptr_t)&sin },
  { "sinf", (uintptr_t)&sinf },
  { "sinh", (uintptr_t)&sinh },
  { "snprintf", (uintptr_t)&snprintf },
  { "socket", (uintptr_t)&socket },
  { "sprintf", (uintptr_t)&sprintf },
  { "sqrt", (uintptr_t)&sqrt },
  { "sqrtf", (uintptr_t)&sqrtf },
  { "srand", (uintptr_t)&srand },
  { "sscanf", (uintptr_t)&sscanf },
  { "stat", (uintptr_t)&stat },
  { "statvfs", (uintptr_t)&statvfs },
  { "strcasecmp", (uintptr_t)&strcasecmp },
  { "strcat", (uintptr_t)&strcat },
  { "strchr", (uintptr_t)&strchr },
  { "strcmp", (uintptr_t)&strcmp },
  { "strcpy", (uintptr_t)&strcpy },
  { "strdup", (uintptr_t)&strdup },
  { "strerror", (uintptr_t)&strerror },
  { "strftime", (uintptr_t)&strftime },
  { "strlen", (uintptr_t)&strlen },
  { "strncat", (uintptr_t)&strncat },
  { "strncmp", (uintptr_t)&strncmp },
  { "strncpy", (uintptr_t)&strncpy },
  { "strnlen", (uintptr_t)&strnlen },
  { "strrchr", (uintptr_t)&strrchr },
  { "strstr", (uintptr_t)&strstr },
  { "strtod", (uintptr_t)&strtod },
  { "strtok_r", (uintptr_t)&strtok_r },
  { "strtol", (uintptr_t)&strtol },
  { "strtoul", (uintptr_t)&strtoul },
  { "tan", (uintptr_t)&tan },
  { "tanf", (uintptr_t)&tanf },
  { "tanh", (uintptr_t)&tanh },
  { "tanhf", (uintptr_t)&tanhf },
  { "th_comment_clear", (uintptr_t)&th_comment_clear },
  { "th_comment_init", (uintptr_t)&th_comment_init },
  { "th_decode_alloc", (uintptr_t)&th_decode_alloc },
  { "th_decode_ctl", (uintptr_t)&th_decode_ctl },
  { "th_decode_free", (uintptr_t)&th_decode_free },
  { "th_decode_headerin", (uintptr_t)&th_decode_headerin },
  { "th_decode_packetin", (uintptr_t)&th_decode_packetin },
  { "th_decode_ycbcr_out", (uintptr_t)&th_decode_ycbcr_out },
  { "th_granule_frame", (uintptr_t)&th_granule_frame },
  { "th_granule_time", (uintptr_t)&th_granule_time },
  { "th_info_clear", (uintptr_t)&th_info_clear },
  { "th_info_init", (uintptr_t)&th_info_init },
  { "th_packet_iskeyframe", (uintptr_t)&th_packet_iskeyframe },
  { "th_setup_free", (uintptr_t)&th_setup_free },
  { "time", (uintptr_t)&time },
  { "tolower", (uintptr_t)&tolower },
  { "tzset", (uintptr_t)&tzset },
  { "uncompress", (uintptr_t)&uncompress },
  { "unlink", (uintptr_t)&unlink },
  { "unsetenv", (uintptr_t)&unsetenv },
  { "vfprintf", (uintptr_t)&vfprintf },
  { "vglInitWithCustomThreshold", (uintptr_t)&vglInitWithCustomThreshold },
  { "vglSetParamBufferSize", (uintptr_t)&vglSetParamBufferSize },
  { "vglSetSemanticBindingMode", (uintptr_t)&vglSetSemanticBindingMode },
  { "vglSwapBuffers", (uintptr_t)&vglSwapBuffers },
  { "vglUseTripleBuffering", (uintptr_t)&vglUseTripleBuffering },
  { "vorbis_block_clear", (uintptr_t)&vorbis_block_clear },
  { "vorbis_block_init", (uintptr_t)&vorbis_block_init },
  { "vorbis_comment_clear", (uintptr_t)&vorbis_comment_clear },
  { "vorbis_comment_init", (uintptr_t)&vorbis_comment_init },
  { "vorbis_dsp_clear", (uintptr_t)&vorbis_dsp_clear },
  { "vorbis_granule_time", (uintptr_t)&vorbis_granule_time },
  { "vorbis_info_clear", (uintptr_t)&vorbis_info_clear },
  { "vorbis_info_init", (uintptr_t)&vorbis_info_init },
  { "vorbis_synthesis", (uintptr_t)&vorbis_synthesis },
  { "vorbis_synthesis_blockin", (uintptr_t)&vorbis_synthesis_blockin },
  { "vorbis_synthesis_headerin", (uintptr_t)&vorbis_synthesis_headerin },
  { "vorbis_synthesis_idheader", (uintptr_t)&vorbis_synthesis_idheader },
  { "vorbis_synthesis_init", (uintptr_t)&vorbis_synthesis_init },
  { "vorbis_synthesis_pcmout", (uintptr_t)&vorbis_synthesis_pcmout },
  { "vorbis_synthesis_read", (uintptr_t)&vorbis_synthesis_read },
  { "vorbis_synthesis_restart", (uintptr_t)&vorbis_synthesis_restart },
  { "vorbis_synthesis_trackonly", (uintptr_t)&vorbis_synthesis_trackonly },
  { "vprintf", (uintptr_t)&vprintf },
  { "vsnprintf", (uintptr_t)&vsnprintf },
  { "wcscmp", (uintptr_t)&wcscmp },
  { "wcscpy", (uintptr_t)&wcscpy },
  { "write", (uintptr_t)&write },
};
int default_dynlib_size = (int)sizeof(default_dynlib);
