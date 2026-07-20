// XTRUETYPE HEADER
// @ OctoDev Software 2025
// NO WARRANTY IMPLIED; USE AT YOUR OWN RISK

#ifndef XI_INCLUDE_XI_IMAGE_H
#define XI_INCLUDE_XI_IMAGE_H

#ifndef XI_NO_STDIO
#include <stdio.h>
#endif // XI_NO_STDIO

#define XI_VERSION 1

enum
{
   XI_default = 0, // only used for desired_channels

   XI_grey       = 1,
   XI_grey_alpha = 2,
   XI_rgb        = 3,
   XI_rgb_alpha  = 4
};

#include <stdlib.h>
typedef unsigned char xi_uc;
typedef unsigned short xi_us;

#ifdef __cplusplus
extern "C" {
#endif

#ifndef XIDEF
#ifdef XI_IMAGE_STATIC
#define XIDEF static
#else
#define XIDEF extern
#endif
#endif

//////////////////////////////////////////////////////////////////////////////
//
// PRIMARY API - works on images of any type
//

//
// load image by filename, open file, or memory buffer
//

typedef struct
{
   int      (*read)  (void *user,char *data,int size);   // fill 'data' with 'size' bytes.  return number of bytes actually read
   void     (*skip)  (void *user,int n);                 // skip the next 'n' bytes, or 'unget' the last -n bytes if negative
   int      (*eof)   (void *user);                       // returns nonzero if we are at end of file/data
} xi_io_callbacks;

////////////////////////////////////
//
// 8-bits-per-channel interface
//

XIDEF xi_uc *xi_load_from_memory   (xi_uc           const *buffer, int len   , int *x, int *y, int *channels_in_file, int desired_channels);
XIDEF xi_uc *xi_load_from_callbacks(xi_io_callbacks const *clbk  , void *user, int *x, int *y, int *channels_in_file, int desired_channels);

#ifndef XI_NO_STDIO
XIDEF xi_uc *xi_load            (char const *filename, int *x, int *y, int *channels_in_file, int desired_channels);
XIDEF xi_uc *xi_load_from_file  (FILE *f, int *x, int *y, int *channels_in_file, int desired_channels);
// for xi_load_from_file, file pointer is left pointing immediately after image
#endif

#ifndef XI_NO_GIF
XIDEF xi_uc *xi_load_gif_from_memory(xi_uc const *buffer, int len, int **delays, int *x, int *y, int *z, int *comp, int req_comp);
#endif

#ifdef XI_WINDOWS_UTF8
XIDEF int xi_convert_wchar_to_utf8(char *buffer, size_t bufferlen, const wchar_t* input);
#endif

////////////////////////////////////
//
// 16-bits-per-channel interface
//

XIDEF xi_us *xi_load_16_from_memory   (xi_uc const *buffer, int len, int *x, int *y, int *channels_in_file, int desired_channels);
XIDEF xi_us *xi_load_16_from_callbacks(xi_io_callbacks const *clbk, void *user, int *x, int *y, int *channels_in_file, int desired_channels);

#ifndef XI_NO_STDIO
XIDEF xi_us *xi_load_16          (char const *filename, int *x, int *y, int *channels_in_file, int desired_channels);
XIDEF xi_us *xi_load_from_file_16(FILE *f, int *x, int *y, int *channels_in_file, int desired_channels);
#endif

////////////////////////////////////
//
// float-per-channel interface
//
#ifndef XI_NO_LINEAR
   XIDEF float *xi_loadf_from_memory     (xi_uc const *buffer, int len, int *x, int *y, int *channels_in_file, int desired_channels);
   XIDEF float *xi_loadf_from_callbacks  (xi_io_callbacks const *clbk, void *user, int *x, int *y,  int *channels_in_file, int desired_channels);

   #ifndef XI_NO_STDIO
   XIDEF float *xi_loadf            (char const *filename, int *x, int *y, int *channels_in_file, int desired_channels);
   XIDEF float *xi_loadf_from_file  (FILE *f, int *x, int *y, int *channels_in_file, int desired_channels);
   #endif
#endif

#ifndef XI_NO_HDR
   XIDEF void   xi_hdr_to_ldr_gamma(float gamma);
   XIDEF void   xi_hdr_to_ldr_scale(float scale);
#endif // XI_NO_HDR

#ifndef XI_NO_LINEAR
   XIDEF void   xi_ldr_to_hdr_gamma(float gamma);
   XIDEF void   xi_ldr_to_hdr_scale(float scale);
#endif // XI_NO_LINEAR

// xi_is_hdr is always defined, but always returns false if XI_NO_HDR
XIDEF int    xi_is_hdr_from_callbacks(xi_io_callbacks const *clbk, void *user);
XIDEF int    xi_is_hdr_from_memory(xi_uc const *buffer, int len);
#ifndef XI_NO_STDIO
XIDEF int      xi_is_hdr          (char const *filename);
XIDEF int      xi_is_hdr_from_file(FILE *f);
#endif // XI_NO_STDIO


// get a VERY brief reason for failure
// on most compilers (and ALL modern mainstream compilers) this is threadsafe
XIDEF const char *xi_failure_reason  (void);

// free the loaded image -- this is just free()
XIDEF void     xi_image_free      (void *retval_from_xi_load);

// get image dimensions & components without fully decoding
XIDEF int      xi_info_from_memory(xi_uc const *buffer, int len, int *x, int *y, int *comp);
XIDEF int      xi_info_from_callbacks(xi_io_callbacks const *clbk, void *user, int *x, int *y, int *comp);
XIDEF int      xi_is_16_bit_from_memory(xi_uc const *buffer, int len);
XIDEF int      xi_is_16_bit_from_callbacks(xi_io_callbacks const *clbk, void *user);

#ifndef XI_NO_STDIO
XIDEF int      xi_info               (char const *filename,     int *x, int *y, int *comp);
XIDEF int      xi_info_from_file     (FILE *f,                  int *x, int *y, int *comp);
XIDEF int      xi_is_16_bit          (char const *filename);
XIDEF int      xi_is_16_bit_from_file(FILE *f);
#endif



// for image formats that explicitly notate that they have premultiplied alpha,
// we just return the colors as stored in the file. set this flag to force
// unpremultiplication. results are undefined if the unpremultiply overflow.
XIDEF void xi_set_unpremultiply_on_load(int flag_true_if_should_unpremultiply);

// indicate whether we should process iphone images back to canonical format,
// or just pass them through "as-is"
XIDEF void xi_convert_iphone_png_to_rgb(int flag_true_if_should_convert);

// flip the image vertically, so the first pixel in the output array is the bottom left
XIDEF void xi_set_flip_vertically_on_load(int flag_true_if_should_flip);

// as above, but only applies to images loaded on the thread that calls the function
// this function is only available if your compiler supports thread-local variables;
// calling it will fail to link if your compiler doesn't
XIDEF void xi_set_unpremultiply_on_load_thread(int flag_true_if_should_unpremultiply);
XIDEF void xi_convert_iphone_png_to_rgb_thread(int flag_true_if_should_convert);
XIDEF void xi_set_flip_vertically_on_load_thread(int flag_true_if_should_flip);

// ZLIB client - used by PNG, available for other purposes

XIDEF char *xi_zlib_decode_malloc_guesssize(const char *buffer, int len, int initial_size, int *outlen);
XIDEF char *xi_zlib_decode_malloc_guesssize_headerflag(const char *buffer, int len, int initial_size, int *outlen, int parse_header);
XIDEF char *xi_zlib_decode_malloc(const char *buffer, int len, int *outlen);
XIDEF int   xi_zlib_decode_buffer(char *obuffer, int olen, const char *ibuffer, int ilen);

XIDEF char *xi_zlib_decode_noheader_malloc(const char *buffer, int len, int *outlen);
XIDEF int   xi_zlib_decode_noheader_buffer(char *obuffer, int olen, const char *ibuffer, int ilen);


#ifdef __cplusplus
}
#endif

//
//
////   end header file   /////////////////////////////////////////////////////
#endif // XI_INCLUDE_XI_IMAGE_H

#ifdef XI_IMAGE_IMPLEMENTATION

#if defined(XI_ONLY_JPEG) || defined(XI_ONLY_PNG) || defined(XI_ONLY_BMP) \
  || defined(XI_ONLY_TGA) || defined(XI_ONLY_GIF) || defined(XI_ONLY_PSD) \
  || defined(XI_ONLY_HDR) || defined(XI_ONLY_PIC) || defined(XI_ONLY_PNM) \
  || defined(XI_ONLY_ZLIB)
   #ifndef XI_ONLY_JPEG
   #define XI_NO_JPEG
   #endif
   #ifndef XI_ONLY_PNG
   #define XI_NO_PNG
   #endif
   #ifndef XI_ONLY_BMP
   #define XI_NO_BMP
   #endif
   #ifndef XI_ONLY_PSD
   #define XI_NO_PSD
   #endif
   #ifndef XI_ONLY_TGA
   #define XI_NO_TGA
   #endif
   #ifndef XI_ONLY_GIF
   #define XI_NO_GIF
   #endif
   #ifndef XI_ONLY_HDR
   #define XI_NO_HDR
   #endif
   #ifndef XI_ONLY_PIC
   #define XI_NO_PIC
   #endif
   #ifndef XI_ONLY_PNM
   #define XI_NO_PNM
   #endif
#endif

#if defined(XI_NO_PNG) && !defined(XI_SUPPORT_ZLIB) && !defined(XI_NO_ZLIB)
#define XI_NO_ZLIB
#endif


#include <stdarg.h>
#include <stddef.h> // ptrdiff_t on osx
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#if !defined(XI_NO_LINEAR) || !defined(XI_NO_HDR)
#include <math.h>  // ldexp, pow
#endif

#ifndef XI_NO_STDIO
#include <stdio.h>
#endif

#ifndef XI_ASSERT
#include <assert.h>
#define XI_ASSERT(x) assert(x)
#endif

#ifdef __cplusplus
#define XI_EXTERN extern "C"
#else
#define XI_EXTERN extern
#endif


#ifndef _MSC_VER
   #ifdef __cplusplus
   #define xi_inline inline
   #else
   #define xi_inline
   #endif
#else
   #define xi_inline __forceinline
#endif

#ifndef XI_NO_THREAD_LOCALS
   #if defined(__cplusplus) &&  __cplusplus >= 201103L
      #define XI_THREAD_LOCAL       thread_local
   #elif defined(__GNUC__) && __GNUC__ < 5
      #define XI_THREAD_LOCAL       __thread
   #elif defined(_MSC_VER)
      #define XI_THREAD_LOCAL       __declspec(thread)
   #elif defined (__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__)
      #define XI_THREAD_LOCAL       _Thread_local
   #endif

   #ifndef XI_THREAD_LOCAL
      #if defined(__GNUC__)
        #define XI_THREAD_LOCAL       __thread
      #endif
   #endif
#endif

#if defined(_MSC_VER) || defined(__SYMBIAN32__)
typedef unsigned short xi__uint16;
typedef   signed short xi__int16;
typedef unsigned int   xi__uint32;
typedef   signed int   xi__int32;
#else
#include <stdint.h>
typedef uint16_t xi__uint16;
typedef int16_t  xi__int16;
typedef uint32_t xi__uint32;
typedef int32_t  xi__int32;
#endif

// should produce compiler error if size is wrong
typedef unsigned char validate_uint32[sizeof(xi__uint32)==4 ? 1 : -1];

#ifdef _MSC_VER
#define XI_NOTUSED(v)  (void)(v)
#else
#define XI_NOTUSED(v)  (void)sizeof(v)
#endif

#ifdef _MSC_VER
#define XI_HAS_LROTL
#endif

#ifdef XI_HAS_LROTL
   #define xi_lrot(x,y)  _lrotl(x,y)
#else
   #define xi_lrot(x,y)  (((x) << (y)) | ((x) >> (-(y) & 31)))
#endif

#if defined(XI_MALLOC) && defined(XI_FREE) && (defined(XI_REALLOC) || defined(XI_REALLOC_SIZED))
// ok
#elif !defined(XI_MALLOC) && !defined(XI_FREE) && !defined(XI_REALLOC) && !defined(XI_REALLOC_SIZED)
// ok
#else
#error "Must define all or none of XI_MALLOC, XI_FREE, and XI_REALLOC (or XI_REALLOC_SIZED)."
#endif

#ifndef XI_MALLOC
#define XI_MALLOC(sz)           malloc(sz)
#define XI_REALLOC(p,newsz)     realloc(p,newsz)
#define XI_FREE(p)              free(p)
#endif

#ifndef XI_REALLOC_SIZED
#define XI_REALLOC_SIZED(p,oldsz,newsz) XI_REALLOC(p,newsz)
#endif

// x86/x64 detection
#if defined(__x86_64__) || defined(_M_X64)
#define XI__X64_TARGET
#elif defined(__i386) || defined(_M_IX86)
#define XI__X86_TARGET
#endif

#if defined(__GNUC__) && defined(XI__X86_TARGET) && !defined(__SSE2__) && !defined(XI_NO_SIMD)
// gcc doesn't support sse2 intrinsics unless you compile with -msse2,
// which in turn means it gets to use SSE2 everywhere. This is unfortunate,
// but previous attempts to provide the SSE2 functions with runtime
// detection caused numerous issues. The way architecture extensions are
// exposed in GCC/Clang is, sadly, not really suited for one-file libs.
// New behavior: if compiled with -msse2, we use SSE2 without any
// detection; if not, we don't use it at all.
#define XI_NO_SIMD
#endif

#if defined(__MINGW32__) && defined(XI__X86_TARGET) && !defined(XI_MINGW_ENABLE_SSE2) && !defined(XI_NO_SIMD)
// Note that __MINGW32__ doesn't actually mean 32-bit, so we have to avoid XI__X64_TARGET
//
// 32-bit MinGW wants ESP to be 16-byte aligned, but this is not in the
// Windows ABI and VC++ as well as Windows DLLs don't maintain that invariant.
// As a result, enabling SSE2 on 32-bit MinGW is dangerous when not
// simultaneously enabling "-mstackrealign".
//
// See https://github.com/nothings/stb/issues/81 for more information.
//
// So default to no SSE2 on 32-bit MinGW. If you've read this far and added
// -mstackrealign to your build settings, feel free to #define XI_MINGW_ENABLE_SSE2.
#define XI_NO_SIMD
#endif

#if !defined(XI_NO_SIMD) && (defined(XI__X86_TARGET) || defined(XI__X64_TARGET))
#define XI_SSE2
#include <emmintrin.h>

#ifdef _MSC_VER

#if _MSC_VER >= 1400  // not VC6
#include <intrin.h> // __cpuid
static int xi__cpuid3(void)
{
   int info[4];
   __cpuid(info,1);
   return info[3];
}
#else
static int xi__cpuid3(void)
{
   int res;
   __asm {
      mov  eax,1
      cpuid
      mov  res,edx
   }
   return res;
}
#endif

#define XI_SIMD_ALIGN(type, name) __declspec(align(16)) type name

#if !defined(XI_NO_JPEG) && defined(XI_SSE2)
static int xi__sse2_available(void)
{
   int info3 = xi__cpuid3();
   return ((info3 >> 26) & 1) != 0;
}
#endif

#else // assume GCC-style if not VC++
#define XI_SIMD_ALIGN(type, name) type name __attribute__((aligned(16)))

#if !defined(XI_NO_JPEG) && defined(XI_SSE2)
static int xi__sse2_available(void)
{
   // If we're even attempting to compile this on GCC/Clang, that means
   // -msse2 is on, which means the compiler is allowed to use SSE2
   // instructions at will, and so are we.
   return 1;
}
#endif

#endif
#endif

// ARM NEON
#if defined(XI_NO_SIMD) && defined(XI_NEON)
#undef XI_NEON
#endif

#ifdef XI_NEON
#include <arm_neon.h>
#ifdef _MSC_VER
#define XI_SIMD_ALIGN(type, name) __declspec(align(16)) type name
#else
#define XI_SIMD_ALIGN(type, name) type name __attribute__((aligned(16)))
#endif
#endif

#ifndef XI_SIMD_ALIGN
#define XI_SIMD_ALIGN(type, name) type name
#endif

#ifndef XI_MAX_DIMENSIONS
#define XI_MAX_DIMENSIONS (1 << 24)
#endif

///////////////////////////////////////////////
//
//  xi__context struct and start_xxx functions

// xi__context structure is our basic context used by all images, so it
// contains all the IO context, plus some basic image information
typedef struct
{
   xi__uint32 img_x, img_y;
   int img_n, img_out_n;

   xi_io_callbacks io;
   void *io_user_data;

   int read_from_callbacks;
   int buflen;
   xi_uc buffer_start[128];
   int callback_already_read;

   xi_uc *img_buffer, *img_buffer_end;
   xi_uc *img_buffer_original, *img_buffer_original_end;
} xi__context;


static void xi__refill_buffer(xi__context *s);

// initialize a memory-decode context
static void xi__start_mem(xi__context *s, xi_uc const *buffer, int len)
{
   s->io.read = NULL;
   s->read_from_callbacks = 0;
   s->callback_already_read = 0;
   s->img_buffer = s->img_buffer_original = (xi_uc *) buffer;
   s->img_buffer_end = s->img_buffer_original_end = (xi_uc *) buffer+len;
}

// initialize a callback-based context
static void xi__start_callbacks(xi__context *s, xi_io_callbacks *c, void *user)
{
   s->io = *c;
   s->io_user_data = user;
   s->buflen = sizeof(s->buffer_start);
   s->read_from_callbacks = 1;
   s->callback_already_read = 0;
   s->img_buffer = s->img_buffer_original = s->buffer_start;
   xi__refill_buffer(s);
   s->img_buffer_original_end = s->img_buffer_end;
}

#ifndef XI_NO_STDIO

static int xi__stdio_read(void *user, char *data, int size)
{
   return (int) fread(data,1,size,(FILE*) user);
}

static void xi__stdio_skip(void *user, int n)
{
   int ch;
   fseek((FILE*) user, n, SEEK_CUR);
   ch = fgetc((FILE*) user);  /* have to read a byte to reset feof()'s flag */
   if (ch != EOF) {
      ungetc(ch, (FILE *) user);  /* push byte back onto stream if valid. */
   }
}

static int xi__stdio_eof(void *user)
{
   return feof((FILE*) user) || ferror((FILE *) user);
}

static xi_io_callbacks xi__stdio_callbacks =
{
   xi__stdio_read,
   xi__stdio_skip,
   xi__stdio_eof,
};

static void xi__start_file(xi__context *s, FILE *f)
{
   xi__start_callbacks(s, &xi__stdio_callbacks, (void *) f);
}

//static void stop_file(xi__context *s) { }

#endif // !XI_NO_STDIO

static void xi__rewind(xi__context *s)
{
   // conceptually rewind SHOULD rewind to the beginning of the stream,
   // but we just rewind to the beginning of the initial buffer, because
   // we only use it after doing 'test', which only ever looks at at most 92 bytes
   s->img_buffer = s->img_buffer_original;
   s->img_buffer_end = s->img_buffer_original_end;
}

enum
{
   XI_ORDER_RGB,
   XI_ORDER_BGR
};

typedef struct
{
   int bits_per_channel;
   int num_channels;
   int channel_order;
} xi__result_info;

#ifndef XI_NO_JPEG
static int      xi__jpeg_test(xi__context *s);
static void    *xi__jpeg_load(xi__context *s, int *x, int *y, int *comp, int req_comp, xi__result_info *ri);
static int      xi__jpeg_info(xi__context *s, int *x, int *y, int *comp);
#endif

#ifndef XI_NO_PNG
static int      xi__png_test(xi__context *s);
static void    *xi__png_load(xi__context *s, int *x, int *y, int *comp, int req_comp, xi__result_info *ri);
static int      xi__png_info(xi__context *s, int *x, int *y, int *comp);
static int      xi__png_is16(xi__context *s);
#endif

#ifndef XI_NO_BMP
static int      xi__bmp_test(xi__context *s);
static void    *xi__bmp_load(xi__context *s, int *x, int *y, int *comp, int req_comp, xi__result_info *ri);
static int      xi__bmp_info(xi__context *s, int *x, int *y, int *comp);
#endif

#ifndef XI_NO_TGA
static int      xi__tga_test(xi__context *s);
static void    *xi__tga_load(xi__context *s, int *x, int *y, int *comp, int req_comp, xi__result_info *ri);
static int      xi__tga_info(xi__context *s, int *x, int *y, int *comp);
#endif

#ifndef XI_NO_PSD
static int      xi__psd_test(xi__context *s);
static void    *xi__psd_load(xi__context *s, int *x, int *y, int *comp, int req_comp, xi__result_info *ri, int bpc);
static int      xi__psd_info(xi__context *s, int *x, int *y, int *comp);
static int      xi__psd_is16(xi__context *s);
#endif

#ifndef XI_NO_HDR
static int      xi__hdr_test(xi__context *s);
static float   *xi__hdr_load(xi__context *s, int *x, int *y, int *comp, int req_comp, xi__result_info *ri);
static int      xi__hdr_info(xi__context *s, int *x, int *y, int *comp);
#endif

#ifndef XI_NO_PIC
static int      xi__pic_test(xi__context *s);
static void    *xi__pic_load(xi__context *s, int *x, int *y, int *comp, int req_comp, xi__result_info *ri);
static int      xi__pic_info(xi__context *s, int *x, int *y, int *comp);
#endif

#ifndef XI_NO_GIF
static int      xi__gif_test(xi__context *s);
static void    *xi__gif_load(xi__context *s, int *x, int *y, int *comp, int req_comp, xi__result_info *ri);
static void    *xi__load_gif_main(xi__context *s, int **delays, int *x, int *y, int *z, int *comp, int req_comp);
static int      xi__gif_info(xi__context *s, int *x, int *y, int *comp);
#endif

#ifndef XI_NO_PNM
static int      xi__pnm_test(xi__context *s);
static void    *xi__pnm_load(xi__context *s, int *x, int *y, int *comp, int req_comp, xi__result_info *ri);
static int      xi__pnm_info(xi__context *s, int *x, int *y, int *comp);
static int      xi__pnm_is16(xi__context *s);
#endif

static
#ifdef XI_THREAD_LOCAL
XI_THREAD_LOCAL
#endif
const char *xi__g_failure_reason;

XIDEF const char *xi_failure_reason(void)
{
   return xi__g_failure_reason;
}

#ifndef XI_NO_FAILURE_STRINGS
static int xi__err(const char *str)
{
   xi__g_failure_reason = str;
   return 0;
}
#endif

static void *xi__malloc(size_t size)
{
    return XI_MALLOC(size);
}

// stb_image uses ints pervasively, including for offset calculations.
// therefore the largest decoded image size we can support with the
// current code, even on 64-bit targets, is INT_MAX. this is not a
// significant limitation for the intended use case.
//
// we do, however, need to make sure our size calculations don't
// overflow. hence a few helper functions for size calculations that
// multiply integers together, making sure that they're non-negative
// and no overflow occurs.

// return 1 if the sum is valid, 0 on overflow.
// negative terms are considered invalid.
static int xi__addsizes_valid(int a, int b)
{
   if (b < 0) return 0;
   // now 0 <= b <= INT_MAX, hence also
   // 0 <= INT_MAX - b <= INTMAX.
   // And "a + b <= INT_MAX" (which might overflow) is the
   // same as a <= INT_MAX - b (no overflow)
   return a <= INT_MAX - b;
}

// returns 1 if the product is valid, 0 on overflow.
// negative factors are considered invalid.
static int xi__mul2sizes_valid(int a, int b)
{
   if (a < 0 || b < 0) return 0;
   if (b == 0) return 1; // mul-by-0 is always safe
   // portable way to check for no overflows in a*b
   return a <= INT_MAX/b;
}

#if !defined(XI_NO_JPEG) || !defined(XI_NO_PNG) || !defined(XI_NO_TGA) || !defined(XI_NO_HDR)
// returns 1 if "a*b + add" has no negative terms/factors and doesn't overflow
static int xi__mad2sizes_valid(int a, int b, int add)
{
   return xi__mul2sizes_valid(a, b) && xi__addsizes_valid(a*b, add);
}
#endif

// returns 1 if "a*b*c + add" has no negative terms/factors and doesn't overflow
static int xi__mad3sizes_valid(int a, int b, int c, int add)
{
   return xi__mul2sizes_valid(a, b) && xi__mul2sizes_valid(a*b, c) &&
      xi__addsizes_valid(a*b*c, add);
}

// returns 1 if "a*b*c*d + add" has no negative terms/factors and doesn't overflow
#if !defined(XI_NO_LINEAR) || !defined(XI_NO_HDR) || !defined(XI_NO_PNM)
static int xi__mad4sizes_valid(int a, int b, int c, int d, int add)
{
   return xi__mul2sizes_valid(a, b) && xi__mul2sizes_valid(a*b, c) &&
      xi__mul2sizes_valid(a*b*c, d) && xi__addsizes_valid(a*b*c*d, add);
}
#endif

#if !defined(XI_NO_JPEG) || !defined(XI_NO_PNG) || !defined(XI_NO_TGA) || !defined(XI_NO_HDR)
// mallocs with size overflow checking
static void *xi__malloc_mad2(int a, int b, int add)
{
   if (!xi__mad2sizes_valid(a, b, add)) return NULL;
   return xi__malloc(a*b + add);
}
#endif

static void *xi__malloc_mad3(int a, int b, int c, int add)
{
   if (!xi__mad3sizes_valid(a, b, c, add)) return NULL;
   return xi__malloc(a*b*c + add);
}

#if !defined(XI_NO_LINEAR) || !defined(XI_NO_HDR) || !defined(XI_NO_PNM)
static void *xi__malloc_mad4(int a, int b, int c, int d, int add)
{
   if (!xi__mad4sizes_valid(a, b, c, d, add)) return NULL;
   return xi__malloc(a*b*c*d + add);
}
#endif

// returns 1 if the sum of two signed ints is valid (between -2^31 and 2^31-1 inclusive), 0 on overflow.
static int xi__addints_valid(int a, int b)
{
   if ((a >= 0) != (b >= 0)) return 1; // a and b have different signs, so no overflow
   if (a < 0 && b < 0) return a >= INT_MIN - b; // same as a + b >= INT_MIN; INT_MIN - b cannot overflow since b < 0.
   return a <= INT_MAX - b;
}

// returns 1 if the product of two ints fits in a signed short, 0 on overflow.
static int xi__mul2shorts_valid(int a, int b)
{
   if (b == 0 || b == -1) return 1; // multiplication by 0 is always 0; check for -1 so SHRT_MIN/b doesn't overflow
   if ((a >= 0) == (b >= 0)) return a <= SHRT_MAX/b; // product is positive, so similar to mul2sizes_valid
   if (b < 0) return a <= SHRT_MIN / b; // same as a * b >= SHRT_MIN
   return a >= SHRT_MIN / b;
}

// xi__err - error
// xi__errpf - error returning pointer to float
// xi__errpuc - error returning pointer to unsigned char

#ifdef XI_NO_FAILURE_STRINGS
   #define xi__err(x,y)  0
#elif defined(XI_FAILURE_USERMSG)
   #define xi__err(x,y)  xi__err(y)
#else
   #define xi__err(x,y)  xi__err(x)
#endif

#define xi__errpf(x,y)   ((float *)(size_t) (xi__err(x,y)?NULL:NULL))
#define xi__errpuc(x,y)  ((unsigned char *)(size_t) (xi__err(x,y)?NULL:NULL))

XIDEF void xi_image_free(void *retval_from_xi_load)
{
   XI_FREE(retval_from_xi_load);
}

#ifndef XI_NO_LINEAR
static float   *xi__ldr_to_hdr(xi_uc *data, int x, int y, int comp);
#endif

#ifndef XI_NO_HDR
static xi_uc *xi__hdr_to_ldr(float   *data, int x, int y, int comp);
#endif

static int xi__vertically_flip_on_load_global = 0;

XIDEF void xi_set_flip_vertically_on_load(int flag_true_if_should_flip)
{
   xi__vertically_flip_on_load_global = flag_true_if_should_flip;
}

#ifndef XI_THREAD_LOCAL
#define xi__vertically_flip_on_load  xi__vertically_flip_on_load_global
#else
static XI_THREAD_LOCAL int xi__vertically_flip_on_load_local, xi__vertically_flip_on_load_set;

XIDEF void xi_set_flip_vertically_on_load_thread(int flag_true_if_should_flip)
{
   xi__vertically_flip_on_load_local = flag_true_if_should_flip;
   xi__vertically_flip_on_load_set = 1;
}

#define xi__vertically_flip_on_load  (xi__vertically_flip_on_load_set       \
                                         ? xi__vertically_flip_on_load_local  \
                                         : xi__vertically_flip_on_load_global)
#endif // XI_THREAD_LOCAL

static void *xi__load_main(xi__context *s, int *x, int *y, int *comp, int req_comp, xi__result_info *ri, int bpc)
{
   memset(ri, 0, sizeof(*ri)); // make sure it's initialized if we add new fields
   ri->bits_per_channel = 8; // default is 8 so most paths don't have to be changed
   ri->channel_order = XI_ORDER_RGB; // all current input & output are this, but this is here so we can add BGR order
   ri->num_channels = 0;

   // test the formats with a very explicit header first (at least a FOURCC
   // or distinctive magic number first)
   #ifndef XI_NO_PNG
   if (xi__png_test(s))  return xi__png_load(s,x,y,comp,req_comp, ri);
   #endif
   #ifndef XI_NO_BMP
   if (xi__bmp_test(s))  return xi__bmp_load(s,x,y,comp,req_comp, ri);
   #endif
   #ifndef XI_NO_GIF
   if (xi__gif_test(s))  return xi__gif_load(s,x,y,comp,req_comp, ri);
   #endif
   #ifndef XI_NO_PSD
   if (xi__psd_test(s))  return xi__psd_load(s,x,y,comp,req_comp, ri, bpc);
   #else
   XI_NOTUSED(bpc);
   #endif
   #ifndef XI_NO_PIC
   if (xi__pic_test(s))  return xi__pic_load(s,x,y,comp,req_comp, ri);
   #endif

   // then the formats that can end up attempting to load with just 1 or 2
   // bytes matching expectations; these are prone to false positives, so
   // try them later
   #ifndef XI_NO_JPEG
   if (xi__jpeg_test(s)) return xi__jpeg_load(s,x,y,comp,req_comp, ri);
   #endif
   #ifndef XI_NO_PNM
   if (xi__pnm_test(s))  return xi__pnm_load(s,x,y,comp,req_comp, ri);
   #endif

   #ifndef XI_NO_HDR
   if (xi__hdr_test(s)) {
      float *hdr = xi__hdr_load(s, x,y,comp,req_comp, ri);
      return xi__hdr_to_ldr(hdr, *x, *y, req_comp ? req_comp : *comp);
   }
   #endif

   #ifndef XI_NO_TGA
   // test tga last because it's a crappy test!
   if (xi__tga_test(s))
      return xi__tga_load(s,x,y,comp,req_comp, ri);
   #endif

   return xi__errpuc("unknown image type", "Image not of any known type, or corrupt");
}

static xi_uc *xi__convert_16_to_8(xi__uint16 *orig, int w, int h, int channels)
{
   int i;
   int img_len = w * h * channels;
   xi_uc *reduced;

   reduced = (xi_uc *) xi__malloc(img_len);
   if (reduced == NULL) return xi__errpuc("outofmem", "Out of memory");

   for (i = 0; i < img_len; ++i)
      reduced[i] = (xi_uc)((orig[i] >> 8) & 0xFF); // top half of each byte is sufficient approx of 16->8 bit scaling

   XI_FREE(orig);
   return reduced;
}

static xi__uint16 *xi__convert_8_to_16(xi_uc *orig, int w, int h, int channels)
{
   int i;
   int img_len = w * h * channels;
   xi__uint16 *enlarged;

   enlarged = (xi__uint16 *) xi__malloc(img_len*2);
   if (enlarged == NULL) return (xi__uint16 *) xi__errpuc("outofmem", "Out of memory");

   for (i = 0; i < img_len; ++i)
      enlarged[i] = (xi__uint16)((orig[i] << 8) + orig[i]); // replicate to high and low byte, maps 0->0, 255->0xffff

   XI_FREE(orig);
   return enlarged;
}

static void xi__vertical_flip(void *image, int w, int h, int bytes_per_pixel)
{
   int row;
   size_t bytes_per_row = (size_t)w * bytes_per_pixel;
   xi_uc temp[2048];
   xi_uc *bytes = (xi_uc *)image;

   for (row = 0; row < (h>>1); row++) {
      xi_uc *row0 = bytes + row*bytes_per_row;
      xi_uc *row1 = bytes + (h - row - 1)*bytes_per_row;
      // swap row0 with row1
      size_t bytes_left = bytes_per_row;
      while (bytes_left) {
         size_t bytes_copy = (bytes_left < sizeof(temp)) ? bytes_left : sizeof(temp);
         memcpy(temp, row0, bytes_copy);
         memcpy(row0, row1, bytes_copy);
         memcpy(row1, temp, bytes_copy);
         row0 += bytes_copy;
         row1 += bytes_copy;
         bytes_left -= bytes_copy;
      }
   }
}

#ifndef XI_NO_GIF
static void xi__vertical_flip_slices(void *image, int w, int h, int z, int bytes_per_pixel)
{
   int slice;
   int slice_size = w * h * bytes_per_pixel;

   xi_uc *bytes = (xi_uc *)image;
   for (slice = 0; slice < z; ++slice) {
      xi__vertical_flip(bytes, w, h, bytes_per_pixel);
      bytes += slice_size;
   }
}
#endif

static unsigned char *xi__load_and_postprocess_8bit(xi__context *s, int *x, int *y, int *comp, int req_comp)
{
   xi__result_info ri;
   void *result = xi__load_main(s, x, y, comp, req_comp, &ri, 8);

   if (result == NULL)
      return NULL;

   // it is the responsibility of the loaders to make sure we get either 8 or 16 bit.
   XI_ASSERT(ri.bits_per_channel == 8 || ri.bits_per_channel == 16);

   if (ri.bits_per_channel != 8) {
      result = xi__convert_16_to_8((xi__uint16 *) result, *x, *y, req_comp == 0 ? *comp : req_comp);
      ri.bits_per_channel = 8;
   }

   // @TODO: move xi__convert_format to here

   if (xi__vertically_flip_on_load) {
      int channels = req_comp ? req_comp : *comp;
      xi__vertical_flip(result, *x, *y, channels * sizeof(xi_uc));
   }

   return (unsigned char *) result;
}

static xi__uint16 *xi__load_and_postprocess_16bit(xi__context *s, int *x, int *y, int *comp, int req_comp)
{
   xi__result_info ri;
   void *result = xi__load_main(s, x, y, comp, req_comp, &ri, 16);

   if (result == NULL)
      return NULL;

   // it is the responsibility of the loaders to make sure we get either 8 or 16 bit.
   XI_ASSERT(ri.bits_per_channel == 8 || ri.bits_per_channel == 16);

   if (ri.bits_per_channel != 16) {
      result = xi__convert_8_to_16((xi_uc *) result, *x, *y, req_comp == 0 ? *comp : req_comp);
      ri.bits_per_channel = 16;
   }

   // @TODO: move xi__convert_format16 to here
   // @TODO: special case RGB-to-Y (and RGBA-to-YA) for 8-bit-to-16-bit case to keep more precision

   if (xi__vertically_flip_on_load) {
      int channels = req_comp ? req_comp : *comp;
      xi__vertical_flip(result, *x, *y, channels * sizeof(xi__uint16));
   }

   return (xi__uint16 *) result;
}

#if !defined(XI_NO_HDR) && !defined(XI_NO_LINEAR)
static void xi__float_postprocess(float *result, int *x, int *y, int *comp, int req_comp)
{
   if (xi__vertically_flip_on_load && result != NULL) {
      int channels = req_comp ? req_comp : *comp;
      xi__vertical_flip(result, *x, *y, channels * sizeof(float));
   }
}
#endif

#ifndef XI_NO_STDIO

#if defined(_WIN32) && defined(XI_WINDOWS_UTF8)
XI_EXTERN __declspec(dllimport) int __stdcall MultiByteToWideChar(unsigned int cp, unsigned long flags, const char *str, int cbmb, wchar_t *widestr, int cchwide);
XI_EXTERN __declspec(dllimport) int __stdcall WideCharToMultiByte(unsigned int cp, unsigned long flags, const wchar_t *widestr, int cchwide, char *str, int cbmb, const char *defchar, int *used_default);
#endif

#if defined(_WIN32) && defined(XI_WINDOWS_UTF8)
XIDEF int xi_convert_wchar_to_utf8(char *buffer, size_t bufferlen, const wchar_t* input)
{
return WideCharToMultiByte(65001 /* UTF8 */, 0, input, -1, buffer, (int) bufferlen, NULL, NULL);
}
#endif

static FILE *xi__fopen(char const *filename, char const *mode)
{
   FILE *f;
#if defined(_WIN32) && defined(XI_WINDOWS_UTF8)
   wchar_t wMode[64];
   wchar_t wFilename[1024];
if (0 == MultiByteToWideChar(65001 /* UTF8 */, 0, filename, -1, wFilename, sizeof(wFilename)/sizeof(*wFilename)))
      return 0;

if (0 == MultiByteToWideChar(65001 /* UTF8 */, 0, mode, -1, wMode, sizeof(wMode)/sizeof(*wMode)))
      return 0;

#if defined(_MSC_VER) && _MSC_VER >= 1400
if (0 != _wfopen_s(&f, wFilename, wMode))
f = 0;
#else
   f = _wfopen(wFilename, wMode);
#endif

#elif defined(_MSC_VER) && _MSC_VER >= 1400
   if (0 != fopen_s(&f, filename, mode))
      f=0;
#else
   f = fopen(filename, mode);
#endif
   return f;
}


XIDEF xi_uc *xi_load(char const *filename, int *x, int *y, int *comp, int req_comp)
{
   FILE *f = xi__fopen(filename, "rb");
   unsigned char *result;
   if (!f) return xi__errpuc("can't fopen", "Unable to open file");
   result = xi_load_from_file(f,x,y,comp,req_comp);
   fclose(f);
   return result;
}

XIDEF xi_uc *xi_load_from_file(FILE *f, int *x, int *y, int *comp, int req_comp)
{
   unsigned char *result;
   xi__context s;
   xi__start_file(&s,f);
   result = xi__load_and_postprocess_8bit(&s,x,y,comp,req_comp);
   if (result) {
      // need to 'unget' all the characters in the IO buffer
      fseek(f, - (int) (s.img_buffer_end - s.img_buffer), SEEK_CUR);
   }
   return result;
}

XIDEF xi__uint16 *xi_load_from_file_16(FILE *f, int *x, int *y, int *comp, int req_comp)
{
   xi__uint16 *result;
   xi__context s;
   xi__start_file(&s,f);
   result = xi__load_and_postprocess_16bit(&s,x,y,comp,req_comp);
   if (result) {
      // need to 'unget' all the characters in the IO buffer
      fseek(f, - (int) (s.img_buffer_end - s.img_buffer), SEEK_CUR);
   }
   return result;
}

XIDEF xi_us *xi_load_16(char const *filename, int *x, int *y, int *comp, int req_comp)
{
   FILE *f = xi__fopen(filename, "rb");
   xi__uint16 *result;
   if (!f) return (xi_us *) xi__errpuc("can't fopen", "Unable to open file");
   result = xi_load_from_file_16(f,x,y,comp,req_comp);
   fclose(f);
   return result;
}


#endif //!XI_NO_STDIO

XIDEF xi_us *xi_load_16_from_memory(xi_uc const *buffer, int len, int *x, int *y, int *channels_in_file, int desired_channels)
{
   xi__context s;
   xi__start_mem(&s,buffer,len);
   return xi__load_and_postprocess_16bit(&s,x,y,channels_in_file,desired_channels);
}

XIDEF xi_us *xi_load_16_from_callbacks(xi_io_callbacks const *clbk, void *user, int *x, int *y, int *channels_in_file, int desired_channels)
{
   xi__context s;
   xi__start_callbacks(&s, (xi_io_callbacks *)clbk, user);
   return xi__load_and_postprocess_16bit(&s,x,y,channels_in_file,desired_channels);
}

XIDEF xi_uc *xi_load_from_memory(xi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp)
{
   xi__context s;
   xi__start_mem(&s,buffer,len);
   return xi__load_and_postprocess_8bit(&s,x,y,comp,req_comp);
}

XIDEF xi_uc *xi_load_from_callbacks(xi_io_callbacks const *clbk, void *user, int *x, int *y, int *comp, int req_comp)
{
   xi__context s;
   xi__start_callbacks(&s, (xi_io_callbacks *) clbk, user);
   return xi__load_and_postprocess_8bit(&s,x,y,comp,req_comp);
}

#ifndef XI_NO_GIF
XIDEF xi_uc *xi_load_gif_from_memory(xi_uc const *buffer, int len, int **delays, int *x, int *y, int *z, int *comp, int req_comp)
{
   unsigned char *result;
   xi__context s;
   xi__start_mem(&s,buffer,len);

   result = (unsigned char*) xi__load_gif_main(&s, delays, x, y, z, comp, req_comp);
   if (xi__vertically_flip_on_load) {
      xi__vertical_flip_slices( result, *x, *y, *z, *comp );
   }

   return result;
}
#endif

#ifndef XI_NO_LINEAR
static float *xi__loadf_main(xi__context *s, int *x, int *y, int *comp, int req_comp)
{
   unsigned char *data;
   #ifndef XI_NO_HDR
   if (xi__hdr_test(s)) {
      xi__result_info ri;
      float *hdr_data = xi__hdr_load(s,x,y,comp,req_comp, &ri);
      if (hdr_data)
         xi__float_postprocess(hdr_data,x,y,comp,req_comp);
      return hdr_data;
   }
   #endif
   data = xi__load_and_postprocess_8bit(s, x, y, comp, req_comp);
   if (data)
      return xi__ldr_to_hdr(data, *x, *y, req_comp ? req_comp : *comp);
   return xi__errpf("unknown image type", "Image not of any known type, or corrupt");
}

XIDEF float *xi_loadf_from_memory(xi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp)
{
   xi__context s;
   xi__start_mem(&s,buffer,len);
   return xi__loadf_main(&s,x,y,comp,req_comp);
}

XIDEF float *xi_loadf_from_callbacks(xi_io_callbacks const *clbk, void *user, int *x, int *y, int *comp, int req_comp)
{
   xi__context s;
   xi__start_callbacks(&s, (xi_io_callbacks *) clbk, user);
   return xi__loadf_main(&s,x,y,comp,req_comp);
}

#ifndef XI_NO_STDIO
XIDEF float *xi_loadf(char const *filename, int *x, int *y, int *comp, int req_comp)
{
   float *result;
   FILE *f = xi__fopen(filename, "rb");
   if (!f) return xi__errpf("can't fopen", "Unable to open file");
   result = xi_loadf_from_file(f,x,y,comp,req_comp);
   fclose(f);
   return result;
}

XIDEF float *xi_loadf_from_file(FILE *f, int *x, int *y, int *comp, int req_comp)
{
   xi__context s;
   xi__start_file(&s,f);
   return xi__loadf_main(&s,x,y,comp,req_comp);
}
#endif // !XI_NO_STDIO

#endif // !XI_NO_LINEAR

// these is-hdr-or-not is defined independent of whether XI_NO_LINEAR is
// defined, for API simplicity; if XI_NO_LINEAR is defined, it always
// reports false!

XIDEF int xi_is_hdr_from_memory(xi_uc const *buffer, int len)
{
   #ifndef XI_NO_HDR
   xi__context s;
   xi__start_mem(&s,buffer,len);
   return xi__hdr_test(&s);
   #else
   XI_NOTUSED(buffer);
   XI_NOTUSED(len);
   return 0;
   #endif
}

#ifndef XI_NO_STDIO
XIDEF int      xi_is_hdr          (char const *filename)
{
   FILE *f = xi__fopen(filename, "rb");
   int result=0;
   if (f) {
      result = xi_is_hdr_from_file(f);
      fclose(f);
   }
   return result;
}

XIDEF int xi_is_hdr_from_file(FILE *f)
{
   #ifndef XI_NO_HDR
   long pos = ftell(f);
   int res;
   xi__context s;
   xi__start_file(&s,f);
   res = xi__hdr_test(&s);
   fseek(f, pos, SEEK_SET);
   return res;
   #else
   XI_NOTUSED(f);
   return 0;
   #endif
}
#endif // !XI_NO_STDIO

XIDEF int      xi_is_hdr_from_callbacks(xi_io_callbacks const *clbk, void *user)
{
   #ifndef XI_NO_HDR
   xi__context s;
   xi__start_callbacks(&s, (xi_io_callbacks *) clbk, user);
   return xi__hdr_test(&s);
   #else
   XI_NOTUSED(clbk);
   XI_NOTUSED(user);
   return 0;
   #endif
}

#ifndef XI_NO_LINEAR
static float xi__l2h_gamma=2.2f, xi__l2h_scale=1.0f;

XIDEF void   xi_ldr_to_hdr_gamma(float gamma) { xi__l2h_gamma = gamma; }
XIDEF void   xi_ldr_to_hdr_scale(float scale) { xi__l2h_scale = scale; }
#endif

static float xi__h2l_gamma_i=1.0f/2.2f, xi__h2l_scale_i=1.0f;

XIDEF void   xi_hdr_to_ldr_gamma(float gamma) { xi__h2l_gamma_i = 1/gamma; }
XIDEF void   xi_hdr_to_ldr_scale(float scale) { xi__h2l_scale_i = 1/scale; }


//////////////////////////////////////////////////////////////////////////////
//
// Common code used by all image loaders
//

enum
{
   XI__SCAN_load=0,
   XI__SCAN_type,
   XI__SCAN_header
};

static void xi__refill_buffer(xi__context *s)
{
   int n = (s->io.read)(s->io_user_data,(char*)s->buffer_start,s->buflen);
   s->callback_already_read += (int) (s->img_buffer - s->img_buffer_original);
   if (n == 0) {
      // at end of file, treat same as if from memory, but need to handle case
      // where s->img_buffer isn't pointing to safe memory, e.g. 0-byte file
      s->read_from_callbacks = 0;
      s->img_buffer = s->buffer_start;
      s->img_buffer_end = s->buffer_start+1;
      *s->img_buffer = 0;
   } else {
      s->img_buffer = s->buffer_start;
      s->img_buffer_end = s->buffer_start + n;
   }
}

xi_inline static xi_uc xi__get8(xi__context *s)
{
   if (s->img_buffer < s->img_buffer_end)
      return *s->img_buffer++;
   if (s->read_from_callbacks) {
      xi__refill_buffer(s);
      return *s->img_buffer++;
   }
   return 0;
}

#if defined(XI_NO_JPEG) && defined(XI_NO_HDR) && defined(XI_NO_PIC) && defined(XI_NO_PNM)
// nothing
#else
xi_inline static int xi__at_eof(xi__context *s)
{
   if (s->io.read) {
      if (!(s->io.eof)(s->io_user_data)) return 0;
      // if feof() is true, check if buffer = end
      // special case: we've only got the special 0 character at the end
      if (s->read_from_callbacks == 0) return 1;
   }

   return s->img_buffer >= s->img_buffer_end;
}
#endif

#if defined(XI_NO_JPEG) && defined(XI_NO_PNG) && defined(XI_NO_BMP) && defined(XI_NO_PSD) && defined(XI_NO_TGA) && defined(XI_NO_GIF) && defined(XI_NO_PIC)
// nothing
#else
static void xi__skip(xi__context *s, int n)
{
   if (n == 0) return;  // already there!
   if (n < 0) {
      s->img_buffer = s->img_buffer_end;
      return;
   }
   if (s->io.read) {
      int blen = (int) (s->img_buffer_end - s->img_buffer);
      if (blen < n) {
         s->img_buffer = s->img_buffer_end;
         (s->io.skip)(s->io_user_data, n - blen);
         return;
      }
   }
   s->img_buffer += n;
}
#endif

#if defined(XI_NO_PNG) && defined(XI_NO_TGA) && defined(XI_NO_HDR) && defined(XI_NO_PNM)
// nothing
#else
static int xi__getn(xi__context *s, xi_uc *buffer, int n)
{
   if (s->io.read) {
      int blen = (int) (s->img_buffer_end - s->img_buffer);
      if (blen < n) {
         int res, count;

         memcpy(buffer, s->img_buffer, blen);

         count = (s->io.read)(s->io_user_data, (char*) buffer + blen, n - blen);
         res = (count == (n-blen));
         s->img_buffer = s->img_buffer_end;
         return res;
      }
   }

   if (s->img_buffer+n <= s->img_buffer_end) {
      memcpy(buffer, s->img_buffer, n);
      s->img_buffer += n;
      return 1;
   } else
      return 0;
}
#endif

#if defined(XI_NO_JPEG) && defined(XI_NO_PNG) && defined(XI_NO_PSD) && defined(XI_NO_PIC)
// nothing
#else
static int xi__get16be(xi__context *s)
{
   int z = xi__get8(s);
   return (z << 8) + xi__get8(s);
}
#endif

#if defined(XI_NO_PNG) && defined(XI_NO_PSD) && defined(XI_NO_PIC)
// nothing
#else
static xi__uint32 xi__get32be(xi__context *s)
{
   xi__uint32 z = xi__get16be(s);
   return (z << 16) + xi__get16be(s);
}
#endif

#if defined(XI_NO_BMP) && defined(XI_NO_TGA) && defined(XI_NO_GIF)
// nothing
#else
static int xi__get16le(xi__context *s)
{
   int z = xi__get8(s);
   return z + (xi__get8(s) << 8);
}
#endif

#ifndef XI_NO_BMP
static xi__uint32 xi__get32le(xi__context *s)
{
   xi__uint32 z = xi__get16le(s);
   z += (xi__uint32)xi__get16le(s) << 16;
   return z;
}
#endif

#define XI__BYTECAST(x)  ((xi_uc) ((x) & 255))  // truncate int to byte without warnings

#if defined(XI_NO_JPEG) && defined(XI_NO_PNG) && defined(XI_NO_BMP) && defined(XI_NO_PSD) && defined(XI_NO_TGA) && defined(XI_NO_GIF) && defined(XI_NO_PIC) && defined(XI_NO_PNM)
// nothing
#else
//////////////////////////////////////////////////////////////////////////////
//
//  generic converter from built-in img_n to req_comp
//    individual types do this automatically as much as possible (e.g. jpeg
//    does all cases internally since it needs to colorspace convert anyway,
//    and it never has alpha, so very few cases ). png can automatically
//    interleave an alpha=255 channel, but falls back to this for other cases
//
//  assume data buffer is malloced, so malloc a new one and free that one
//  only failure mode is malloc failing

static xi_uc xi__compute_y(int r, int g, int b)
{
   return (xi_uc) (((r*77) + (g*150) +  (29*b)) >> 8);
}
#endif

#if defined(XI_NO_PNG) && defined(XI_NO_BMP) && defined(XI_NO_PSD) && defined(XI_NO_TGA) && defined(XI_NO_GIF) && defined(XI_NO_PIC) && defined(XI_NO_PNM)
// nothing
#else
static unsigned char *xi__convert_format(unsigned char *data, int img_n, int req_comp, unsigned int x, unsigned int y)
{
   int i,j;
   unsigned char *good;

   if (req_comp == img_n) return data;
   XI_ASSERT(req_comp >= 1 && req_comp <= 4);

   good = (unsigned char *) xi__malloc_mad3(req_comp, x, y, 0);
   if (good == NULL) {
      XI_FREE(data);
      return xi__errpuc("outofmem", "Out of memory");
   }

   for (j=0; j < (int) y; ++j) {
      unsigned char *src  = data + j * x * img_n   ;
      unsigned char *dest = good + j * x * req_comp;

      #define XI__COMBO(a,b)  ((a)*8+(b))
      #define XI__CASE(a,b)   case XI__COMBO(a,b): for(i=x-1; i >= 0; --i, src += a, dest += b)
      // convert source image with img_n components to one with req_comp components;
      // avoid switch per pixel, so use switch per scanline and massive macros
      switch (XI__COMBO(img_n, req_comp)) {
         XI__CASE(1,2) { dest[0]=src[0]; dest[1]=255;                                     } break;
         XI__CASE(1,3) { dest[0]=dest[1]=dest[2]=src[0];                                  } break;
         XI__CASE(1,4) { dest[0]=dest[1]=dest[2]=src[0]; dest[3]=255;                     } break;
         XI__CASE(2,1) { dest[0]=src[0];                                                  } break;
         XI__CASE(2,3) { dest[0]=dest[1]=dest[2]=src[0];                                  } break;
         XI__CASE(2,4) { dest[0]=dest[1]=dest[2]=src[0]; dest[3]=src[1];                  } break;
         XI__CASE(3,4) { dest[0]=src[0];dest[1]=src[1];dest[2]=src[2];dest[3]=255;        } break;
         XI__CASE(3,1) { dest[0]=xi__compute_y(src[0],src[1],src[2]);                   } break;
         XI__CASE(3,2) { dest[0]=xi__compute_y(src[0],src[1],src[2]); dest[1] = 255;    } break;
         XI__CASE(4,1) { dest[0]=xi__compute_y(src[0],src[1],src[2]);                   } break;
         XI__CASE(4,2) { dest[0]=xi__compute_y(src[0],src[1],src[2]); dest[1] = src[3]; } break;
         XI__CASE(4,3) { dest[0]=src[0];dest[1]=src[1];dest[2]=src[2];                    } break;
         default: XI_ASSERT(0); XI_FREE(data); XI_FREE(good); return xi__errpuc("unsupported", "Unsupported format conversion");
      }
      #undef XI__CASE
   }

   XI_FREE(data);
   return good;
}
#endif

#if defined(XI_NO_PNG) && defined(XI_NO_PSD)
// nothing
#else
static xi__uint16 xi__compute_y_16(int r, int g, int b)
{
   return (xi__uint16) (((r*77) + (g*150) +  (29*b)) >> 8);
}
#endif

#if defined(XI_NO_PNG) && defined(XI_NO_PSD)
// nothing
#else
static xi__uint16 *xi__convert_format16(xi__uint16 *data, int img_n, int req_comp, unsigned int x, unsigned int y)
{
   int i,j;
   xi__uint16 *good;

   if (req_comp == img_n) return data;
   XI_ASSERT(req_comp >= 1 && req_comp <= 4);

   good = (xi__uint16 *) xi__malloc(req_comp * x * y * 2);
   if (good == NULL) {
      XI_FREE(data);
      return (xi__uint16 *) xi__errpuc("outofmem", "Out of memory");
   }

   for (j=0; j < (int) y; ++j) {
      xi__uint16 *src  = data + j * x * img_n   ;
      xi__uint16 *dest = good + j * x * req_comp;

      #define XI__COMBO(a,b)  ((a)*8+(b))
      #define XI__CASE(a,b)   case XI__COMBO(a,b): for(i=x-1; i >= 0; --i, src += a, dest += b)
      // convert source image with img_n components to one with req_comp components;
      // avoid switch per pixel, so use switch per scanline and massive macros
      switch (XI__COMBO(img_n, req_comp)) {
         XI__CASE(1,2) { dest[0]=src[0]; dest[1]=0xffff;                                     } break;
         XI__CASE(1,3) { dest[0]=dest[1]=dest[2]=src[0];                                     } break;
         XI__CASE(1,4) { dest[0]=dest[1]=dest[2]=src[0]; dest[3]=0xffff;                     } break;
         XI__CASE(2,1) { dest[0]=src[0];                                                     } break;
         XI__CASE(2,3) { dest[0]=dest[1]=dest[2]=src[0];                                     } break;
         XI__CASE(2,4) { dest[0]=dest[1]=dest[2]=src[0]; dest[3]=src[1];                     } break;
         XI__CASE(3,4) { dest[0]=src[0];dest[1]=src[1];dest[2]=src[2];dest[3]=0xffff;        } break;
         XI__CASE(3,1) { dest[0]=xi__compute_y_16(src[0],src[1],src[2]);                   } break;
         XI__CASE(3,2) { dest[0]=xi__compute_y_16(src[0],src[1],src[2]); dest[1] = 0xffff; } break;
         XI__CASE(4,1) { dest[0]=xi__compute_y_16(src[0],src[1],src[2]);                   } break;
         XI__CASE(4,2) { dest[0]=xi__compute_y_16(src[0],src[1],src[2]); dest[1] = src[3]; } break;
         XI__CASE(4,3) { dest[0]=src[0];dest[1]=src[1];dest[2]=src[2];                       } break;
         default: XI_ASSERT(0); XI_FREE(data); XI_FREE(good); return (xi__uint16*) xi__errpuc("unsupported", "Unsupported format conversion");
      }
      #undef XI__CASE
   }

   XI_FREE(data);
   return good;
}
#endif

#ifndef XI_NO_LINEAR
static float   *xi__ldr_to_hdr(xi_uc *data, int x, int y, int comp)
{
   int i,k,n;
   float *output;
   if (!data) return NULL;
   output = (float *) xi__malloc_mad4(x, y, comp, sizeof(float), 0);
   if (output == NULL) { XI_FREE(data); return xi__errpf("outofmem", "Out of memory"); }
   // compute number of non-alpha components
   if (comp & 1) n = comp; else n = comp-1;
   for (i=0; i < x*y; ++i) {
      for (k=0; k < n; ++k) {
         output[i*comp + k] = (float) (pow(data[i*comp+k]/255.0f, xi__l2h_gamma) * xi__l2h_scale);
      }
   }
   if (n < comp) {
      for (i=0; i < x*y; ++i) {
         output[i*comp + n] = data[i*comp + n]/255.0f;
      }
   }
   XI_FREE(data);
   return output;
}
#endif

#ifndef XI_NO_HDR
#define xi__float2int(x)   ((int) (x))
static xi_uc *xi__hdr_to_ldr(float   *data, int x, int y, int comp)
{
   int i,k,n;
   xi_uc *output;
   if (!data) return NULL;
   output = (xi_uc *) xi__malloc_mad3(x, y, comp, 0);
   if (output == NULL) { XI_FREE(data); return xi__errpuc("outofmem", "Out of memory"); }
   // compute number of non-alpha components
   if (comp & 1) n = comp; else n = comp-1;
   for (i=0; i < x*y; ++i) {
      for (k=0; k < n; ++k) {
         float z = (float) pow(data[i*comp+k]*xi__h2l_scale_i, xi__h2l_gamma_i) * 255 + 0.5f;
         if (z < 0) z = 0;
         if (z > 255) z = 255;
         output[i*comp + k] = (xi_uc) xi__float2int(z);
      }
      if (k < comp) {
         float z = data[i*comp+k] * 255 + 0.5f;
         if (z < 0) z = 0;
         if (z > 255) z = 255;
         output[i*comp + k] = (xi_uc) xi__float2int(z);
      }
   }
   XI_FREE(data);
   return output;
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
//  "baseline" JPEG/JFIF decoder
//
//    simple implementation
//      - doesn't support delayed output of y-dimension
//      - simple interface (only one output format: 8-bit interleaved RGB)
//      - doesn't try to recover corrupt jpegs
//      - doesn't allow partial loading, loading multiple at once
//      - still fast on x86 (copying globals into locals doesn't help x86)
//      - allocates lots of intermediate memory (full size of all components)
//        - non-interleaved case requires this anyway
//        - allows good upsampling (see next)
//    high-quality
//      - upsampled channels are bilinearly interpolated, even across blocks
//      - quality integer IDCT derived from IJG's 'slow'
//    performance
//      - fast huffman; reasonable integer IDCT
//      - some SIMD kernels for common paths on targets with SSE2/NEON
//      - uses a lot of intermediate memory, could cache poorly

#ifndef XI_NO_JPEG

// huffman decoding acceleration
#define FAST_BITS   9  // larger handles more cases; smaller stomps less cache

typedef struct
{
   xi_uc  fast[1 << FAST_BITS];
   // weirdly, repacking this into AoS is a 10% speed loss, instead of a win
   xi__uint16 code[256];
   xi_uc  values[256];
   xi_uc  size[257];
   unsigned int maxcode[18];
   int    delta[17];   // old 'firstsymbol' - old 'firstcode'
} xi__huffman;

typedef struct
{
   xi__context *s;
   xi__huffman huff_dc[4];
   xi__huffman huff_ac[4];
   xi__uint16 dequant[4][64];
   xi__int16 fast_ac[4][1 << FAST_BITS];

// sizes for components, interleaved MCUs
   int img_h_max, img_v_max;
   int img_mcu_x, img_mcu_y;
   int img_mcu_w, img_mcu_h;

// definition of jpeg image component
   struct
   {
      int id;
      int h,v;
      int tq;
      int hd,ha;
      int dc_pred;

      int x,y,w2,h2;
      xi_uc *data;
      void *raw_data, *raw_coeff;
      xi_uc *linebuf;
      short   *coeff;   // progressive only
      int      coeff_w, coeff_h; // number of 8x8 coefficient blocks
   } img_comp[4];

   xi__uint32   code_buffer; // jpeg entropy-coded buffer
   int            code_bits;   // number of valid bits
   unsigned char  marker;      // marker seen while filling entropy buffer
   int            nomore;      // flag if we saw a marker so must stop

   int            progressive;
   int            spec_start;
   int            spec_end;
   int            succ_high;
   int            succ_low;
   int            eob_run;
   int            jfif;
   int            app14_color_transform; // Adobe APP14 tag
   int            rgb;

   int scan_n, order[4];
   int restart_interval, todo;

// kernels
   void (*idct_block_kernel)(xi_uc *out, int out_stride, short data[64]);
   void (*YCbCr_to_RGB_kernel)(xi_uc *out, const xi_uc *y, const xi_uc *pcb, const xi_uc *pcr, int count, int step);
   xi_uc *(*resample_row_hv_2_kernel)(xi_uc *out, xi_uc *in_near, xi_uc *in_far, int w, int hs);
} xi__jpeg;

static int xi__build_huffman(xi__huffman *h, int *count)
{
   int i,j,k=0;
   unsigned int code;
   // build size list for each symbol (from JPEG spec)
   for (i=0; i < 16; ++i) {
      for (j=0; j < count[i]; ++j) {
         h->size[k++] = (xi_uc) (i+1);
         if(k >= 257) return xi__err("bad size list","Corrupt JPEG");
      }
   }
   h->size[k] = 0;

   // compute actual symbols (from jpeg spec)
   code = 0;
   k = 0;
   for(j=1; j <= 16; ++j) {
      // compute delta to add to code to compute symbol id
      h->delta[j] = k - code;
      if (h->size[k] == j) {
         while (h->size[k] == j)
            h->code[k++] = (xi__uint16) (code++);
         if (code-1 >= (1u << j)) return xi__err("bad code lengths","Corrupt JPEG");
      }
      // compute largest code + 1 for this size, preshifted as needed later
      h->maxcode[j] = code << (16-j);
      code <<= 1;
   }
   h->maxcode[j] = 0xffffffff;

   // build non-spec acceleration table; 255 is flag for not-accelerated
   memset(h->fast, 255, 1 << FAST_BITS);
   for (i=0; i < k; ++i) {
      int s = h->size[i];
      if (s <= FAST_BITS) {
         int c = h->code[i] << (FAST_BITS-s);
         int m = 1 << (FAST_BITS-s);
         for (j=0; j < m; ++j) {
            h->fast[c+j] = (xi_uc) i;
         }
      }
   }
   return 1;
}

// build a table that decodes both magnitude and value of small ACs in
// one go.
static void xi__build_fast_ac(xi__int16 *fast_ac, xi__huffman *h)
{
   int i;
   for (i=0; i < (1 << FAST_BITS); ++i) {
      xi_uc fast = h->fast[i];
      fast_ac[i] = 0;
      if (fast < 255) {
         int rs = h->values[fast];
         int run = (rs >> 4) & 15;
         int magbits = rs & 15;
         int len = h->size[fast];

         if (magbits && len + magbits <= FAST_BITS) {
            // magnitude code followed by receive_extend code
            int k = ((i << len) & ((1 << FAST_BITS) - 1)) >> (FAST_BITS - magbits);
            int m = 1 << (magbits - 1);
            if (k < m) k += (~0U << magbits) + 1;
            // if the result is small enough, we can fit it in fast_ac table
            if (k >= -128 && k <= 127)
               fast_ac[i] = (xi__int16) ((k * 256) + (run * 16) + (len + magbits));
         }
      }
   }
}

static void xi__grow_buffer_unsafe(xi__jpeg *j)
{
   do {
      unsigned int b = j->nomore ? 0 : xi__get8(j->s);
      if (b == 0xff) {
         int c = xi__get8(j->s);
         while (c == 0xff) c = xi__get8(j->s); // consume fill bytes
         if (c != 0) {
            j->marker = (unsigned char) c;
            j->nomore = 1;
            return;
         }
      }
      j->code_buffer |= b << (24 - j->code_bits);
      j->code_bits += 8;
   } while (j->code_bits <= 24);
}

// (1 << n) - 1
static const xi__uint32 xi__bmask[17]={0,1,3,7,15,31,63,127,255,511,1023,2047,4095,8191,16383,32767,65535};

// decode a jpeg huffman value from the bitstream
xi_inline static int xi__jpeg_huff_decode(xi__jpeg *j, xi__huffman *h)
{
   unsigned int temp;
   int c,k;

   if (j->code_bits < 16) xi__grow_buffer_unsafe(j);

   // look at the top FAST_BITS and determine what symbol ID it is,
   // if the code is <= FAST_BITS
   c = (j->code_buffer >> (32 - FAST_BITS)) & ((1 << FAST_BITS)-1);
   k = h->fast[c];
   if (k < 255) {
      int s = h->size[k];
      if (s > j->code_bits)
         return -1;
      j->code_buffer <<= s;
      j->code_bits -= s;
      return h->values[k];
   }

   // naive test is to shift the code_buffer down so k bits are
   // valid, then test against maxcode. To speed this up, we've
   // preshifted maxcode left so that it has (16-k) 0s at the
   // end; in other words, regardless of the number of bits, it
   // wants to be compared against something shifted to have 16;
   // that way we don't need to shift inside the loop.
   temp = j->code_buffer >> 16;
   for (k=FAST_BITS+1 ; ; ++k)
      if (temp < h->maxcode[k])
         break;
   if (k == 17) {
      // error! code not found
      j->code_bits -= 16;
      return -1;
   }

   if (k > j->code_bits)
      return -1;

   // convert the huffman code to the symbol id
   c = ((j->code_buffer >> (32 - k)) & xi__bmask[k]) + h->delta[k];
   if(c < 0 || c >= 256) // symbol id out of bounds!
       return -1;
   XI_ASSERT((((j->code_buffer) >> (32 - h->size[c])) & xi__bmask[h->size[c]]) == h->code[c]);

   // convert the id to a symbol
   j->code_bits -= k;
   j->code_buffer <<= k;
   return h->values[c];
}

// bias[n] = (-1<<n) + 1
static const int xi__jbias[16] = {0,-1,-3,-7,-15,-31,-63,-127,-255,-511,-1023,-2047,-4095,-8191,-16383,-32767};

// combined JPEG 'receive' and JPEG 'extend', since baseline
// always extends everything it receives.
xi_inline static int xi__extend_receive(xi__jpeg *j, int n)
{
   unsigned int k;
   int sgn;
   if (j->code_bits < n) xi__grow_buffer_unsafe(j);
   if (j->code_bits < n) return 0; // ran out of bits from stream, return 0s intead of continuing

   sgn = j->code_buffer >> 31; // sign bit always in MSB; 0 if MSB clear (positive), 1 if MSB set (negative)
   k = xi_lrot(j->code_buffer, n);
   j->code_buffer = k & ~xi__bmask[n];
   k &= xi__bmask[n];
   j->code_bits -= n;
   return k + (xi__jbias[n] & (sgn - 1));
}

// get some unsigned bits
xi_inline static int xi__jpeg_get_bits(xi__jpeg *j, int n)
{
   unsigned int k;
   if (j->code_bits < n) xi__grow_buffer_unsafe(j);
   if (j->code_bits < n) return 0; // ran out of bits from stream, return 0s intead of continuing
   k = xi_lrot(j->code_buffer, n);
   j->code_buffer = k & ~xi__bmask[n];
   k &= xi__bmask[n];
   j->code_bits -= n;
   return k;
}

xi_inline static int xi__jpeg_get_bit(xi__jpeg *j)
{
   unsigned int k;
   if (j->code_bits < 1) xi__grow_buffer_unsafe(j);
   if (j->code_bits < 1) return 0; // ran out of bits from stream, return 0s intead of continuing
   k = j->code_buffer;
   j->code_buffer <<= 1;
   --j->code_bits;
   return k & 0x80000000;
}

// given a value that's at position X in the zigzag stream,
// where does it appear in the 8x8 matrix coded as row-major?
static const xi_uc xi__jpeg_dezigzag[64+15] =
{
    0,  1,  8, 16,  9,  2,  3, 10,
   17, 24, 32, 25, 18, 11,  4,  5,
   12, 19, 26, 33, 40, 48, 41, 34,
   27, 20, 13,  6,  7, 14, 21, 28,
   35, 42, 49, 56, 57, 50, 43, 36,
   29, 22, 15, 23, 30, 37, 44, 51,
   58, 59, 52, 45, 38, 31, 39, 46,
   53, 60, 61, 54, 47, 55, 62, 63,
   // let corrupt input sample past end
   63, 63, 63, 63, 63, 63, 63, 63,
   63, 63, 63, 63, 63, 63, 63
};

// decode one 64-entry block--
static int xi__jpeg_decode_block(xi__jpeg *j, short data[64], xi__huffman *hdc, xi__huffman *hac, xi__int16 *fac, int b, xi__uint16 *dequant)
{
   int diff,dc,k;
   int t;

   if (j->code_bits < 16) xi__grow_buffer_unsafe(j);
   t = xi__jpeg_huff_decode(j, hdc);
   if (t < 0 || t > 15) return xi__err("bad huffman code","Corrupt JPEG");

   // 0 all the ac values now so we can do it 32-bits at a time
   memset(data,0,64*sizeof(data[0]));

   diff = t ? xi__extend_receive(j, t) : 0;
   if (!xi__addints_valid(j->img_comp[b].dc_pred, diff)) return xi__err("bad delta","Corrupt JPEG");
   dc = j->img_comp[b].dc_pred + diff;
   j->img_comp[b].dc_pred = dc;
   if (!xi__mul2shorts_valid(dc, dequant[0])) return xi__err("can't merge dc and ac", "Corrupt JPEG");
   data[0] = (short) (dc * dequant[0]);

   // decode AC components, see JPEG spec
   k = 1;
   do {
      unsigned int zig;
      int c,r,s;
      if (j->code_bits < 16) xi__grow_buffer_unsafe(j);
      c = (j->code_buffer >> (32 - FAST_BITS)) & ((1 << FAST_BITS)-1);
      r = fac[c];
      if (r) { // fast-AC path
         k += (r >> 4) & 15; // run
         s = r & 15; // combined length
         if (s > j->code_bits) return xi__err("bad huffman code", "Combined length longer than code bits available");
         j->code_buffer <<= s;
         j->code_bits -= s;
         // decode into unzigzag'd location
         zig = xi__jpeg_dezigzag[k++];
         data[zig] = (short) ((r >> 8) * dequant[zig]);
      } else {
         int rs = xi__jpeg_huff_decode(j, hac);
         if (rs < 0) return xi__err("bad huffman code","Corrupt JPEG");
         s = rs & 15;
         r = rs >> 4;
         if (s == 0) {
            if (rs != 0xf0) break; // end block
            k += 16;
         } else {
            k += r;
            // decode into unzigzag'd location
            zig = xi__jpeg_dezigzag[k++];
            data[zig] = (short) (xi__extend_receive(j,s) * dequant[zig]);
         }
      }
   } while (k < 64);
   return 1;
}

static int xi__jpeg_decode_block_prog_dc(xi__jpeg *j, short data[64], xi__huffman *hdc, int b)
{
   int diff,dc;
   int t;
   if (j->spec_end != 0) return xi__err("can't merge dc and ac", "Corrupt JPEG");

   if (j->code_bits < 16) xi__grow_buffer_unsafe(j);

   if (j->succ_high == 0) {
      // first scan for DC coefficient, must be first
      memset(data,0,64*sizeof(data[0])); // 0 all the ac values now
      t = xi__jpeg_huff_decode(j, hdc);
      if (t < 0 || t > 15) return xi__err("can't merge dc and ac", "Corrupt JPEG");
      diff = t ? xi__extend_receive(j, t) : 0;

      if (!xi__addints_valid(j->img_comp[b].dc_pred, diff)) return xi__err("bad delta", "Corrupt JPEG");
      dc = j->img_comp[b].dc_pred + diff;
      j->img_comp[b].dc_pred = dc;
      if (!xi__mul2shorts_valid(dc, 1 << j->succ_low)) return xi__err("can't merge dc and ac", "Corrupt JPEG");
      data[0] = (short) (dc * (1 << j->succ_low));
   } else {
      // refinement scan for DC coefficient
      if (xi__jpeg_get_bit(j))
         data[0] += (short) (1 << j->succ_low);
   }
   return 1;
}

// @OPTIMIZE: store non-zigzagged during the decode passes,
// and only de-zigzag when dequantizing
static int xi__jpeg_decode_block_prog_ac(xi__jpeg *j, short data[64], xi__huffman *hac, xi__int16 *fac)
{
   int k;
   if (j->spec_start == 0) return xi__err("can't merge dc and ac", "Corrupt JPEG");

   if (j->succ_high == 0) {
      int shift = j->succ_low;

      if (j->eob_run) {
         --j->eob_run;
         return 1;
      }

      k = j->spec_start;
      do {
         unsigned int zig;
         int c,r,s;
         if (j->code_bits < 16) xi__grow_buffer_unsafe(j);
         c = (j->code_buffer >> (32 - FAST_BITS)) & ((1 << FAST_BITS)-1);
         r = fac[c];
         if (r) { // fast-AC path
            k += (r >> 4) & 15; // run
            s = r & 15; // combined length
            if (s > j->code_bits) return xi__err("bad huffman code", "Combined length longer than code bits available");
            j->code_buffer <<= s;
            j->code_bits -= s;
            zig = xi__jpeg_dezigzag[k++];
            data[zig] = (short) ((r >> 8) * (1 << shift));
         } else {
            int rs = xi__jpeg_huff_decode(j, hac);
            if (rs < 0) return xi__err("bad huffman code","Corrupt JPEG");
            s = rs & 15;
            r = rs >> 4;
            if (s == 0) {
               if (r < 15) {
                  j->eob_run = (1 << r);
                  if (r)
                     j->eob_run += xi__jpeg_get_bits(j, r);
                  --j->eob_run;
                  break;
               }
               k += 16;
            } else {
               k += r;
               zig = xi__jpeg_dezigzag[k++];
               data[zig] = (short) (xi__extend_receive(j,s) * (1 << shift));
            }
         }
      } while (k <= j->spec_end);
   } else {
      // refinement scan for these AC coefficients

      short bit = (short) (1 << j->succ_low);

      if (j->eob_run) {
         --j->eob_run;
         for (k = j->spec_start; k <= j->spec_end; ++k) {
            short *p = &data[xi__jpeg_dezigzag[k]];
            if (*p != 0)
               if (xi__jpeg_get_bit(j))
                  if ((*p & bit)==0) {
                     if (*p > 0)
                        *p += bit;
                     else
                        *p -= bit;
                  }
         }
      } else {
         k = j->spec_start;
         do {
            int r,s;
            int rs = xi__jpeg_huff_decode(j, hac); // @OPTIMIZE see if we can use the fast path here, advance-by-r is so slow, eh
            if (rs < 0) return xi__err("bad huffman code","Corrupt JPEG");
            s = rs & 15;
            r = rs >> 4;
            if (s == 0) {
               if (r < 15) {
                  j->eob_run = (1 << r) - 1;
                  if (r)
                     j->eob_run += xi__jpeg_get_bits(j, r);
                  r = 64; // force end of block
               } else {
                  // r=15 s=0 should write 16 0s, so we just do
                  // a run of 15 0s and then write s (which is 0),
                  // so we don't have to do anything special here
               }
            } else {
               if (s != 1) return xi__err("bad huffman code", "Corrupt JPEG");
               // sign bit
               if (xi__jpeg_get_bit(j))
                  s = bit;
               else
                  s = -bit;
            }

            // advance by r
            while (k <= j->spec_end) {
               short *p = &data[xi__jpeg_dezigzag[k++]];
               if (*p != 0) {
                  if (xi__jpeg_get_bit(j))
                     if ((*p & bit)==0) {
                        if (*p > 0)
                           *p += bit;
                        else
                           *p -= bit;
                     }
               } else {
                  if (r == 0) {
                     *p = (short) s;
                     break;
                  }
                  --r;
               }
            }
         } while (k <= j->spec_end);
      }
   }
   return 1;
}

// take a -128..127 value and xi__clamp it and convert to 0..255
xi_inline static xi_uc xi__clamp(int x)
{
   // trick to use a single test to catch both cases
   if ((unsigned int) x > 255) {
      if (x < 0) return 0;
      if (x > 255) return 255;
   }
   return (xi_uc) x;
}

#define xi__f2f(x)  ((int) (((x) * 4096 + 0.5)))
#define xi__fsh(x)  ((x) * 4096)

// derived from jidctint -- DCT_ISLOW
#define XI__IDCT_1D(s0,s1,s2,s3,s4,s5,s6,s7) \
   int t0,t1,t2,t3,p1,p2,p3,p4,p5,x0,x1,x2,x3; \
   p2 = s2;                                    \
   p3 = s6;                                    \
   p1 = (p2+p3) * xi__f2f(0.5411961f);       \
   t2 = p1 + p3*xi__f2f(-1.847759065f);      \
   t3 = p1 + p2*xi__f2f( 0.765366865f);      \
   p2 = s0;                                    \
   p3 = s4;                                    \
   t0 = xi__fsh(p2+p3);                      \
   t1 = xi__fsh(p2-p3);                      \
   x0 = t0+t3;                                 \
   x3 = t0-t3;                                 \
   x1 = t1+t2;                                 \
   x2 = t1-t2;                                 \
   t0 = s7;                                    \
   t1 = s5;                                    \
   t2 = s3;                                    \
   t3 = s1;                                    \
   p3 = t0+t2;                                 \
   p4 = t1+t3;                                 \
   p1 = t0+t3;                                 \
   p2 = t1+t2;                                 \
   p5 = (p3+p4)*xi__f2f( 1.175875602f);      \
   t0 = t0*xi__f2f( 0.298631336f);           \
   t1 = t1*xi__f2f( 2.053119869f);           \
   t2 = t2*xi__f2f( 3.072711026f);           \
   t3 = t3*xi__f2f( 1.501321110f);           \
   p1 = p5 + p1*xi__f2f(-0.899976223f);      \
   p2 = p5 + p2*xi__f2f(-2.562915447f);      \
   p3 = p3*xi__f2f(-1.961570560f);           \
   p4 = p4*xi__f2f(-0.390180644f);           \
   t3 += p1+p4;                                \
   t2 += p2+p3;                                \
   t1 += p2+p4;                                \
   t0 += p1+p3;

static void xi__idct_block(xi_uc *out, int out_stride, short data[64])
{
   int i,val[64],*v=val;
   xi_uc *o;
   short *d = data;

   // columns
   for (i=0; i < 8; ++i,++d, ++v) {
      // if all zeroes, shortcut -- this avoids dequantizing 0s and IDCTing
      if (d[ 8]==0 && d[16]==0 && d[24]==0 && d[32]==0
           && d[40]==0 && d[48]==0 && d[56]==0) {
         //    no shortcut                 0     seconds
         //    (1|2|3|4|5|6|7)==0          0     seconds
         //    all separate               -0.047 seconds
         //    1 && 2|3 && 4|5 && 6|7:    -0.047 seconds
         int dcterm = d[0]*4;
         v[0] = v[8] = v[16] = v[24] = v[32] = v[40] = v[48] = v[56] = dcterm;
      } else {
         XI__IDCT_1D(d[ 0],d[ 8],d[16],d[24],d[32],d[40],d[48],d[56])
         // constants scaled things up by 1<<12; let's bring them back
         // down, but keep 2 extra bits of precision
         x0 += 512; x1 += 512; x2 += 512; x3 += 512;
         v[ 0] = (x0+t3) >> 10;
         v[56] = (x0-t3) >> 10;
         v[ 8] = (x1+t2) >> 10;
         v[48] = (x1-t2) >> 10;
         v[16] = (x2+t1) >> 10;
         v[40] = (x2-t1) >> 10;
         v[24] = (x3+t0) >> 10;
         v[32] = (x3-t0) >> 10;
      }
   }

   for (i=0, v=val, o=out; i < 8; ++i,v+=8,o+=out_stride) {
      // no fast case since the first 1D IDCT spread components out
      XI__IDCT_1D(v[0],v[1],v[2],v[3],v[4],v[5],v[6],v[7])
      // constants scaled things up by 1<<12, plus we had 1<<2 from first
      // loop, plus horizontal and vertical each scale by sqrt(8) so together
      // we've got an extra 1<<3, so 1<<17 total we need to remove.
      // so we want to round that, which means adding 0.5 * 1<<17,
      // aka 65536. Also, we'll end up with -128 to 127 that we want
      // to encode as 0..255 by adding 128, so we'll add that before the shift
      x0 += 65536 + (128<<17);
      x1 += 65536 + (128<<17);
      x2 += 65536 + (128<<17);
      x3 += 65536 + (128<<17);
      // tried computing the shifts into temps, or'ing the temps to see
      // if any were out of range, but that was slower
      o[0] = xi__clamp((x0+t3) >> 17);
      o[7] = xi__clamp((x0-t3) >> 17);
      o[1] = xi__clamp((x1+t2) >> 17);
      o[6] = xi__clamp((x1-t2) >> 17);
      o[2] = xi__clamp((x2+t1) >> 17);
      o[5] = xi__clamp((x2-t1) >> 17);
      o[3] = xi__clamp((x3+t0) >> 17);
      o[4] = xi__clamp((x3-t0) >> 17);
   }
}

#ifdef XI_SSE2
// sse2 integer IDCT. not the fastest possible implementation but it
// produces bit-identical results to the generic C version so it's
// fully "transparent".
static void xi__idct_simd(xi_uc *out, int out_stride, short data[64])
{
   // This is constructed to match our regular (generic) integer IDCT exactly.
   __m128i row0, row1, row2, row3, row4, row5, row6, row7;
   __m128i tmp;

   // dot product constant: even elems=x, odd elems=y
   #define dct_const(x,y)  _mm_setr_epi16((x),(y),(x),(y),(x),(y),(x),(y))

   // out(0) = c0[even]*x + c0[odd]*y   (c0, x, y 16-bit, out 32-bit)
   // out(1) = c1[even]*x + c1[odd]*y
   #define dct_rot(out0,out1, x,y,c0,c1) \
      __m128i c0##lo = _mm_unpacklo_epi16((x),(y)); \
      __m128i c0##hi = _mm_unpackhi_epi16((x),(y)); \
      __m128i out0##_l = _mm_madd_epi16(c0##lo, c0); \
      __m128i out0##_h = _mm_madd_epi16(c0##hi, c0); \
      __m128i out1##_l = _mm_madd_epi16(c0##lo, c1); \
      __m128i out1##_h = _mm_madd_epi16(c0##hi, c1)

   // out = in << 12  (in 16-bit, out 32-bit)
   #define dct_widen(out, in) \
      __m128i out##_l = _mm_srai_epi32(_mm_unpacklo_epi16(_mm_setzero_si128(), (in)), 4); \
      __m128i out##_h = _mm_srai_epi32(_mm_unpackhi_epi16(_mm_setzero_si128(), (in)), 4)

   // wide add
   #define dct_wadd(out, a, b) \
      __m128i out##_l = _mm_add_epi32(a##_l, b##_l); \
      __m128i out##_h = _mm_add_epi32(a##_h, b##_h)

   // wide sub
   #define dct_wsub(out, a, b) \
      __m128i out##_l = _mm_sub_epi32(a##_l, b##_l); \
      __m128i out##_h = _mm_sub_epi32(a##_h, b##_h)

   // butterfly a/b, add bias, then shift by "s" and pack
   #define dct_bfly32o(out0, out1, a,b,bias,s) \
      { \
         __m128i abiased_l = _mm_add_epi32(a##_l, bias); \
         __m128i abiased_h = _mm_add_epi32(a##_h, bias); \
         dct_wadd(sum, abiased, b); \
         dct_wsub(dif, abiased, b); \
         out0 = _mm_packs_epi32(_mm_srai_epi32(sum_l, s), _mm_srai_epi32(sum_h, s)); \
         out1 = _mm_packs_epi32(_mm_srai_epi32(dif_l, s), _mm_srai_epi32(dif_h, s)); \
      }

   // 8-bit interleave step (for transposes)
   #define dct_interleave8(a, b) \
      tmp = a; \
      a = _mm_unpacklo_epi8(a, b); \
      b = _mm_unpackhi_epi8(tmp, b)

   // 16-bit interleave step (for transposes)
   #define dct_interleave16(a, b) \
      tmp = a; \
      a = _mm_unpacklo_epi16(a, b); \
      b = _mm_unpackhi_epi16(tmp, b)

   #define dct_pass(bias,shift) \
      { \
         /* even part */ \
         dct_rot(t2e,t3e, row2,row6, rot0_0,rot0_1); \
         __m128i sum04 = _mm_add_epi16(row0, row4); \
         __m128i dif04 = _mm_sub_epi16(row0, row4); \
         dct_widen(t0e, sum04); \
         dct_widen(t1e, dif04); \
         dct_wadd(x0, t0e, t3e); \
         dct_wsub(x3, t0e, t3e); \
         dct_wadd(x1, t1e, t2e); \
         dct_wsub(x2, t1e, t2e); \
         /* odd part */ \
         dct_rot(y0o,y2o, row7,row3, rot2_0,rot2_1); \
         dct_rot(y1o,y3o, row5,row1, rot3_0,rot3_1); \
         __m128i sum17 = _mm_add_epi16(row1, row7); \
         __m128i sum35 = _mm_add_epi16(row3, row5); \
         dct_rot(y4o,y5o, sum17,sum35, rot1_0,rot1_1); \
         dct_wadd(x4, y0o, y4o); \
         dct_wadd(x5, y1o, y5o); \
         dct_wadd(x6, y2o, y5o); \
         dct_wadd(x7, y3o, y4o); \
         dct_bfly32o(row0,row7, x0,x7,bias,shift); \
         dct_bfly32o(row1,row6, x1,x6,bias,shift); \
         dct_bfly32o(row2,row5, x2,x5,bias,shift); \
         dct_bfly32o(row3,row4, x3,x4,bias,shift); \
      }

   __m128i rot0_0 = dct_const(xi__f2f(0.5411961f), xi__f2f(0.5411961f) + xi__f2f(-1.847759065f));
   __m128i rot0_1 = dct_const(xi__f2f(0.5411961f) + xi__f2f( 0.765366865f), xi__f2f(0.5411961f));
   __m128i rot1_0 = dct_const(xi__f2f(1.175875602f) + xi__f2f(-0.899976223f), xi__f2f(1.175875602f));
   __m128i rot1_1 = dct_const(xi__f2f(1.175875602f), xi__f2f(1.175875602f) + xi__f2f(-2.562915447f));
   __m128i rot2_0 = dct_const(xi__f2f(-1.961570560f) + xi__f2f( 0.298631336f), xi__f2f(-1.961570560f));
   __m128i rot2_1 = dct_const(xi__f2f(-1.961570560f), xi__f2f(-1.961570560f) + xi__f2f( 3.072711026f));
   __m128i rot3_0 = dct_const(xi__f2f(-0.390180644f) + xi__f2f( 2.053119869f), xi__f2f(-0.390180644f));
   __m128i rot3_1 = dct_const(xi__f2f(-0.390180644f), xi__f2f(-0.390180644f) + xi__f2f( 1.501321110f));

   // rounding biases in column/row passes, see xi__idct_block for explanation.
   __m128i bias_0 = _mm_set1_epi32(512);
   __m128i bias_1 = _mm_set1_epi32(65536 + (128<<17));

   // load
   row0 = _mm_load_si128((const __m128i *) (data + 0*8));
   row1 = _mm_load_si128((const __m128i *) (data + 1*8));
   row2 = _mm_load_si128((const __m128i *) (data + 2*8));
   row3 = _mm_load_si128((const __m128i *) (data + 3*8));
   row4 = _mm_load_si128((const __m128i *) (data + 4*8));
   row5 = _mm_load_si128((const __m128i *) (data + 5*8));
   row6 = _mm_load_si128((const __m128i *) (data + 6*8));
   row7 = _mm_load_si128((const __m128i *) (data + 7*8));

   // column pass
   dct_pass(bias_0, 10);

   {
      // 16bit 8x8 transpose pass 1
      dct_interleave16(row0, row4);
      dct_interleave16(row1, row5);
      dct_interleave16(row2, row6);
      dct_interleave16(row3, row7);

      // transpose pass 2
      dct_interleave16(row0, row2);
      dct_interleave16(row1, row3);
      dct_interleave16(row4, row6);
      dct_interleave16(row5, row7);

      // transpose pass 3
      dct_interleave16(row0, row1);
      dct_interleave16(row2, row3);
      dct_interleave16(row4, row5);
      dct_interleave16(row6, row7);
   }

   // row pass
   dct_pass(bias_1, 17);

   {
      // pack
      __m128i p0 = _mm_packus_epi16(row0, row1); // a0a1a2a3...a7b0b1b2b3...b7
      __m128i p1 = _mm_packus_epi16(row2, row3);
      __m128i p2 = _mm_packus_epi16(row4, row5);
      __m128i p3 = _mm_packus_epi16(row6, row7);

      // 8bit 8x8 transpose pass 1
      dct_interleave8(p0, p2); // a0e0a1e1...
      dct_interleave8(p1, p3); // c0g0c1g1...

      // transpose pass 2
      dct_interleave8(p0, p1); // a0c0e0g0...
      dct_interleave8(p2, p3); // b0d0f0h0...

      // transpose pass 3
      dct_interleave8(p0, p2); // a0b0c0d0...
      dct_interleave8(p1, p3); // a4b4c4d4...

      // store
      _mm_storel_epi64((__m128i *) out, p0); out += out_stride;
      _mm_storel_epi64((__m128i *) out, _mm_shuffle_epi32(p0, 0x4e)); out += out_stride;
      _mm_storel_epi64((__m128i *) out, p2); out += out_stride;
      _mm_storel_epi64((__m128i *) out, _mm_shuffle_epi32(p2, 0x4e)); out += out_stride;
      _mm_storel_epi64((__m128i *) out, p1); out += out_stride;
      _mm_storel_epi64((__m128i *) out, _mm_shuffle_epi32(p1, 0x4e)); out += out_stride;
      _mm_storel_epi64((__m128i *) out, p3); out += out_stride;
      _mm_storel_epi64((__m128i *) out, _mm_shuffle_epi32(p3, 0x4e));
   }

#undef dct_const
#undef dct_rot
#undef dct_widen
#undef dct_wadd
#undef dct_wsub
#undef dct_bfly32o
#undef dct_interleave8
#undef dct_interleave16
#undef dct_pass
}

#endif // XI_SSE2

#ifdef XI_NEON

// NEON integer IDCT. should produce bit-identical
// results to the generic C version.
static void xi__idct_simd(xi_uc *out, int out_stride, short data[64])
{
   int16x8_t row0, row1, row2, row3, row4, row5, row6, row7;

   int16x4_t rot0_0 = vdup_n_s16(xi__f2f(0.5411961f));
   int16x4_t rot0_1 = vdup_n_s16(xi__f2f(-1.847759065f));
   int16x4_t rot0_2 = vdup_n_s16(xi__f2f( 0.765366865f));
   int16x4_t rot1_0 = vdup_n_s16(xi__f2f( 1.175875602f));
   int16x4_t rot1_1 = vdup_n_s16(xi__f2f(-0.899976223f));
   int16x4_t rot1_2 = vdup_n_s16(xi__f2f(-2.562915447f));
   int16x4_t rot2_0 = vdup_n_s16(xi__f2f(-1.961570560f));
   int16x4_t rot2_1 = vdup_n_s16(xi__f2f(-0.390180644f));
   int16x4_t rot3_0 = vdup_n_s16(xi__f2f( 0.298631336f));
   int16x4_t rot3_1 = vdup_n_s16(xi__f2f( 2.053119869f));
   int16x4_t rot3_2 = vdup_n_s16(xi__f2f( 3.072711026f));
   int16x4_t rot3_3 = vdup_n_s16(xi__f2f( 1.501321110f));

#define dct_long_mul(out, inq, coeff) \
   int32x4_t out##_l = vmull_s16(vget_low_s16(inq), coeff); \
   int32x4_t out##_h = vmull_s16(vget_high_s16(inq), coeff)

#define dct_long_mac(out, acc, inq, coeff) \
   int32x4_t out##_l = vmlal_s16(acc##_l, vget_low_s16(inq), coeff); \
   int32x4_t out##_h = vmlal_s16(acc##_h, vget_high_s16(inq), coeff)

#define dct_widen(out, inq) \
   int32x4_t out##_l = vshll_n_s16(vget_low_s16(inq), 12); \
   int32x4_t out##_h = vshll_n_s16(vget_high_s16(inq), 12)

// wide add
#define dct_wadd(out, a, b) \
   int32x4_t out##_l = vaddq_s32(a##_l, b##_l); \
   int32x4_t out##_h = vaddq_s32(a##_h, b##_h)

// wide sub
#define dct_wsub(out, a, b) \
   int32x4_t out##_l = vsubq_s32(a##_l, b##_l); \
   int32x4_t out##_h = vsubq_s32(a##_h, b##_h)

// butterfly a/b, then shift using "shiftop" by "s" and pack
#define dct_bfly32o(out0,out1, a,b,shiftop,s) \
   { \
      dct_wadd(sum, a, b); \
      dct_wsub(dif, a, b); \
      out0 = vcombine_s16(shiftop(sum_l, s), shiftop(sum_h, s)); \
      out1 = vcombine_s16(shiftop(dif_l, s), shiftop(dif_h, s)); \
   }

#define dct_pass(shiftop, shift) \
   { \
      /* even part */ \
      int16x8_t sum26 = vaddq_s16(row2, row6); \
      dct_long_mul(p1e, sum26, rot0_0); \
      dct_long_mac(t2e, p1e, row6, rot0_1); \
      dct_long_mac(t3e, p1e, row2, rot0_2); \
      int16x8_t sum04 = vaddq_s16(row0, row4); \
      int16x8_t dif04 = vsubq_s16(row0, row4); \
      dct_widen(t0e, sum04); \
      dct_widen(t1e, dif04); \
      dct_wadd(x0, t0e, t3e); \
      dct_wsub(x3, t0e, t3e); \
      dct_wadd(x1, t1e, t2e); \
      dct_wsub(x2, t1e, t2e); \
      /* odd part */ \
      int16x8_t sum15 = vaddq_s16(row1, row5); \
      int16x8_t sum17 = vaddq_s16(row1, row7); \
      int16x8_t sum35 = vaddq_s16(row3, row5); \
      int16x8_t sum37 = vaddq_s16(row3, row7); \
      int16x8_t sumodd = vaddq_s16(sum17, sum35); \
      dct_long_mul(p5o, sumodd, rot1_0); \
      dct_long_mac(p1o, p5o, sum17, rot1_1); \
      dct_long_mac(p2o, p5o, sum35, rot1_2); \
      dct_long_mul(p3o, sum37, rot2_0); \
      dct_long_mul(p4o, sum15, rot2_1); \
      dct_wadd(sump13o, p1o, p3o); \
      dct_wadd(sump24o, p2o, p4o); \
      dct_wadd(sump23o, p2o, p3o); \
      dct_wadd(sump14o, p1o, p4o); \
      dct_long_mac(x4, sump13o, row7, rot3_0); \
      dct_long_mac(x5, sump24o, row5, rot3_1); \
      dct_long_mac(x6, sump23o, row3, rot3_2); \
      dct_long_mac(x7, sump14o, row1, rot3_3); \
      dct_bfly32o(row0,row7, x0,x7,shiftop,shift); \
      dct_bfly32o(row1,row6, x1,x6,shiftop,shift); \
      dct_bfly32o(row2,row5, x2,x5,shiftop,shift); \
      dct_bfly32o(row3,row4, x3,x4,shiftop,shift); \
   }

   // load
   row0 = vld1q_s16(data + 0*8);
   row1 = vld1q_s16(data + 1*8);
   row2 = vld1q_s16(data + 2*8);
   row3 = vld1q_s16(data + 3*8);
   row4 = vld1q_s16(data + 4*8);
   row5 = vld1q_s16(data + 5*8);
   row6 = vld1q_s16(data + 6*8);
   row7 = vld1q_s16(data + 7*8);

   // add DC bias
   row0 = vaddq_s16(row0, vsetq_lane_s16(1024, vdupq_n_s16(0), 0));

   // column pass
   dct_pass(vrshrn_n_s32, 10);

   // 16bit 8x8 transpose
   {
// these three map to a single VTRN.16, VTRN.32, and VSWP, respectively.
// whether compilers actually get this is another story, sadly.
#define dct_trn16(x, y) { int16x8x2_t t = vtrnq_s16(x, y); x = t.val[0]; y = t.val[1]; }
#define dct_trn32(x, y) { int32x4x2_t t = vtrnq_s32(vreinterpretq_s32_s16(x), vreinterpretq_s32_s16(y)); x = vreinterpretq_s16_s32(t.val[0]); y = vreinterpretq_s16_s32(t.val[1]); }
#define dct_trn64(x, y) { int16x8_t x0 = x; int16x8_t y0 = y; x = vcombine_s16(vget_low_s16(x0), vget_low_s16(y0)); y = vcombine_s16(vget_high_s16(x0), vget_high_s16(y0)); }

      // pass 1
      dct_trn16(row0, row1); // a0b0a2b2a4b4a6b6
      dct_trn16(row2, row3);
      dct_trn16(row4, row5);
      dct_trn16(row6, row7);

      // pass 2
      dct_trn32(row0, row2); // a0b0c0d0a4b4c4d4
      dct_trn32(row1, row3);
      dct_trn32(row4, row6);
      dct_trn32(row5, row7);

      // pass 3
      dct_trn64(row0, row4); // a0b0c0d0e0f0g0h0
      dct_trn64(row1, row5);
      dct_trn64(row2, row6);
      dct_trn64(row3, row7);

#undef dct_trn16
#undef dct_trn32
#undef dct_trn64
   }

   // row pass
   // vrshrn_n_s32 only supports shifts up to 16, we need
   // 17. so do a non-rounding shift of 16 first then follow
   // up with a rounding shift by 1.
   dct_pass(vshrn_n_s32, 16);

   {
      // pack and round
      uint8x8_t p0 = vqrshrun_n_s16(row0, 1);
      uint8x8_t p1 = vqrshrun_n_s16(row1, 1);
      uint8x8_t p2 = vqrshrun_n_s16(row2, 1);
      uint8x8_t p3 = vqrshrun_n_s16(row3, 1);
      uint8x8_t p4 = vqrshrun_n_s16(row4, 1);
      uint8x8_t p5 = vqrshrun_n_s16(row5, 1);
      uint8x8_t p6 = vqrshrun_n_s16(row6, 1);
      uint8x8_t p7 = vqrshrun_n_s16(row7, 1);

      // again, these can translate into one instruction, but often don't.
#define dct_trn8_8(x, y) { uint8x8x2_t t = vtrn_u8(x, y); x = t.val[0]; y = t.val[1]; }
#define dct_trn8_16(x, y) { uint16x4x2_t t = vtrn_u16(vreinterpret_u16_u8(x), vreinterpret_u16_u8(y)); x = vreinterpret_u8_u16(t.val[0]); y = vreinterpret_u8_u16(t.val[1]); }
#define dct_trn8_32(x, y) { uint32x2x2_t t = vtrn_u32(vreinterpret_u32_u8(x), vreinterpret_u32_u8(y)); x = vreinterpret_u8_u32(t.val[0]); y = vreinterpret_u8_u32(t.val[1]); }

      // sadly can't use interleaved stores here since we only write
      // 8 bytes to each scan line!

      // 8x8 8-bit transpose pass 1
      dct_trn8_8(p0, p1);
      dct_trn8_8(p2, p3);
      dct_trn8_8(p4, p5);
      dct_trn8_8(p6, p7);

      // pass 2
      dct_trn8_16(p0, p2);
      dct_trn8_16(p1, p3);
      dct_trn8_16(p4, p6);
      dct_trn8_16(p5, p7);

      // pass 3
      dct_trn8_32(p0, p4);
      dct_trn8_32(p1, p5);
      dct_trn8_32(p2, p6);
      dct_trn8_32(p3, p7);

      // store
      vst1_u8(out, p0); out += out_stride;
      vst1_u8(out, p1); out += out_stride;
      vst1_u8(out, p2); out += out_stride;
      vst1_u8(out, p3); out += out_stride;
      vst1_u8(out, p4); out += out_stride;
      vst1_u8(out, p5); out += out_stride;
      vst1_u8(out, p6); out += out_stride;
      vst1_u8(out, p7);

#undef dct_trn8_8
#undef dct_trn8_16
#undef dct_trn8_32
   }

#undef dct_long_mul
#undef dct_long_mac
#undef dct_widen
#undef dct_wadd
#undef dct_wsub
#undef dct_bfly32o
#undef dct_pass
}

#endif // XI_NEON

#define XI__MARKER_none  0xff
// if there's a pending marker from the entropy stream, return that
// otherwise, fetch from the stream and get a marker. if there's no
// marker, return 0xff, which is never a valid marker value
static xi_uc xi__get_marker(xi__jpeg *j)
{
   xi_uc x;
   if (j->marker != XI__MARKER_none) { x = j->marker; j->marker = XI__MARKER_none; return x; }
   x = xi__get8(j->s);
   if (x != 0xff) return XI__MARKER_none;
   while (x == 0xff)
      x = xi__get8(j->s); // consume repeated 0xff fill bytes
   return x;
}

// in each scan, we'll have scan_n components, and the order
// of the components is specified by order[]
#define XI__RESTART(x)     ((x) >= 0xd0 && (x) <= 0xd7)

// after a restart interval, xi__jpeg_reset the entropy decoder and
// the dc prediction
static void xi__jpeg_reset(xi__jpeg *j)
{
   j->code_bits = 0;
   j->code_buffer = 0;
   j->nomore = 0;
   j->img_comp[0].dc_pred = j->img_comp[1].dc_pred = j->img_comp[2].dc_pred = j->img_comp[3].dc_pred = 0;
   j->marker = XI__MARKER_none;
   j->todo = j->restart_interval ? j->restart_interval : 0x7fffffff;
   j->eob_run = 0;
   // no more than 1<<31 MCUs if no restart_interal? that's plenty safe,
   // since we don't even allow 1<<30 pixels
}

static int xi__parse_entropy_coded_data(xi__jpeg *z)
{
   xi__jpeg_reset(z);
   if (!z->progressive) {
      if (z->scan_n == 1) {
         int i,j;
         XI_SIMD_ALIGN(short, data[64]);
         int n = z->order[0];
         // non-interleaved data, we just need to process one block at a time,
         // in trivial scanline order
         // number of blocks to do just depends on how many actual "pixels" this
         // component has, independent of interleaved MCU blocking and such
         int w = (z->img_comp[n].x+7) >> 3;
         int h = (z->img_comp[n].y+7) >> 3;
         for (j=0; j < h; ++j) {
            for (i=0; i < w; ++i) {
               int ha = z->img_comp[n].ha;
               if (!xi__jpeg_decode_block(z, data, z->huff_dc+z->img_comp[n].hd, z->huff_ac+ha, z->fast_ac[ha], n, z->dequant[z->img_comp[n].tq])) return 0;
               z->idct_block_kernel(z->img_comp[n].data+z->img_comp[n].w2*j*8+i*8, z->img_comp[n].w2, data);
               // every data block is an MCU, so countdown the restart interval
               if (--z->todo <= 0) {
                  if (z->code_bits < 24) xi__grow_buffer_unsafe(z);
                  // if it's NOT a restart, then just bail, so we get corrupt data
                  // rather than no data
                  if (!XI__RESTART(z->marker)) return 1;
                  xi__jpeg_reset(z);
               }
            }
         }
         return 1;
      } else { // interleaved
         int i,j,k,x,y;
         XI_SIMD_ALIGN(short, data[64]);
         for (j=0; j < z->img_mcu_y; ++j) {
            for (i=0; i < z->img_mcu_x; ++i) {
               // scan an interleaved mcu... process scan_n components in order
               for (k=0; k < z->scan_n; ++k) {
                  int n = z->order[k];
                  // scan out an mcu's worth of this component; that's just determined
                  // by the basic H and V specified for the component
                  for (y=0; y < z->img_comp[n].v; ++y) {
                     for (x=0; x < z->img_comp[n].h; ++x) {
                        int x2 = (i*z->img_comp[n].h + x)*8;
                        int y2 = (j*z->img_comp[n].v + y)*8;
                        int ha = z->img_comp[n].ha;
                        if (!xi__jpeg_decode_block(z, data, z->huff_dc+z->img_comp[n].hd, z->huff_ac+ha, z->fast_ac[ha], n, z->dequant[z->img_comp[n].tq])) return 0;
                        z->idct_block_kernel(z->img_comp[n].data+z->img_comp[n].w2*y2+x2, z->img_comp[n].w2, data);
                     }
                  }
               }
               // after all interleaved components, that's an interleaved MCU,
               // so now count down the restart interval
               if (--z->todo <= 0) {
                  if (z->code_bits < 24) xi__grow_buffer_unsafe(z);
                  if (!XI__RESTART(z->marker)) return 1;
                  xi__jpeg_reset(z);
               }
            }
         }
         return 1;
      }
   } else {
      if (z->scan_n == 1) {
         int i,j;
         int n = z->order[0];
         // non-interleaved data, we just need to process one block at a time,
         // in trivial scanline order
         // number of blocks to do just depends on how many actual "pixels" this
         // component has, independent of interleaved MCU blocking and such
         int w = (z->img_comp[n].x+7) >> 3;
         int h = (z->img_comp[n].y+7) >> 3;
         for (j=0; j < h; ++j) {
            for (i=0; i < w; ++i) {
               short *data = z->img_comp[n].coeff + 64 * (i + j * z->img_comp[n].coeff_w);
               if (z->spec_start == 0) {
                  if (!xi__jpeg_decode_block_prog_dc(z, data, &z->huff_dc[z->img_comp[n].hd], n))
                     return 0;
               } else {
                  int ha = z->img_comp[n].ha;
                  if (!xi__jpeg_decode_block_prog_ac(z, data, &z->huff_ac[ha], z->fast_ac[ha]))
                     return 0;
               }
               // every data block is an MCU, so countdown the restart interval
               if (--z->todo <= 0) {
                  if (z->code_bits < 24) xi__grow_buffer_unsafe(z);
                  if (!XI__RESTART(z->marker)) return 1;
                  xi__jpeg_reset(z);
               }
            }
         }
         return 1;
      } else { // interleaved
         int i,j,k,x,y;
         for (j=0; j < z->img_mcu_y; ++j) {
            for (i=0; i < z->img_mcu_x; ++i) {
               // scan an interleaved mcu... process scan_n components in order
               for (k=0; k < z->scan_n; ++k) {
                  int n = z->order[k];
                  // scan out an mcu's worth of this component; that's just determined
                  // by the basic H and V specified for the component
                  for (y=0; y < z->img_comp[n].v; ++y) {
                     for (x=0; x < z->img_comp[n].h; ++x) {
                        int x2 = (i*z->img_comp[n].h + x);
                        int y2 = (j*z->img_comp[n].v + y);
                        short *data = z->img_comp[n].coeff + 64 * (x2 + y2 * z->img_comp[n].coeff_w);
                        if (!xi__jpeg_decode_block_prog_dc(z, data, &z->huff_dc[z->img_comp[n].hd], n))
                           return 0;
                     }
                  }
               }
               // after all interleaved components, that's an interleaved MCU,
               // so now count down the restart interval
               if (--z->todo <= 0) {
                  if (z->code_bits < 24) xi__grow_buffer_unsafe(z);
                  if (!XI__RESTART(z->marker)) return 1;
                  xi__jpeg_reset(z);
               }
            }
         }
         return 1;
      }
   }
}

static void xi__jpeg_dequantize(short *data, xi__uint16 *dequant)
{
   int i;
   for (i=0; i < 64; ++i)
      data[i] *= dequant[i];
}

static void xi__jpeg_finish(xi__jpeg *z)
{
   if (z->progressive) {
      // dequantize and idct the data
      int i,j,n;
      for (n=0; n < z->s->img_n; ++n) {
         int w = (z->img_comp[n].x+7) >> 3;
         int h = (z->img_comp[n].y+7) >> 3;
         for (j=0; j < h; ++j) {
            for (i=0; i < w; ++i) {
               short *data = z->img_comp[n].coeff + 64 * (i + j * z->img_comp[n].coeff_w);
               xi__jpeg_dequantize(data, z->dequant[z->img_comp[n].tq]);
               z->idct_block_kernel(z->img_comp[n].data+z->img_comp[n].w2*j*8+i*8, z->img_comp[n].w2, data);
            }
         }
      }
   }
}

static int xi__process_marker(xi__jpeg *z, int m)
{
   int L;
   switch (m) {
      case XI__MARKER_none: // no marker found
         return xi__err("expected marker","Corrupt JPEG");

      case 0xDD: // DRI - specify restart interval
         if (xi__get16be(z->s) != 4) return xi__err("bad DRI len","Corrupt JPEG");
         z->restart_interval = xi__get16be(z->s);
         return 1;

      case 0xDB: // DQT - define quantization table
         L = xi__get16be(z->s)-2;
         while (L > 0) {
            int q = xi__get8(z->s);
            int p = q >> 4, sixteen = (p != 0);
            int t = q & 15,i;
            if (p != 0 && p != 1) return xi__err("bad DQT type","Corrupt JPEG");
            if (t > 3) return xi__err("bad DQT table","Corrupt JPEG");

            for (i=0; i < 64; ++i)
               z->dequant[t][xi__jpeg_dezigzag[i]] = (xi__uint16)(sixteen ? xi__get16be(z->s) : xi__get8(z->s));
            L -= (sixteen ? 129 : 65);
         }
         return L==0;

      case 0xC4: // DHT - define huffman table
         L = xi__get16be(z->s)-2;
         while (L > 0) {
            xi_uc *v;
            int sizes[16],i,n=0;
            int q = xi__get8(z->s);
            int tc = q >> 4;
            int th = q & 15;
            if (tc > 1 || th > 3) return xi__err("bad DHT header","Corrupt JPEG");
            for (i=0; i < 16; ++i) {
               sizes[i] = xi__get8(z->s);
               n += sizes[i];
            }
            if(n > 256) return xi__err("bad DHT header","Corrupt JPEG"); // Loop over i < n would write past end of values!
            L -= 17;
            if (tc == 0) {
               if (!xi__build_huffman(z->huff_dc+th, sizes)) return 0;
               v = z->huff_dc[th].values;
            } else {
               if (!xi__build_huffman(z->huff_ac+th, sizes)) return 0;
               v = z->huff_ac[th].values;
            }
            for (i=0; i < n; ++i)
               v[i] = xi__get8(z->s);
            if (tc != 0)
               xi__build_fast_ac(z->fast_ac[th], z->huff_ac + th);
            L -= n;
         }
         return L==0;
   }

   // check for comment block or APP blocks
   if ((m >= 0xE0 && m <= 0xEF) || m == 0xFE) {
      L = xi__get16be(z->s);
      if (L < 2) {
         if (m == 0xFE)
            return xi__err("bad COM len","Corrupt JPEG");
         else
            return xi__err("bad APP len","Corrupt JPEG");
      }
      L -= 2;

      if (m == 0xE0 && L >= 5) { // JFIF APP0 segment
         static const unsigned char tag[5] = {'J','F','I','F','\0'};
         int ok = 1;
         int i;
         for (i=0; i < 5; ++i)
            if (xi__get8(z->s) != tag[i])
               ok = 0;
         L -= 5;
         if (ok)
            z->jfif = 1;
      } else if (m == 0xEE && L >= 12) { // Adobe APP14 segment
         static const unsigned char tag[6] = {'A','d','o','b','e','\0'};
         int ok = 1;
         int i;
         for (i=0; i < 6; ++i)
            if (xi__get8(z->s) != tag[i])
               ok = 0;
         L -= 6;
         if (ok) {
            xi__get8(z->s); // version
            xi__get16be(z->s); // flags0
            xi__get16be(z->s); // flags1
            z->app14_color_transform = xi__get8(z->s); // color transform
            L -= 6;
         }
      }

      xi__skip(z->s, L);
      return 1;
   }

   return xi__err("unknown marker","Corrupt JPEG");
}

// after we see SOS
static int xi__process_scan_header(xi__jpeg *z)
{
   int i;
   int Ls = xi__get16be(z->s);
   z->scan_n = xi__get8(z->s);
   if (z->scan_n < 1 || z->scan_n > 4 || z->scan_n > (int) z->s->img_n) return xi__err("bad SOS component count","Corrupt JPEG");
   if (Ls != 6+2*z->scan_n) return xi__err("bad SOS len","Corrupt JPEG");
   for (i=0; i < z->scan_n; ++i) {
      int id = xi__get8(z->s), which;
      int q = xi__get8(z->s);
      for (which = 0; which < z->s->img_n; ++which)
         if (z->img_comp[which].id == id)
            break;
      if (which == z->s->img_n) return 0; // no match
      z->img_comp[which].hd = q >> 4;   if (z->img_comp[which].hd > 3) return xi__err("bad DC huff","Corrupt JPEG");
      z->img_comp[which].ha = q & 15;   if (z->img_comp[which].ha > 3) return xi__err("bad AC huff","Corrupt JPEG");
      z->order[i] = which;
   }

   {
      int aa;
      z->spec_start = xi__get8(z->s);
      z->spec_end   = xi__get8(z->s); // should be 63, but might be 0
      aa = xi__get8(z->s);
      z->succ_high = (aa >> 4);
      z->succ_low  = (aa & 15);
      if (z->progressive) {
         if (z->spec_start > 63 || z->spec_end > 63  || z->spec_start > z->spec_end || z->succ_high > 13 || z->succ_low > 13)
            return xi__err("bad SOS", "Corrupt JPEG");
      } else {
         if (z->spec_start != 0) return xi__err("bad SOS","Corrupt JPEG");
         if (z->succ_high != 0 || z->succ_low != 0) return xi__err("bad SOS","Corrupt JPEG");
         z->spec_end = 63;
      }
   }

   return 1;
}

static int xi__free_jpeg_components(xi__jpeg *z, int ncomp, int why)
{
   int i;
   for (i=0; i < ncomp; ++i) {
      if (z->img_comp[i].raw_data) {
         XI_FREE(z->img_comp[i].raw_data);
         z->img_comp[i].raw_data = NULL;
         z->img_comp[i].data = NULL;
      }
      if (z->img_comp[i].raw_coeff) {
         XI_FREE(z->img_comp[i].raw_coeff);
         z->img_comp[i].raw_coeff = 0;
         z->img_comp[i].coeff = 0;
      }
      if (z->img_comp[i].linebuf) {
         XI_FREE(z->img_comp[i].linebuf);
         z->img_comp[i].linebuf = NULL;
      }
   }
   return why;
}

static int xi__process_frame_header(xi__jpeg *z, int scan)
{
   xi__context *s = z->s;
   int Lf,p,i,q, h_max=1,v_max=1,c;
   Lf = xi__get16be(s);         if (Lf < 11) return xi__err("bad SOF len","Corrupt JPEG"); // JPEG
   p  = xi__get8(s);            if (p != 8) return xi__err("only 8-bit","JPEG format not supported: 8-bit only"); // JPEG baseline
   s->img_y = xi__get16be(s);   if (s->img_y == 0) return xi__err("no header height", "JPEG format not supported: delayed height"); // Legal, but we don't handle it--but neither does IJG
   s->img_x = xi__get16be(s);   if (s->img_x == 0) return xi__err("0 width","Corrupt JPEG"); // JPEG requires
   if (s->img_y > XI_MAX_DIMENSIONS) return xi__err("too large","Very large image (corrupt?)");
   if (s->img_x > XI_MAX_DIMENSIONS) return xi__err("too large","Very large image (corrupt?)");
   c = xi__get8(s);
   if (c != 3 && c != 1 && c != 4) return xi__err("bad component count","Corrupt JPEG");
   s->img_n = c;
   for (i=0; i < c; ++i) {
      z->img_comp[i].data = NULL;
      z->img_comp[i].linebuf = NULL;
   }

   if (Lf != 8+3*s->img_n) return xi__err("bad SOF len","Corrupt JPEG");

   z->rgb = 0;
   for (i=0; i < s->img_n; ++i) {
      static const unsigned char rgb[3] = { 'R', 'G', 'B' };
      z->img_comp[i].id = xi__get8(s);
      if (s->img_n == 3 && z->img_comp[i].id == rgb[i])
         ++z->rgb;
      q = xi__get8(s);
      z->img_comp[i].h = (q >> 4);  if (!z->img_comp[i].h || z->img_comp[i].h > 4) return xi__err("bad H","Corrupt JPEG");
      z->img_comp[i].v = q & 15;    if (!z->img_comp[i].v || z->img_comp[i].v > 4) return xi__err("bad V","Corrupt JPEG");
      z->img_comp[i].tq = xi__get8(s);  if (z->img_comp[i].tq > 3) return xi__err("bad TQ","Corrupt JPEG");
   }

   if (scan != XI__SCAN_load) return 1;

   if (!xi__mad3sizes_valid(s->img_x, s->img_y, s->img_n, 0)) return xi__err("too large", "Image too large to decode");

   for (i=0; i < s->img_n; ++i) {
      if (z->img_comp[i].h > h_max) h_max = z->img_comp[i].h;
      if (z->img_comp[i].v > v_max) v_max = z->img_comp[i].v;
   }

   // check that plane subsampling factors are integer ratios; our resamplers can't deal with fractional ratios
   // and I've never seen a non-corrupted JPEG file actually use them
   for (i=0; i < s->img_n; ++i) {
      if (h_max % z->img_comp[i].h != 0) return xi__err("bad H","Corrupt JPEG");
      if (v_max % z->img_comp[i].v != 0) return xi__err("bad V","Corrupt JPEG");
   }

   // compute interleaved mcu info
   z->img_h_max = h_max;
   z->img_v_max = v_max;
   z->img_mcu_w = h_max * 8;
   z->img_mcu_h = v_max * 8;
   // these sizes can't be more than 17 bits
   z->img_mcu_x = (s->img_x + z->img_mcu_w-1) / z->img_mcu_w;
   z->img_mcu_y = (s->img_y + z->img_mcu_h-1) / z->img_mcu_h;

   for (i=0; i < s->img_n; ++i) {
      // number of effective pixels (e.g. for non-interleaved MCU)
      z->img_comp[i].x = (s->img_x * z->img_comp[i].h + h_max-1) / h_max;
      z->img_comp[i].y = (s->img_y * z->img_comp[i].v + v_max-1) / v_max;
      // to simplify generation, we'll allocate enough memory to decode
      // the bogus oversized data from using interleaved MCUs and their
      // big blocks (e.g. a 16x16 iMCU on an image of width 33); we won't
      // discard the extra data until colorspace conversion
      //
      // img_mcu_x, img_mcu_y: <=17 bits; comp[i].h and .v are <=4 (checked earlier)
      // so these muls can't overflow with 32-bit ints (which we require)
      z->img_comp[i].w2 = z->img_mcu_x * z->img_comp[i].h * 8;
      z->img_comp[i].h2 = z->img_mcu_y * z->img_comp[i].v * 8;
      z->img_comp[i].coeff = 0;
      z->img_comp[i].raw_coeff = 0;
      z->img_comp[i].linebuf = NULL;
      z->img_comp[i].raw_data = xi__malloc_mad2(z->img_comp[i].w2, z->img_comp[i].h2, 15);
      if (z->img_comp[i].raw_data == NULL)
         return xi__free_jpeg_components(z, i+1, xi__err("outofmem", "Out of memory"));
      // align blocks for idct using mmx/sse
      z->img_comp[i].data = (xi_uc*) (((size_t) z->img_comp[i].raw_data + 15) & ~15);
      if (z->progressive) {
         // w2, h2 are multiples of 8 (see above)
         z->img_comp[i].coeff_w = z->img_comp[i].w2 / 8;
         z->img_comp[i].coeff_h = z->img_comp[i].h2 / 8;
         z->img_comp[i].raw_coeff = xi__malloc_mad3(z->img_comp[i].w2, z->img_comp[i].h2, sizeof(short), 15);
         if (z->img_comp[i].raw_coeff == NULL)
            return xi__free_jpeg_components(z, i+1, xi__err("outofmem", "Out of memory"));
         z->img_comp[i].coeff = (short*) (((size_t) z->img_comp[i].raw_coeff + 15) & ~15);
      }
   }

   return 1;
}

// use comparisons since in some cases we handle more than one case (e.g. SOF)
#define xi__DNL(x)         ((x) == 0xdc)
#define xi__SOI(x)         ((x) == 0xd8)
#define xi__EOI(x)         ((x) == 0xd9)
#define xi__SOF(x)         ((x) == 0xc0 || (x) == 0xc1 || (x) == 0xc2)
#define xi__SOS(x)         ((x) == 0xda)

#define xi__SOF_progressive(x)   ((x) == 0xc2)

static int xi__decode_jpeg_header(xi__jpeg *z, int scan)
{
   int m;
   z->jfif = 0;
   z->app14_color_transform = -1; // valid values are 0,1,2
   z->marker = XI__MARKER_none; // initialize cached marker to empty
   m = xi__get_marker(z);
   if (!xi__SOI(m)) return xi__err("no SOI","Corrupt JPEG");
   if (scan == XI__SCAN_type) return 1;
   m = xi__get_marker(z);
   while (!xi__SOF(m)) {
      if (!xi__process_marker(z,m)) return 0;
      m = xi__get_marker(z);
      while (m == XI__MARKER_none) {
         // some files have extra padding after their blocks, so ok, we'll scan
         if (xi__at_eof(z->s)) return xi__err("no SOF", "Corrupt JPEG");
         m = xi__get_marker(z);
      }
   }
   z->progressive = xi__SOF_progressive(m);
   if (!xi__process_frame_header(z, scan)) return 0;
   return 1;
}

static xi_uc xi__skip_jpeg_junk_at_end(xi__jpeg *j)
{
   // some JPEGs have junk at end, skip over it but if we find what looks
   // like a valid marker, resume there
   while (!xi__at_eof(j->s)) {
      xi_uc x = xi__get8(j->s);
      while (x == 0xff) { // might be a marker
         if (xi__at_eof(j->s)) return XI__MARKER_none;
         x = xi__get8(j->s);
         if (x != 0x00 && x != 0xff) {
            // not a stuffed zero or lead-in to another marker, looks
            // like an actual marker, return it
            return x;
         }
         // stuffed zero has x=0 now which ends the loop, meaning we go
         // back to regular scan loop.
         // repeated 0xff keeps trying to read the next byte of the marker.
      }
   }
   return XI__MARKER_none;
}

// decode image to YCbCr format
static int xi__decode_jpeg_image(xi__jpeg *j)
{
   int m;
   for (m = 0; m < 4; m++) {
      j->img_comp[m].raw_data = NULL;
      j->img_comp[m].raw_coeff = NULL;
   }
   j->restart_interval = 0;
   if (!xi__decode_jpeg_header(j, XI__SCAN_load)) return 0;
   m = xi__get_marker(j);
   while (!xi__EOI(m)) {
      if (xi__SOS(m)) {
         if (!xi__process_scan_header(j)) return 0;
         if (!xi__parse_entropy_coded_data(j)) return 0;
         if (j->marker == XI__MARKER_none ) {
         j->marker = xi__skip_jpeg_junk_at_end(j);
            // if we reach eof without hitting a marker, xi__get_marker() below will fail and we'll eventually return 0
         }
         m = xi__get_marker(j);
         if (XI__RESTART(m))
            m = xi__get_marker(j);
      } else if (xi__DNL(m)) {
         int Ld = xi__get16be(j->s);
         xi__uint32 NL = xi__get16be(j->s);
         if (Ld != 4) return xi__err("bad DNL len", "Corrupt JPEG");
         if (NL != j->s->img_y) return xi__err("bad DNL height", "Corrupt JPEG");
         m = xi__get_marker(j);
      } else {
         if (!xi__process_marker(j, m)) return 1;
         m = xi__get_marker(j);
      }
   }
   if (j->progressive)
      xi__jpeg_finish(j);
   return 1;
}

// static jfif-centered resampling (across block boundaries)

typedef xi_uc *(*resample_row_func)(xi_uc *out, xi_uc *in0, xi_uc *in1,
                                    int w, int hs);

#define xi__div4(x) ((xi_uc) ((x) >> 2))

static xi_uc *resample_row_1(xi_uc *out, xi_uc *in_near, xi_uc *in_far, int w, int hs)
{
   XI_NOTUSED(out);
   XI_NOTUSED(in_far);
   XI_NOTUSED(w);
   XI_NOTUSED(hs);
   return in_near;
}

static xi_uc* xi__resample_row_v_2(xi_uc *out, xi_uc *in_near, xi_uc *in_far, int w, int hs)
{
   // need to generate two samples vertically for every one in input
   int i;
   XI_NOTUSED(hs);
   for (i=0; i < w; ++i)
      out[i] = xi__div4(3*in_near[i] + in_far[i] + 2);
   return out;
}

static xi_uc*  xi__resample_row_h_2(xi_uc *out, xi_uc *in_near, xi_uc *in_far, int w, int hs)
{
   // need to generate two samples horizontally for every one in input
   int i;
   xi_uc *input = in_near;

   if (w == 1) {
      // if only one sample, can't do any interpolation
      out[0] = out[1] = input[0];
      return out;
   }

   out[0] = input[0];
   out[1] = xi__div4(input[0]*3 + input[1] + 2);
   for (i=1; i < w-1; ++i) {
      int n = 3*input[i]+2;
      out[i*2+0] = xi__div4(n+input[i-1]);
      out[i*2+1] = xi__div4(n+input[i+1]);
   }
   out[i*2+0] = xi__div4(input[w-2]*3 + input[w-1] + 2);
   out[i*2+1] = input[w-1];

   XI_NOTUSED(in_far);
   XI_NOTUSED(hs);

   return out;
}

#define xi__div16(x) ((xi_uc) ((x) >> 4))

static xi_uc *xi__resample_row_hv_2(xi_uc *out, xi_uc *in_near, xi_uc *in_far, int w, int hs)
{
   // need to generate 2x2 samples for every one in input
   int i,t0,t1;
   if (w == 1) {
      out[0] = out[1] = xi__div4(3*in_near[0] + in_far[0] + 2);
      return out;
   }

   t1 = 3*in_near[0] + in_far[0];
   out[0] = xi__div4(t1+2);
   for (i=1; i < w; ++i) {
      t0 = t1;
      t1 = 3*in_near[i]+in_far[i];
      out[i*2-1] = xi__div16(3*t0 + t1 + 8);
      out[i*2  ] = xi__div16(3*t1 + t0 + 8);
   }
   out[w*2-1] = xi__div4(t1+2);

   XI_NOTUSED(hs);

   return out;
}

#if defined(XI_SSE2) || defined(XI_NEON)
static xi_uc *xi__resample_row_hv_2_simd(xi_uc *out, xi_uc *in_near, xi_uc *in_far, int w, int hs)
{
   // need to generate 2x2 samples for every one in input
   int i=0,t0,t1;

   if (w == 1) {
      out[0] = out[1] = xi__div4(3*in_near[0] + in_far[0] + 2);
      return out;
   }

   t1 = 3*in_near[0] + in_far[0];
   // process groups of 8 pixels for as long as we can.
   // note we can't handle the last pixel in a row in this loop
   // because we need to handle the filter boundary conditions.
   for (; i < ((w-1) & ~7); i += 8) {
#if defined(XI_SSE2)
      // load and perform the vertical filtering pass
      // this uses 3*x + y = 4*x + (y - x)
      __m128i zero  = _mm_setzero_si128();
      __m128i farb  = _mm_loadl_epi64((__m128i *) (in_far + i));
      __m128i nearb = _mm_loadl_epi64((__m128i *) (in_near + i));
      __m128i farw  = _mm_unpacklo_epi8(farb, zero);
      __m128i nearw = _mm_unpacklo_epi8(nearb, zero);
      __m128i diff  = _mm_sub_epi16(farw, nearw);
      __m128i nears = _mm_slli_epi16(nearw, 2);
      __m128i curr  = _mm_add_epi16(nears, diff); // current row

      // horizontal filter works the same based on shifted vers of current
      // row. "prev" is current row shifted right by 1 pixel; we need to
      // insert the previous pixel value (from t1).
      // "next" is current row shifted left by 1 pixel, with first pixel
      // of next block of 8 pixels added in.
      __m128i prv0 = _mm_slli_si128(curr, 2);
      __m128i nxt0 = _mm_srli_si128(curr, 2);
      __m128i prev = _mm_insert_epi16(prv0, t1, 0);
      __m128i next = _mm_insert_epi16(nxt0, 3*in_near[i+8] + in_far[i+8], 7);

      // horizontal filter, polyphase implementation since it's convenient:
      // even pixels = 3*cur + prev = cur*4 + (prev - cur)
      // odd  pixels = 3*cur + next = cur*4 + (next - cur)
      // note the shared term.
      __m128i bias  = _mm_set1_epi16(8);
      __m128i curs = _mm_slli_epi16(curr, 2);
      __m128i prvd = _mm_sub_epi16(prev, curr);
      __m128i nxtd = _mm_sub_epi16(next, curr);
      __m128i curb = _mm_add_epi16(curs, bias);
      __m128i even = _mm_add_epi16(prvd, curb);
      __m128i odd  = _mm_add_epi16(nxtd, curb);

      // interleave even and odd pixels, then undo scaling.
      __m128i int0 = _mm_unpacklo_epi16(even, odd);
      __m128i int1 = _mm_unpackhi_epi16(even, odd);
      __m128i de0  = _mm_srli_epi16(int0, 4);
      __m128i de1  = _mm_srli_epi16(int1, 4);

      // pack and write output
      __m128i outv = _mm_packus_epi16(de0, de1);
      _mm_storeu_si128((__m128i *) (out + i*2), outv);
#elif defined(XI_NEON)
      // load and perform the vertical filtering pass
      // this uses 3*x + y = 4*x + (y - x)
      uint8x8_t farb  = vld1_u8(in_far + i);
      uint8x8_t nearb = vld1_u8(in_near + i);
      int16x8_t diff  = vreinterpretq_s16_u16(vsubl_u8(farb, nearb));
      int16x8_t nears = vreinterpretq_s16_u16(vshll_n_u8(nearb, 2));
      int16x8_t curr  = vaddq_s16(nears, diff); // current row

      // horizontal filter works the same based on shifted vers of current
      // row. "prev" is current row shifted right by 1 pixel; we need to
      // insert the previous pixel value (from t1).
      // "next" is current row shifted left by 1 pixel, with first pixel
      // of next block of 8 pixels added in.
      int16x8_t prv0 = vextq_s16(curr, curr, 7);
      int16x8_t nxt0 = vextq_s16(curr, curr, 1);
      int16x8_t prev = vsetq_lane_s16(t1, prv0, 0);
      int16x8_t next = vsetq_lane_s16(3*in_near[i+8] + in_far[i+8], nxt0, 7);

      // horizontal filter, polyphase implementation since it's convenient:
      // even pixels = 3*cur + prev = cur*4 + (prev - cur)
      // odd  pixels = 3*cur + next = cur*4 + (next - cur)
      // note the shared term.
      int16x8_t curs = vshlq_n_s16(curr, 2);
      int16x8_t prvd = vsubq_s16(prev, curr);
      int16x8_t nxtd = vsubq_s16(next, curr);
      int16x8_t even = vaddq_s16(curs, prvd);
      int16x8_t odd  = vaddq_s16(curs, nxtd);

      // undo scaling and round, then store with even/odd phases interleaved
      uint8x8x2_t o;
      o.val[0] = vqrshrun_n_s16(even, 4);
      o.val[1] = vqrshrun_n_s16(odd,  4);
      vst2_u8(out + i*2, o);
#endif

      // "previous" value for next iter
      t1 = 3*in_near[i+7] + in_far[i+7];
   }

   t0 = t1;
   t1 = 3*in_near[i] + in_far[i];
   out[i*2] = xi__div16(3*t1 + t0 + 8);

   for (++i; i < w; ++i) {
      t0 = t1;
      t1 = 3*in_near[i]+in_far[i];
      out[i*2-1] = xi__div16(3*t0 + t1 + 8);
      out[i*2  ] = xi__div16(3*t1 + t0 + 8);
   }
   out[w*2-1] = xi__div4(t1+2);

   XI_NOTUSED(hs);

   return out;
}
#endif

static xi_uc *xi__resample_row_generic(xi_uc *out, xi_uc *in_near, xi_uc *in_far, int w, int hs)
{
   // resample with nearest-neighbor
   int i,j;
   XI_NOTUSED(in_far);
   for (i=0; i < w; ++i)
      for (j=0; j < hs; ++j)
         out[i*hs+j] = in_near[i];
   return out;
}

// this is a reduced-precision calculation of YCbCr-to-RGB introduced
// to make sure the code produces the same results in both SIMD and scalar
#define xi__float2fixed(x)  (((int) ((x) * 4096.0f + 0.5f)) << 8)
static void xi__YCbCr_to_RGB_row(xi_uc *out, const xi_uc *y, const xi_uc *pcb, const xi_uc *pcr, int count, int step)
{
   int i;
   for (i=0; i < count; ++i) {
      int y_fixed = (y[i] << 20) + (1<<19); // rounding
      int r,g,b;
      int cr = pcr[i] - 128;
      int cb = pcb[i] - 128;
      r = y_fixed +  cr* xi__float2fixed(1.40200f);
      g = y_fixed + (cr*-xi__float2fixed(0.71414f)) + ((cb*-xi__float2fixed(0.34414f)) & 0xffff0000);
      b = y_fixed                                     +   cb* xi__float2fixed(1.77200f);
      r >>= 20;
      g >>= 20;
      b >>= 20;
      if ((unsigned) r > 255) { if (r < 0) r = 0; else r = 255; }
      if ((unsigned) g > 255) { if (g < 0) g = 0; else g = 255; }
      if ((unsigned) b > 255) { if (b < 0) b = 0; else b = 255; }
      out[0] = (xi_uc)r;
      out[1] = (xi_uc)g;
      out[2] = (xi_uc)b;
      out[3] = 255;
      out += step;
   }
}

#if defined(XI_SSE2) || defined(XI_NEON)
static void xi__YCbCr_to_RGB_simd(xi_uc *out, xi_uc const *y, xi_uc const *pcb, xi_uc const *pcr, int count, int step)
{
   int i = 0;

#ifdef XI_SSE2
   // step == 3 is pretty ugly on the final interleave, and i'm not convinced
   // it's useful in practice (you wouldn't use it for textures, for example).
   // so just accelerate step == 4 case.
   if (step == 4) {
      // this is a fairly straightforward implementation and not super-optimized.
      __m128i signflip  = _mm_set1_epi8(-0x80);
      __m128i cr_const0 = _mm_set1_epi16(   (short) ( 1.40200f*4096.0f+0.5f));
      __m128i cr_const1 = _mm_set1_epi16( - (short) ( 0.71414f*4096.0f+0.5f));
      __m128i cb_const0 = _mm_set1_epi16( - (short) ( 0.34414f*4096.0f+0.5f));
      __m128i cb_const1 = _mm_set1_epi16(   (short) ( 1.77200f*4096.0f+0.5f));
      __m128i y_bias = _mm_set1_epi8((char) (unsigned char) 128);
      __m128i xw = _mm_set1_epi16(255); // alpha channel

      for (; i+7 < count; i += 8) {
         // load
         __m128i y_bytes = _mm_loadl_epi64((__m128i *) (y+i));
         __m128i cr_bytes = _mm_loadl_epi64((__m128i *) (pcr+i));
         __m128i cb_bytes = _mm_loadl_epi64((__m128i *) (pcb+i));
         __m128i cr_biased = _mm_xor_si128(cr_bytes, signflip); // -128
         __m128i cb_biased = _mm_xor_si128(cb_bytes, signflip); // -128

         // unpack to short (and left-shift cr, cb by 8)
         __m128i yw  = _mm_unpacklo_epi8(y_bias, y_bytes);
         __m128i crw = _mm_unpacklo_epi8(_mm_setzero_si128(), cr_biased);
         __m128i cbw = _mm_unpacklo_epi8(_mm_setzero_si128(), cb_biased);

         // color transform
         __m128i yws = _mm_srli_epi16(yw, 4);
         __m128i cr0 = _mm_mulhi_epi16(cr_const0, crw);
         __m128i cb0 = _mm_mulhi_epi16(cb_const0, cbw);
         __m128i cb1 = _mm_mulhi_epi16(cbw, cb_const1);
         __m128i cr1 = _mm_mulhi_epi16(crw, cr_const1);
         __m128i rws = _mm_add_epi16(cr0, yws);
         __m128i gwt = _mm_add_epi16(cb0, yws);
         __m128i bws = _mm_add_epi16(yws, cb1);
         __m128i gws = _mm_add_epi16(gwt, cr1);

         // descale
         __m128i rw = _mm_srai_epi16(rws, 4);
         __m128i bw = _mm_srai_epi16(bws, 4);
         __m128i gw = _mm_srai_epi16(gws, 4);

         // back to byte, set up for transpose
         __m128i brb = _mm_packus_epi16(rw, bw);
         __m128i gxb = _mm_packus_epi16(gw, xw);

         // transpose to interleave channels
         __m128i t0 = _mm_unpacklo_epi8(brb, gxb);
         __m128i t1 = _mm_unpackhi_epi8(brb, gxb);
         __m128i o0 = _mm_unpacklo_epi16(t0, t1);
         __m128i o1 = _mm_unpackhi_epi16(t0, t1);

         // store
         _mm_storeu_si128((__m128i *) (out + 0), o0);
         _mm_storeu_si128((__m128i *) (out + 16), o1);
         out += 32;
      }
   }
#endif

#ifdef XI_NEON
   // in this version, step=3 support would be easy to add. but is there demand?
   if (step == 4) {
      // this is a fairly straightforward implementation and not super-optimized.
      uint8x8_t signflip = vdup_n_u8(0x80);
      int16x8_t cr_const0 = vdupq_n_s16(   (short) ( 1.40200f*4096.0f+0.5f));
      int16x8_t cr_const1 = vdupq_n_s16( - (short) ( 0.71414f*4096.0f+0.5f));
      int16x8_t cb_const0 = vdupq_n_s16( - (short) ( 0.34414f*4096.0f+0.5f));
      int16x8_t cb_const1 = vdupq_n_s16(   (short) ( 1.77200f*4096.0f+0.5f));

      for (; i+7 < count; i += 8) {
         // load
         uint8x8_t y_bytes  = vld1_u8(y + i);
         uint8x8_t cr_bytes = vld1_u8(pcr + i);
         uint8x8_t cb_bytes = vld1_u8(pcb + i);
         int8x8_t cr_biased = vreinterpret_s8_u8(vsub_u8(cr_bytes, signflip));
         int8x8_t cb_biased = vreinterpret_s8_u8(vsub_u8(cb_bytes, signflip));

         // expand to s16
         int16x8_t yws = vreinterpretq_s16_u16(vshll_n_u8(y_bytes, 4));
         int16x8_t crw = vshll_n_s8(cr_biased, 7);
         int16x8_t cbw = vshll_n_s8(cb_biased, 7);

         // color transform
         int16x8_t cr0 = vqdmulhq_s16(crw, cr_const0);
         int16x8_t cb0 = vqdmulhq_s16(cbw, cb_const0);
         int16x8_t cr1 = vqdmulhq_s16(crw, cr_const1);
         int16x8_t cb1 = vqdmulhq_s16(cbw, cb_const1);
         int16x8_t rws = vaddq_s16(yws, cr0);
         int16x8_t gws = vaddq_s16(vaddq_s16(yws, cb0), cr1);
         int16x8_t bws = vaddq_s16(yws, cb1);

         // undo scaling, round, convert to byte
         uint8x8x4_t o;
         o.val[0] = vqrshrun_n_s16(rws, 4);
         o.val[1] = vqrshrun_n_s16(gws, 4);
         o.val[2] = vqrshrun_n_s16(bws, 4);
         o.val[3] = vdup_n_u8(255);

         // store, interleaving r/g/b/a
         vst4_u8(out, o);
         out += 8*4;
      }
   }
#endif

   for (; i < count; ++i) {
      int y_fixed = (y[i] << 20) + (1<<19); // rounding
      int r,g,b;
      int cr = pcr[i] - 128;
      int cb = pcb[i] - 128;
      r = y_fixed + cr* xi__float2fixed(1.40200f);
      g = y_fixed + cr*-xi__float2fixed(0.71414f) + ((cb*-xi__float2fixed(0.34414f)) & 0xffff0000);
      b = y_fixed                                   +   cb* xi__float2fixed(1.77200f);
      r >>= 20;
      g >>= 20;
      b >>= 20;
      if ((unsigned) r > 255) { if (r < 0) r = 0; else r = 255; }
      if ((unsigned) g > 255) { if (g < 0) g = 0; else g = 255; }
      if ((unsigned) b > 255) { if (b < 0) b = 0; else b = 255; }
      out[0] = (xi_uc)r;
      out[1] = (xi_uc)g;
      out[2] = (xi_uc)b;
      out[3] = 255;
      out += step;
   }
}
#endif

// set up the kernels
static void xi__setup_jpeg(xi__jpeg *j)
{
   j->idct_block_kernel = xi__idct_block;
   j->YCbCr_to_RGB_kernel = xi__YCbCr_to_RGB_row;
   j->resample_row_hv_2_kernel = xi__resample_row_hv_2;

#ifdef XI_SSE2
   if (xi__sse2_available()) {
      j->idct_block_kernel = xi__idct_simd;
      j->YCbCr_to_RGB_kernel = xi__YCbCr_to_RGB_simd;
      j->resample_row_hv_2_kernel = xi__resample_row_hv_2_simd;
   }
#endif

#ifdef XI_NEON
   j->idct_block_kernel = xi__idct_simd;
   j->YCbCr_to_RGB_kernel = xi__YCbCr_to_RGB_simd;
   j->resample_row_hv_2_kernel = xi__resample_row_hv_2_simd;
#endif
}

// clean up the temporary component buffers
static void xi__cleanup_jpeg(xi__jpeg *j)
{
   xi__free_jpeg_components(j, j->s->img_n, 0);
}

typedef struct
{
   resample_row_func resample;
   xi_uc *line0,*line1;
   int hs,vs;   // expansion factor in each axis
   int w_lores; // horizontal pixels pre-expansion
   int ystep;   // how far through vertical expansion we are
   int ypos;    // which pre-expansion row we're on
} xi__resample;

// fast 0..255 * 0..255 => 0..255 rounded multiplication
static xi_uc xi__blinn_8x8(xi_uc x, xi_uc y)
{
   unsigned int t = x*y + 128;
   return (xi_uc) ((t + (t >>8)) >> 8);
}

static xi_uc *load_jpeg_image(xi__jpeg *z, int *out_x, int *out_y, int *comp, int req_comp)
{
   int n, decode_n, is_rgb;
   z->s->img_n = 0; // make xi__cleanup_jpeg safe

   // validate req_comp
   if (req_comp < 0 || req_comp > 4) return xi__errpuc("bad req_comp", "Internal error");

   // load a jpeg image from whichever source, but leave in YCbCr format
   if (!xi__decode_jpeg_image(z)) { xi__cleanup_jpeg(z); return NULL; }

   // determine actual number of components to generate
   n = req_comp ? req_comp : z->s->img_n >= 3 ? 3 : 1;

   is_rgb = z->s->img_n == 3 && (z->rgb == 3 || (z->app14_color_transform == 0 && !z->jfif));

   if (z->s->img_n == 3 && n < 3 && !is_rgb)
      decode_n = 1;
   else
      decode_n = z->s->img_n;

   // nothing to do if no components requested; check this now to avoid
   // accessing uninitialized coutput[0] later
   if (decode_n <= 0) { xi__cleanup_jpeg(z); return NULL; }

   // resample and color-convert
   {
      int k;
      unsigned int i,j;
      xi_uc *output;
      xi_uc *coutput[4] = { NULL, NULL, NULL, NULL };

      xi__resample res_comp[4];

      for (k=0; k < decode_n; ++k) {
         xi__resample *r = &res_comp[k];

         // allocate line buffer big enough for upsampling off the edges
         // with upsample factor of 4
         z->img_comp[k].linebuf = (xi_uc *) xi__malloc(z->s->img_x + 3);
         if (!z->img_comp[k].linebuf) { xi__cleanup_jpeg(z); return xi__errpuc("outofmem", "Out of memory"); }

         r->hs      = z->img_h_max / z->img_comp[k].h;
         r->vs      = z->img_v_max / z->img_comp[k].v;
         r->ystep   = r->vs >> 1;
         r->w_lores = (z->s->img_x + r->hs-1) / r->hs;
         r->ypos    = 0;
         r->line0   = r->line1 = z->img_comp[k].data;

         if      (r->hs == 1 && r->vs == 1) r->resample = resample_row_1;
         else if (r->hs == 1 && r->vs == 2) r->resample = xi__resample_row_v_2;
         else if (r->hs == 2 && r->vs == 1) r->resample = xi__resample_row_h_2;
         else if (r->hs == 2 && r->vs == 2) r->resample = z->resample_row_hv_2_kernel;
         else                               r->resample = xi__resample_row_generic;
      }

      // can't error after this so, this is safe
      output = (xi_uc *) xi__malloc_mad3(n, z->s->img_x, z->s->img_y, 1);
      if (!output) { xi__cleanup_jpeg(z); return xi__errpuc("outofmem", "Out of memory"); }

      // now go ahead and resample
      for (j=0; j < z->s->img_y; ++j) {
         xi_uc *out = output + n * z->s->img_x * j;
         for (k=0; k < decode_n; ++k) {
            xi__resample *r = &res_comp[k];
            int y_bot = r->ystep >= (r->vs >> 1);
            coutput[k] = r->resample(z->img_comp[k].linebuf,
                                     y_bot ? r->line1 : r->line0,
                                     y_bot ? r->line0 : r->line1,
                                     r->w_lores, r->hs);
            if (++r->ystep >= r->vs) {
               r->ystep = 0;
               r->line0 = r->line1;
               if (++r->ypos < z->img_comp[k].y)
                  r->line1 += z->img_comp[k].w2;
            }
         }
         if (n >= 3) {
            xi_uc *y = coutput[0];
            if (z->s->img_n == 3) {
               if (is_rgb) {
                  for (i=0; i < z->s->img_x; ++i) {
                     out[0] = y[i];
                     out[1] = coutput[1][i];
                     out[2] = coutput[2][i];
                     out[3] = 255;
                     out += n;
                  }
               } else {
                  z->YCbCr_to_RGB_kernel(out, y, coutput[1], coutput[2], z->s->img_x, n);
               }
            } else if (z->s->img_n == 4) {
               if (z->app14_color_transform == 0) { // CMYK
                  for (i=0; i < z->s->img_x; ++i) {
                     xi_uc m = coutput[3][i];
                     out[0] = xi__blinn_8x8(coutput[0][i], m);
                     out[1] = xi__blinn_8x8(coutput[1][i], m);
                     out[2] = xi__blinn_8x8(coutput[2][i], m);
                     out[3] = 255;
                     out += n;
                  }
               } else if (z->app14_color_transform == 2) { // YCCK
                  z->YCbCr_to_RGB_kernel(out, y, coutput[1], coutput[2], z->s->img_x, n);
                  for (i=0; i < z->s->img_x; ++i) {
                     xi_uc m = coutput[3][i];
                     out[0] = xi__blinn_8x8(255 - out[0], m);
                     out[1] = xi__blinn_8x8(255 - out[1], m);
                     out[2] = xi__blinn_8x8(255 - out[2], m);
                     out += n;
                  }
               } else { // YCbCr + alpha?  Ignore the fourth channel for now
                  z->YCbCr_to_RGB_kernel(out, y, coutput[1], coutput[2], z->s->img_x, n);
               }
            } else
               for (i=0; i < z->s->img_x; ++i) {
                  out[0] = out[1] = out[2] = y[i];
                  out[3] = 255; // not used if n==3
                  out += n;
               }
         } else {
            if (is_rgb) {
               if (n == 1)
                  for (i=0; i < z->s->img_x; ++i)
                     *out++ = xi__compute_y(coutput[0][i], coutput[1][i], coutput[2][i]);
               else {
                  for (i=0; i < z->s->img_x; ++i, out += 2) {
                     out[0] = xi__compute_y(coutput[0][i], coutput[1][i], coutput[2][i]);
                     out[1] = 255;
                  }
               }
            } else if (z->s->img_n == 4 && z->app14_color_transform == 0) {
               for (i=0; i < z->s->img_x; ++i) {
                  xi_uc m = coutput[3][i];
                  xi_uc r = xi__blinn_8x8(coutput[0][i], m);
                  xi_uc g = xi__blinn_8x8(coutput[1][i], m);
                  xi_uc b = xi__blinn_8x8(coutput[2][i], m);
                  out[0] = xi__compute_y(r, g, b);
                  out[1] = 255;
                  out += n;
               }
            } else if (z->s->img_n == 4 && z->app14_color_transform == 2) {
               for (i=0; i < z->s->img_x; ++i) {
                  out[0] = xi__blinn_8x8(255 - coutput[0][i], coutput[3][i]);
                  out[1] = 255;
                  out += n;
               }
            } else {
               xi_uc *y = coutput[0];
               if (n == 1)
                  for (i=0; i < z->s->img_x; ++i) out[i] = y[i];
               else
                  for (i=0; i < z->s->img_x; ++i) { *out++ = y[i]; *out++ = 255; }
            }
         }
      }
      xi__cleanup_jpeg(z);
      *out_x = z->s->img_x;
      *out_y = z->s->img_y;
      if (comp) *comp = z->s->img_n >= 3 ? 3 : 1; // report original components, not output
      return output;
   }
}

static void *xi__jpeg_load(xi__context *s, int *x, int *y, int *comp, int req_comp, xi__result_info *ri)
{
   unsigned char* result;
   xi__jpeg* j = (xi__jpeg*) xi__malloc(sizeof(xi__jpeg));
   if (!j) return xi__errpuc("outofmem", "Out of memory");
   memset(j, 0, sizeof(xi__jpeg));
   XI_NOTUSED(ri);
   j->s = s;
   xi__setup_jpeg(j);
   result = load_jpeg_image(j, x,y,comp,req_comp);
   XI_FREE(j);
   return result;
}

static int xi__jpeg_test(xi__context *s)
{
   int r;
   xi__jpeg* j = (xi__jpeg*)xi__malloc(sizeof(xi__jpeg));
   if (!j) return xi__err("outofmem", "Out of memory");
   memset(j, 0, sizeof(xi__jpeg));
   j->s = s;
   xi__setup_jpeg(j);
   r = xi__decode_jpeg_header(j, XI__SCAN_type);
   xi__rewind(s);
   XI_FREE(j);
   return r;
}

static int xi__jpeg_info_raw(xi__jpeg *j, int *x, int *y, int *comp)
{
   if (!xi__decode_jpeg_header(j, XI__SCAN_header)) {
      xi__rewind( j->s );
      return 0;
   }
   if (x) *x = j->s->img_x;
   if (y) *y = j->s->img_y;
   if (comp) *comp = j->s->img_n >= 3 ? 3 : 1;
   return 1;
}

static int xi__jpeg_info(xi__context *s, int *x, int *y, int *comp)
{
   int result;
   xi__jpeg* j = (xi__jpeg*) (xi__malloc(sizeof(xi__jpeg)));
   if (!j) return xi__err("outofmem", "Out of memory");
   memset(j, 0, sizeof(xi__jpeg));
   j->s = s;
   result = xi__jpeg_info_raw(j, x, y, comp);
   XI_FREE(j);
   return result;
}
#endif

// public domain zlib decode    v0.2  Sean Barrett 2006-11-18
//    simple implementation
//      - all input must be provided in an upfront buffer
//      - all output is written to a single output buffer (can malloc/realloc)
//    performance
//      - fast huffman

#ifndef XI_NO_ZLIB

// fast-way is faster to check than jpeg huffman, but slow way is slower
#define XI__ZFAST_BITS  9 // accelerate all cases in default tables
#define XI__ZFAST_MASK  ((1 << XI__ZFAST_BITS) - 1)
#define XI__ZNSYMS 288 // number of symbols in literal/length alphabet

// zlib-style huffman encoding
// (jpegs packs from left, zlib from right, so can't share code)
typedef struct
{
   xi__uint16 fast[1 << XI__ZFAST_BITS];
   xi__uint16 firstcode[16];
   int maxcode[17];
   xi__uint16 firstsymbol[16];
   xi_uc  size[XI__ZNSYMS];
   xi__uint16 value[XI__ZNSYMS];
} xi__zhuffman;

xi_inline static int xi__bitreverse16(int n)
{
  n = ((n & 0xAAAA) >>  1) | ((n & 0x5555) << 1);
  n = ((n & 0xCCCC) >>  2) | ((n & 0x3333) << 2);
  n = ((n & 0xF0F0) >>  4) | ((n & 0x0F0F) << 4);
  n = ((n & 0xFF00) >>  8) | ((n & 0x00FF) << 8);
  return n;
}

xi_inline static int xi__bit_reverse(int v, int bits)
{
   XI_ASSERT(bits <= 16);
   // to bit reverse n bits, reverse 16 and shift
   // e.g. 11 bits, bit reverse and shift away 5
   return xi__bitreverse16(v) >> (16-bits);
}

static int xi__zbuild_huffman(xi__zhuffman *z, const xi_uc *sizelist, int num)
{
   int i,k=0;
   int code, next_code[16], sizes[17];

   // DEFLATE spec for generating codes
   memset(sizes, 0, sizeof(sizes));
   memset(z->fast, 0, sizeof(z->fast));
   for (i=0; i < num; ++i)
      ++sizes[sizelist[i]];
   sizes[0] = 0;
   for (i=1; i < 16; ++i)
      if (sizes[i] > (1 << i))
         return xi__err("bad sizes", "Corrupt PNG");
   code = 0;
   for (i=1; i < 16; ++i) {
      next_code[i] = code;
      z->firstcode[i] = (xi__uint16) code;
      z->firstsymbol[i] = (xi__uint16) k;
      code = (code + sizes[i]);
      if (sizes[i])
         if (code-1 >= (1 << i)) return xi__err("bad codelengths","Corrupt PNG");
      z->maxcode[i] = code << (16-i); // preshift for inner loop
      code <<= 1;
      k += sizes[i];
   }
   z->maxcode[16] = 0x10000; // sentinel
   for (i=0; i < num; ++i) {
      int s = sizelist[i];
      if (s) {
         int c = next_code[s] - z->firstcode[s] + z->firstsymbol[s];
         xi__uint16 fastv = (xi__uint16) ((s << 9) | i);
         z->size [c] = (xi_uc     ) s;
         z->value[c] = (xi__uint16) i;
         if (s <= XI__ZFAST_BITS) {
            int j = xi__bit_reverse(next_code[s],s);
            while (j < (1 << XI__ZFAST_BITS)) {
               z->fast[j] = fastv;
               j += (1 << s);
            }
         }
         ++next_code[s];
      }
   }
   return 1;
}

// zlib-from-memory implementation for PNG reading
//    because PNG allows splitting the zlib stream arbitrarily,
//    and it's annoying structurally to have PNG call ZLIB call PNG,
//    we require PNG read all the IDATs and combine them into a single
//    memory buffer

typedef struct
{
   xi_uc *zbuffer, *zbuffer_end;
   int num_bits;
   int hit_zeof_once;
   xi__uint32 code_buffer;

   char *zout;
   char *zout_start;
   char *zout_end;
   int   z_expandable;

   xi__zhuffman z_length, z_distance;
} xi__zbuf;

xi_inline static int xi__zeof(xi__zbuf *z)
{
   return (z->zbuffer >= z->zbuffer_end);
}

xi_inline static xi_uc xi__zget8(xi__zbuf *z)
{
   return xi__zeof(z) ? 0 : *z->zbuffer++;
}

static void xi__fill_bits(xi__zbuf *z)
{
   do {
      if (z->code_buffer >= (1U << z->num_bits)) {
        z->zbuffer = z->zbuffer_end;  /* treat this as EOF so we fail. */
        return;
      }
      z->code_buffer |= (unsigned int) xi__zget8(z) << z->num_bits;
      z->num_bits += 8;
   } while (z->num_bits <= 24);
}

xi_inline static unsigned int xi__zreceive(xi__zbuf *z, int n)
{
   unsigned int k;
   if (z->num_bits < n) xi__fill_bits(z);
   k = z->code_buffer & ((1 << n) - 1);
   z->code_buffer >>= n;
   z->num_bits -= n;
   return k;
}

static int xi__zhuffman_decode_slowpath(xi__zbuf *a, xi__zhuffman *z)
{
   int b,s,k;
   // not resolved by fast table, so compute it the slow way
   // use jpeg approach, which requires MSbits at top
   k = xi__bit_reverse(a->code_buffer, 16);
   for (s=XI__ZFAST_BITS+1; ; ++s)
      if (k < z->maxcode[s])
         break;
   if (s >= 16) return -1; // invalid code!
   // code size is s, so:
   b = (k >> (16-s)) - z->firstcode[s] + z->firstsymbol[s];
   if (b >= XI__ZNSYMS) return -1; // some data was corrupt somewhere!
   if (z->size[b] != s) return -1;  // was originally an assert, but report failure instead.
   a->code_buffer >>= s;
   a->num_bits -= s;
   return z->value[b];
}

xi_inline static int xi__zhuffman_decode(xi__zbuf *a, xi__zhuffman *z)
{
   int b,s;
   if (a->num_bits < 16) {
      if (xi__zeof(a)) {
         if (!a->hit_zeof_once) {
            // This is the first time we hit eof, insert 16 extra padding btis
            // to allow us to keep going; if we actually consume any of them
            // though, that is invalid data. This is caught later.
            a->hit_zeof_once = 1;
            a->num_bits += 16; // add 16 implicit zero bits
         } else {
            // We already inserted our extra 16 padding bits and are again
            // out, this stream is actually prematurely terminated.
            return -1;
         }
      } else {
         xi__fill_bits(a);
      }
   }
   b = z->fast[a->code_buffer & XI__ZFAST_MASK];
   if (b) {
      s = b >> 9;
      a->code_buffer >>= s;
      a->num_bits -= s;
      return b & 511;
   }
   return xi__zhuffman_decode_slowpath(a, z);
}

static int xi__zexpand(xi__zbuf *z, char *zout, int n)  // need to make room for n bytes
{
   char *q;
   unsigned int cur, limit, old_limit;
   z->zout = zout;
   if (!z->z_expandable) return xi__err("output buffer limit","Corrupt PNG");
   cur   = (unsigned int) (z->zout - z->zout_start);
   limit = old_limit = (unsigned) (z->zout_end - z->zout_start);
   if (UINT_MAX - cur < (unsigned) n) return xi__err("outofmem", "Out of memory");
   while (cur + n > limit) {
      if(limit > UINT_MAX / 2) return xi__err("outofmem", "Out of memory");
      limit *= 2;
   }
   q = (char *) XI_REALLOC_SIZED(z->zout_start, old_limit, limit);
   XI_NOTUSED(old_limit);
   if (q == NULL) return xi__err("outofmem", "Out of memory");
   z->zout_start = q;
   z->zout       = q + cur;
   z->zout_end   = q + limit;
   return 1;
}

static const int xi__zlength_base[31] = {
   3,4,5,6,7,8,9,10,11,13,
   15,17,19,23,27,31,35,43,51,59,
   67,83,99,115,131,163,195,227,258,0,0 };

static const int xi__zlength_extra[31]=
{ 0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0,0,0 };

static const int xi__zdist_base[32] = { 1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,
257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577,0,0};

static const int xi__zdist_extra[32] =
{ 0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};

static int xi__parse_huffman_block(xi__zbuf *a)
{
   char *zout = a->zout;
   for(;;) {
      int z = xi__zhuffman_decode(a, &a->z_length);
      if (z < 256) {
         if (z < 0) return xi__err("bad huffman code","Corrupt PNG"); // error in huffman codes
         if (zout >= a->zout_end) {
            if (!xi__zexpand(a, zout, 1)) return 0;
            zout = a->zout;
         }
         *zout++ = (char) z;
      } else {
         xi_uc *p;
         int len,dist;
         if (z == 256) {
            a->zout = zout;
            if (a->hit_zeof_once && a->num_bits < 16) {
               // The first time we hit zeof, we inserted 16 extra zero bits into our bit
               // buffer so the decoder can just do its speculative decoding. But if we
               // actually consumed any of those bits (which is the case when num_bits < 16),
               // the stream actually read past the end so it is malformed.
               return xi__err("unexpected end","Corrupt PNG");
            }
            return 1;
         }
         if (z >= 286) return xi__err("bad huffman code","Corrupt PNG"); // per DEFLATE, length codes 286 and 287 must not appear in compressed data
         z -= 257;
         len = xi__zlength_base[z];
         if (xi__zlength_extra[z]) len += xi__zreceive(a, xi__zlength_extra[z]);
         z = xi__zhuffman_decode(a, &a->z_distance);
         if (z < 0 || z >= 30) return xi__err("bad huffman code","Corrupt PNG"); // per DEFLATE, distance codes 30 and 31 must not appear in compressed data
         dist = xi__zdist_base[z];
         if (xi__zdist_extra[z]) dist += xi__zreceive(a, xi__zdist_extra[z]);
         if (zout - a->zout_start < dist) return xi__err("bad dist","Corrupt PNG");
         if (len > a->zout_end - zout) {
            if (!xi__zexpand(a, zout, len)) return 0;
            zout = a->zout;
         }
         p = (xi_uc *) (zout - dist);
         if (dist == 1) { // run of one byte; common in images.
            xi_uc v = *p;
            if (len) { do *zout++ = v; while (--len); }
         } else {
            if (len) { do *zout++ = *p++; while (--len); }
         }
      }
   }
}

static int xi__compute_huffman_codes(xi__zbuf *a)
{
   static const xi_uc length_dezigzag[19] = { 16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15 };
   xi__zhuffman z_codelength;
   xi_uc lencodes[286+32+137];//padding for maximum single op
   xi_uc codelength_sizes[19];
   int i,n;

   int hlit  = xi__zreceive(a,5) + 257;
   int hdist = xi__zreceive(a,5) + 1;
   int hclen = xi__zreceive(a,4) + 4;
   int ntot  = hlit + hdist;

   memset(codelength_sizes, 0, sizeof(codelength_sizes));
   for (i=0; i < hclen; ++i) {
      int s = xi__zreceive(a,3);
      codelength_sizes[length_dezigzag[i]] = (xi_uc) s;
   }
   if (!xi__zbuild_huffman(&z_codelength, codelength_sizes, 19)) return 0;

   n = 0;
   while (n < ntot) {
      int c = xi__zhuffman_decode(a, &z_codelength);
      if (c < 0 || c >= 19) return xi__err("bad codelengths", "Corrupt PNG");
      if (c < 16)
         lencodes[n++] = (xi_uc) c;
      else {
         xi_uc fill = 0;
         if (c == 16) {
            c = xi__zreceive(a,2)+3;
            if (n == 0) return xi__err("bad codelengths", "Corrupt PNG");
            fill = lencodes[n-1];
         } else if (c == 17) {
            c = xi__zreceive(a,3)+3;
         } else if (c == 18) {
            c = xi__zreceive(a,7)+11;
         } else {
            return xi__err("bad codelengths", "Corrupt PNG");
         }
         if (ntot - n < c) return xi__err("bad codelengths", "Corrupt PNG");
         memset(lencodes+n, fill, c);
         n += c;
      }
   }
   if (n != ntot) return xi__err("bad codelengths","Corrupt PNG");
   if (!xi__zbuild_huffman(&a->z_length, lencodes, hlit)) return 0;
   if (!xi__zbuild_huffman(&a->z_distance, lencodes+hlit, hdist)) return 0;
   return 1;
}

static int xi__parse_uncompressed_block(xi__zbuf *a)
{
   xi_uc header[4];
   int len,nlen,k;
   if (a->num_bits & 7)
      xi__zreceive(a, a->num_bits & 7); // discard
   // drain the bit-packed data into header
   k = 0;
   while (a->num_bits > 0) {
      header[k++] = (xi_uc) (a->code_buffer & 255); // suppress MSVC run-time check
      a->code_buffer >>= 8;
      a->num_bits -= 8;
   }
   if (a->num_bits < 0) return xi__err("zlib corrupt","Corrupt PNG");
   // now fill header the normal way
   while (k < 4)
      header[k++] = xi__zget8(a);
   len  = header[1] * 256 + header[0];
   nlen = header[3] * 256 + header[2];
   if (nlen != (len ^ 0xffff)) return xi__err("zlib corrupt","Corrupt PNG");
   if (a->zbuffer + len > a->zbuffer_end) return xi__err("read past buffer","Corrupt PNG");
   if (a->zout + len > a->zout_end)
      if (!xi__zexpand(a, a->zout, len)) return 0;
   memcpy(a->zout, a->zbuffer, len);
   a->zbuffer += len;
   a->zout += len;
   return 1;
}

static int xi__parse_zlib_header(xi__zbuf *a)
{
   int cmf   = xi__zget8(a);
   int cm    = cmf & 15;
   /* int cinfo = cmf >> 4; */
   int flg   = xi__zget8(a);
   if (xi__zeof(a)) return xi__err("bad zlib header","Corrupt PNG"); // zlib spec
   if ((cmf*256+flg) % 31 != 0) return xi__err("bad zlib header","Corrupt PNG"); // zlib spec
   if (flg & 32) return xi__err("no preset dict","Corrupt PNG"); // preset dictionary not allowed in png
   if (cm != 8) return xi__err("bad compression","Corrupt PNG"); // DEFLATE required for png
   // window = 1 << (8 + cinfo)... but who cares, we fully buffer output
   return 1;
}

static const xi_uc xi__zdefault_length[XI__ZNSYMS] =
{
   8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
   8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
   8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
   8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
   8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
   9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
   9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
   9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8
};
static const xi_uc xi__zdefault_distance[32] =
{
   5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5
};
/*
Init algorithm:
{
   int i;   // use <= to match clearly with spec
   for (i=0; i <= 143; ++i)     xi__zdefault_length[i]   = 8;
   for (   ; i <= 255; ++i)     xi__zdefault_length[i]   = 9;
   for (   ; i <= 279; ++i)     xi__zdefault_length[i]   = 7;
   for (   ; i <= 287; ++i)     xi__zdefault_length[i]   = 8;

   for (i=0; i <=  31; ++i)     xi__zdefault_distance[i] = 5;
}
*/

static int xi__parse_zlib(xi__zbuf *a, int parse_header)
{
   int final, type;
   if (parse_header)
      if (!xi__parse_zlib_header(a)) return 0;
   a->num_bits = 0;
   a->code_buffer = 0;
   a->hit_zeof_once = 0;
   do {
      final = xi__zreceive(a,1);
      type = xi__zreceive(a,2);
      if (type == 0) {
         if (!xi__parse_uncompressed_block(a)) return 0;
      } else if (type == 3) {
         return 0;
      } else {
         if (type == 1) {
            // use fixed code lengths
            if (!xi__zbuild_huffman(&a->z_length  , xi__zdefault_length  , XI__ZNSYMS)) return 0;
            if (!xi__zbuild_huffman(&a->z_distance, xi__zdefault_distance,  32)) return 0;
         } else {
            if (!xi__compute_huffman_codes(a)) return 0;
         }
         if (!xi__parse_huffman_block(a)) return 0;
      }
   } while (!final);
   return 1;
}

static int xi__do_zlib(xi__zbuf *a, char *obuf, int olen, int exp, int parse_header)
{
   a->zout_start = obuf;
   a->zout       = obuf;
   a->zout_end   = obuf + olen;
   a->z_expandable = exp;

   return xi__parse_zlib(a, parse_header);
}

XIDEF char *xi_zlib_decode_malloc_guesssize(const char *buffer, int len, int initial_size, int *outlen)
{
   xi__zbuf a;
   char *p = (char *) xi__malloc(initial_size);
   if (p == NULL) return NULL;
   a.zbuffer = (xi_uc *) buffer;
   a.zbuffer_end = (xi_uc *) buffer + len;
   if (xi__do_zlib(&a, p, initial_size, 1, 1)) {
      if (outlen) *outlen = (int) (a.zout - a.zout_start);
      return a.zout_start;
   } else {
      XI_FREE(a.zout_start);
      return NULL;
   }
}

XIDEF char *xi_zlib_decode_malloc(char const *buffer, int len, int *outlen)
{
   return xi_zlib_decode_malloc_guesssize(buffer, len, 16384, outlen);
}

XIDEF char *xi_zlib_decode_malloc_guesssize_headerflag(const char *buffer, int len, int initial_size, int *outlen, int parse_header)
{
   xi__zbuf a;
   char *p = (char *) xi__malloc(initial_size);
   if (p == NULL) return NULL;
   a.zbuffer = (xi_uc *) buffer;
   a.zbuffer_end = (xi_uc *) buffer + len;
   if (xi__do_zlib(&a, p, initial_size, 1, parse_header)) {
      if (outlen) *outlen = (int) (a.zout - a.zout_start);
      return a.zout_start;
   } else {
      XI_FREE(a.zout_start);
      return NULL;
   }
}

XIDEF int xi_zlib_decode_buffer(char *obuffer, int olen, char const *ibuffer, int ilen)
{
   xi__zbuf a;
   a.zbuffer = (xi_uc *) ibuffer;
   a.zbuffer_end = (xi_uc *) ibuffer + ilen;
   if (xi__do_zlib(&a, obuffer, olen, 0, 1))
      return (int) (a.zout - a.zout_start);
   else
      return -1;
}

XIDEF char *xi_zlib_decode_noheader_malloc(char const *buffer, int len, int *outlen)
{
   xi__zbuf a;
   char *p = (char *) xi__malloc(16384);
   if (p == NULL) return NULL;
   a.zbuffer = (xi_uc *) buffer;
   a.zbuffer_end = (xi_uc *) buffer+len;
   if (xi__do_zlib(&a, p, 16384, 1, 0)) {
      if (outlen) *outlen = (int) (a.zout - a.zout_start);
      return a.zout_start;
   } else {
      XI_FREE(a.zout_start);
      return NULL;
   }
}

XIDEF int xi_zlib_decode_noheader_buffer(char *obuffer, int olen, const char *ibuffer, int ilen)
{
   xi__zbuf a;
   a.zbuffer = (xi_uc *) ibuffer;
   a.zbuffer_end = (xi_uc *) ibuffer + ilen;
   if (xi__do_zlib(&a, obuffer, olen, 0, 0))
      return (int) (a.zout - a.zout_start);
   else
      return -1;
}
#endif

// public domain "baseline" PNG decoder   v0.10  Sean Barrett 2006-11-18
//    simple implementation
//      - only 8-bit samples
//      - no CRC checking
//      - allocates lots of intermediate memory
//        - avoids problem of streaming data between subsystems
//        - avoids explicit window management
//    performance
//      - uses stb_zlib, a PD zlib implementation with fast huffman decoding

#ifndef XI_NO_PNG
typedef struct
{
   xi__uint32 length;
   xi__uint32 type;
} xi__pngchunk;

static xi__pngchunk xi__get_chunk_header(xi__context *s)
{
   xi__pngchunk c;
   c.length = xi__get32be(s);
   c.type   = xi__get32be(s);
   return c;
}

static int xi__check_png_header(xi__context *s)
{
   static const xi_uc png_sig[8] = { 137,80,78,71,13,10,26,10 };
   int i;
   for (i=0; i < 8; ++i)
      if (xi__get8(s) != png_sig[i]) return xi__err("bad png sig","Not a PNG");
   return 1;
}

typedef struct
{
   xi__context *s;
   xi_uc *idata, *expanded, *out;
   int depth;
} xi__png;


enum {
   XI__F_none=0,
   XI__F_sub=1,
   XI__F_up=2,
   XI__F_avg=3,
   XI__F_paeth=4,
   // synthetic filter used for first scanline to avoid needing a dummy row of 0s
   XI__F_avg_first
};

static xi_uc first_row_filter[5] =
{
   XI__F_none,
   XI__F_sub,
   XI__F_none,
   XI__F_avg_first,
   XI__F_sub // Paeth with b=c=0 turns out to be equivalent to sub
};

static int xi__paeth(int a, int b, int c)
{
   // This formulation looks very different from the reference in the PNG spec, but is
   // actually equivalent and has favorable data dependencies and admits straightforward
   // generation of branch-free code, which helps performance significantly.
   int thresh = c*3 - (a + b);
   int lo = a < b ? a : b;
   int hi = a < b ? b : a;
   int t0 = (hi <= thresh) ? lo : c;
   int t1 = (thresh <= lo) ? hi : t0;
   return t1;
}

static const xi_uc xi__depth_scale_table[9] = { 0, 0xff, 0x55, 0, 0x11, 0,0,0, 0x01 };

// adds an extra all-255 alpha channel
// dest == src is legal
// img_n must be 1 or 3
static void xi__create_png_alpha_expand8(xi_uc *dest, xi_uc *src, xi__uint32 x, int img_n)
{
   int i;
   // must process data backwards since we allow dest==src
   if (img_n == 1) {
      for (i=x-1; i >= 0; --i) {
         dest[i*2+1] = 255;
         dest[i*2+0] = src[i];
      }
   } else {
      XI_ASSERT(img_n == 3);
      for (i=x-1; i >= 0; --i) {
         dest[i*4+3] = 255;
         dest[i*4+2] = src[i*3+2];
         dest[i*4+1] = src[i*3+1];
         dest[i*4+0] = src[i*3+0];
      }
   }
}

// create the png data from post-deflated data
static int xi__create_png_image_raw(xi__png *a, xi_uc *raw, xi__uint32 raw_len, int out_n, xi__uint32 x, xi__uint32 y, int depth, int color)
{
   int bytes = (depth == 16 ? 2 : 1);
   xi__context *s = a->s;
   xi__uint32 i,j,stride = x*out_n*bytes;
   xi__uint32 img_len, img_width_bytes;
   xi_uc *filter_buf;
   int all_ok = 1;
   int k;
   int img_n = s->img_n; // copy it into a local for later

   int output_bytes = out_n*bytes;
   int filter_bytes = img_n*bytes;
   int width = x;

   XI_ASSERT(out_n == s->img_n || out_n == s->img_n+1);
   a->out = (xi_uc *) xi__malloc_mad3(x, y, output_bytes, 0); // extra bytes to write off the end into
   if (!a->out) return xi__err("outofmem", "Out of memory");

   // note: error exits here don't need to clean up a->out individually,
   // xi__do_png always does on error.
   if (!xi__mad3sizes_valid(img_n, x, depth, 7)) return xi__err("too large", "Corrupt PNG");
   img_width_bytes = (((img_n * x * depth) + 7) >> 3);
   if (!xi__mad2sizes_valid(img_width_bytes, y, img_width_bytes)) return xi__err("too large", "Corrupt PNG");
   img_len = (img_width_bytes + 1) * y;

   // we used to check for exact match between raw_len and img_len on non-interlaced PNGs,
   // but issue #276 reported a PNG in the wild that had extra data at the end (all zeros),
   // so just check for raw_len < img_len always.
   if (raw_len < img_len) return xi__err("not enough pixels","Corrupt PNG");

   // Allocate two scan lines worth of filter workspace buffer.
   filter_buf = (xi_uc *) xi__malloc_mad2(img_width_bytes, 2, 0);
   if (!filter_buf) return xi__err("outofmem", "Out of memory");

   // Filtering for low-bit-depth images
   if (depth < 8) {
      filter_bytes = 1;
      width = img_width_bytes;
   }

   for (j=0; j < y; ++j) {
      // cur/prior filter buffers alternate
      xi_uc *cur = filter_buf + (j & 1)*img_width_bytes;
      xi_uc *prior = filter_buf + (~j & 1)*img_width_bytes;
      xi_uc *dest = a->out + stride*j;
      int nk = width * filter_bytes;
      int filter = *raw++;

      // check filter type
      if (filter > 4) {
         all_ok = xi__err("invalid filter","Corrupt PNG");
         break;
      }

      // if first row, use special filter that doesn't sample previous row
      if (j == 0) filter = first_row_filter[filter];

      // perform actual filtering
      switch (filter) {
      case XI__F_none:
         memcpy(cur, raw, nk);
         break;
      case XI__F_sub:
         memcpy(cur, raw, filter_bytes);
         for (k = filter_bytes; k < nk; ++k)
            cur[k] = XI__BYTECAST(raw[k] + cur[k-filter_bytes]);
         break;
      case XI__F_up:
         for (k = 0; k < nk; ++k)
            cur[k] = XI__BYTECAST(raw[k] + prior[k]);
         break;
      case XI__F_avg:
         for (k = 0; k < filter_bytes; ++k)
            cur[k] = XI__BYTECAST(raw[k] + (prior[k]>>1));
         for (k = filter_bytes; k < nk; ++k)
            cur[k] = XI__BYTECAST(raw[k] + ((prior[k] + cur[k-filter_bytes])>>1));
         break;
      case XI__F_paeth:
         for (k = 0; k < filter_bytes; ++k)
            cur[k] = XI__BYTECAST(raw[k] + prior[k]); // prior[k] == xi__paeth(0,prior[k],0)
         for (k = filter_bytes; k < nk; ++k)
            cur[k] = XI__BYTECAST(raw[k] + xi__paeth(cur[k-filter_bytes], prior[k], prior[k-filter_bytes]));
         break;
      case XI__F_avg_first:
         memcpy(cur, raw, filter_bytes);
         for (k = filter_bytes; k < nk; ++k)
            cur[k] = XI__BYTECAST(raw[k] + (cur[k-filter_bytes] >> 1));
         break;
      }

      raw += nk;

      // expand decoded bits in cur to dest, also adding an extra alpha channel if desired
      if (depth < 8) {
         xi_uc scale = (color == 0) ? xi__depth_scale_table[depth] : 1; // scale grayscale values to 0..255 range
         xi_uc *in = cur;
         xi_uc *out = dest;
         xi_uc inb = 0;
         xi__uint32 nsmp = x*img_n;

         // expand bits to bytes first
         if (depth == 4) {
            for (i=0; i < nsmp; ++i) {
               if ((i & 1) == 0) inb = *in++;
               *out++ = scale * (inb >> 4);
               inb <<= 4;
            }
         } else if (depth == 2) {
            for (i=0; i < nsmp; ++i) {
               if ((i & 3) == 0) inb = *in++;
               *out++ = scale * (inb >> 6);
               inb <<= 2;
            }
         } else {
            XI_ASSERT(depth == 1);
            for (i=0; i < nsmp; ++i) {
               if ((i & 7) == 0) inb = *in++;
               *out++ = scale * (inb >> 7);
               inb <<= 1;
            }
         }

         // insert alpha=255 values if desired
         if (img_n != out_n)
            xi__create_png_alpha_expand8(dest, dest, x, img_n);
      } else if (depth == 8) {
         if (img_n == out_n)
            memcpy(dest, cur, x*img_n);
         else
            xi__create_png_alpha_expand8(dest, cur, x, img_n);
      } else if (depth == 16) {
         // convert the image data from big-endian to platform-native
         xi__uint16 *dest16 = (xi__uint16*)dest;
         xi__uint32 nsmp = x*img_n;

         if (img_n == out_n) {
            for (i = 0; i < nsmp; ++i, ++dest16, cur += 2)
               *dest16 = (cur[0] << 8) | cur[1];
         } else {
            XI_ASSERT(img_n+1 == out_n);
            if (img_n == 1) {
               for (i = 0; i < x; ++i, dest16 += 2, cur += 2) {
                  dest16[0] = (cur[0] << 8) | cur[1];
                  dest16[1] = 0xffff;
               }
            } else {
               XI_ASSERT(img_n == 3);
               for (i = 0; i < x; ++i, dest16 += 4, cur += 6) {
                  dest16[0] = (cur[0] << 8) | cur[1];
                  dest16[1] = (cur[2] << 8) | cur[3];
                  dest16[2] = (cur[4] << 8) | cur[5];
                  dest16[3] = 0xffff;
               }
            }
         }
      }
   }

   XI_FREE(filter_buf);
   if (!all_ok) return 0;

   return 1;
}

static int xi__create_png_image(xi__png *a, xi_uc *image_data, xi__uint32 image_data_len, int out_n, int depth, int color, int interlaced)
{
   int bytes = (depth == 16 ? 2 : 1);
   int out_bytes = out_n * bytes;
   xi_uc *final;
   int p;
   if (!interlaced)
      return xi__create_png_image_raw(a, image_data, image_data_len, out_n, a->s->img_x, a->s->img_y, depth, color);

   // de-interlacing
   final = (xi_uc *) xi__malloc_mad3(a->s->img_x, a->s->img_y, out_bytes, 0);
   if (!final) return xi__err("outofmem", "Out of memory");
   for (p=0; p < 7; ++p) {
      int xorig[] = { 0,4,0,2,0,1,0 };
      int yorig[] = { 0,0,4,0,2,0,1 };
      int xspc[]  = { 8,8,4,4,2,2,1 };
      int yspc[]  = { 8,8,8,4,4,2,2 };
      int i,j,x,y;
      // pass1_x[4] = 0, pass1_x[5] = 1, pass1_x[12] = 1
      x = (a->s->img_x - xorig[p] + xspc[p]-1) / xspc[p];
      y = (a->s->img_y - yorig[p] + yspc[p]-1) / yspc[p];
      if (x && y) {
         xi__uint32 img_len = ((((a->s->img_n * x * depth) + 7) >> 3) + 1) * y;
         if (!xi__create_png_image_raw(a, image_data, image_data_len, out_n, x, y, depth, color)) {
            XI_FREE(final);
            return 0;
         }
         for (j=0; j < y; ++j) {
            for (i=0; i < x; ++i) {
               int out_y = j*yspc[p]+yorig[p];
               int out_x = i*xspc[p]+xorig[p];
               memcpy(final + out_y*a->s->img_x*out_bytes + out_x*out_bytes,
                      a->out + (j*x+i)*out_bytes, out_bytes);
            }
         }
         XI_FREE(a->out);
         image_data += img_len;
         image_data_len -= img_len;
      }
   }
   a->out = final;

   return 1;
}

static int xi__compute_transparency(xi__png *z, xi_uc tc[3], int out_n)
{
   xi__context *s = z->s;
   xi__uint32 i, pixel_count = s->img_x * s->img_y;
   xi_uc *p = z->out;

   // compute color-based transparency, assuming we've
   // already got 255 as the alpha value in the output
   XI_ASSERT(out_n == 2 || out_n == 4);

   if (out_n == 2) {
      for (i=0; i < pixel_count; ++i) {
         p[1] = (p[0] == tc[0] ? 0 : 255);
         p += 2;
      }
   } else {
      for (i=0; i < pixel_count; ++i) {
         if (p[0] == tc[0] && p[1] == tc[1] && p[2] == tc[2])
            p[3] = 0;
         p += 4;
      }
   }
   return 1;
}

static int xi__compute_transparency16(xi__png *z, xi__uint16 tc[3], int out_n)
{
   xi__context *s = z->s;
   xi__uint32 i, pixel_count = s->img_x * s->img_y;
   xi__uint16 *p = (xi__uint16*) z->out;

   // compute color-based transparency, assuming we've
   // already got 65535 as the alpha value in the output
   XI_ASSERT(out_n == 2 || out_n == 4);

   if (out_n == 2) {
      for (i = 0; i < pixel_count; ++i) {
         p[1] = (p[0] == tc[0] ? 0 : 65535);
         p += 2;
      }
   } else {
      for (i = 0; i < pixel_count; ++i) {
         if (p[0] == tc[0] && p[1] == tc[1] && p[2] == tc[2])
            p[3] = 0;
         p += 4;
      }
   }
   return 1;
}

static int xi__expand_png_palette(xi__png *a, xi_uc *palette, int len, int pal_img_n)
{
   xi__uint32 i, pixel_count = a->s->img_x * a->s->img_y;
   xi_uc *p, *temp_out, *orig = a->out;

   p = (xi_uc *) xi__malloc_mad2(pixel_count, pal_img_n, 0);
   if (p == NULL) return xi__err("outofmem", "Out of memory");

   // between here and free(out) below, exitting would leak
   temp_out = p;

   if (pal_img_n == 3) {
      for (i=0; i < pixel_count; ++i) {
         int n = orig[i]*4;
         p[0] = palette[n  ];
         p[1] = palette[n+1];
         p[2] = palette[n+2];
         p += 3;
      }
   } else {
      for (i=0; i < pixel_count; ++i) {
         int n = orig[i]*4;
         p[0] = palette[n  ];
         p[1] = palette[n+1];
         p[2] = palette[n+2];
         p[3] = palette[n+3];
         p += 4;
      }
   }
   XI_FREE(a->out);
   a->out = temp_out;

   XI_NOTUSED(len);

   return 1;
}

static int xi__unpremultiply_on_load_global = 0;
static int xi__de_iphone_flag_global = 0;

XIDEF void xi_set_unpremultiply_on_load(int flag_true_if_should_unpremultiply)
{
   xi__unpremultiply_on_load_global = flag_true_if_should_unpremultiply;
}

XIDEF void xi_convert_iphone_png_to_rgb(int flag_true_if_should_convert)
{
   xi__de_iphone_flag_global = flag_true_if_should_convert;
}

#ifndef XI_THREAD_LOCAL
#define xi__unpremultiply_on_load  xi__unpremultiply_on_load_global
#define xi__de_iphone_flag  xi__de_iphone_flag_global
#else
static XI_THREAD_LOCAL int xi__unpremultiply_on_load_local, xi__unpremultiply_on_load_set;
static XI_THREAD_LOCAL int xi__de_iphone_flag_local, xi__de_iphone_flag_set;

XIDEF void xi_set_unpremultiply_on_load_thread(int flag_true_if_should_unpremultiply)
{
   xi__unpremultiply_on_load_local = flag_true_if_should_unpremultiply;
   xi__unpremultiply_on_load_set = 1;
}

XIDEF void xi_convert_iphone_png_to_rgb_thread(int flag_true_if_should_convert)
{
   xi__de_iphone_flag_local = flag_true_if_should_convert;
   xi__de_iphone_flag_set = 1;
}

#define xi__unpremultiply_on_load  (xi__unpremultiply_on_load_set           \
                                       ? xi__unpremultiply_on_load_local      \
                                       : xi__unpremultiply_on_load_global)
#define xi__de_iphone_flag  (xi__de_iphone_flag_set                         \
                                ? xi__de_iphone_flag_local                    \
                                : xi__de_iphone_flag_global)
#endif // XI_THREAD_LOCAL

static void xi__de_iphone(xi__png *z)
{
   xi__context *s = z->s;
   xi__uint32 i, pixel_count = s->img_x * s->img_y;
   xi_uc *p = z->out;

   if (s->img_out_n == 3) {  // convert bgr to rgb
      for (i=0; i < pixel_count; ++i) {
         xi_uc t = p[0];
         p[0] = p[2];
         p[2] = t;
         p += 3;
      }
   } else {
      XI_ASSERT(s->img_out_n == 4);
      if (xi__unpremultiply_on_load) {
         // convert bgr to rgb and unpremultiply
         for (i=0; i < pixel_count; ++i) {
            xi_uc a = p[3];
            xi_uc t = p[0];
            if (a) {
               xi_uc half = a / 2;
               p[0] = (p[2] * 255 + half) / a;
               p[1] = (p[1] * 255 + half) / a;
               p[2] = ( t   * 255 + half) / a;
            } else {
               p[0] = p[2];
               p[2] = t;
            }
            p += 4;
         }
      } else {
         // convert bgr to rgb
         for (i=0; i < pixel_count; ++i) {
            xi_uc t = p[0];
            p[0] = p[2];
            p[2] = t;
            p += 4;
         }
      }
   }
}

#define XI__PNG_TYPE(a,b,c,d)  (((unsigned) (a) << 24) + ((unsigned) (b) << 16) + ((unsigned) (c) << 8) + (unsigned) (d))

static int xi__parse_png_file(xi__png *z, int scan, int req_comp)
{
   xi_uc palette[1024], pal_img_n=0;
   xi_uc has_trans=0, tc[3]={0};
   xi__uint16 tc16[3];
   xi__uint32 ioff=0, idata_limit=0, i, pal_len=0;
   int first=1,k,interlace=0, color=0, is_iphone=0;
   xi__context *s = z->s;

   z->expanded = NULL;
   z->idata = NULL;
   z->out = NULL;

   if (!xi__check_png_header(s)) return 0;

   if (scan == XI__SCAN_type) return 1;

   for (;;) {
      xi__pngchunk c = xi__get_chunk_header(s);
      switch (c.type) {
         case XI__PNG_TYPE('C','g','B','I'):
            is_iphone = 1;
            xi__skip(s, c.length);
            break;
         case XI__PNG_TYPE('I','H','D','R'): {
            int comp,filter;
            if (!first) return xi__err("multiple IHDR","Corrupt PNG");
            first = 0;
            if (c.length != 13) return xi__err("bad IHDR len","Corrupt PNG");
            s->img_x = xi__get32be(s);
            s->img_y = xi__get32be(s);
            if (s->img_y > XI_MAX_DIMENSIONS) return xi__err("too large","Very large image (corrupt?)");
            if (s->img_x > XI_MAX_DIMENSIONS) return xi__err("too large","Very large image (corrupt?)");
            z->depth = xi__get8(s);  if (z->depth != 1 && z->depth != 2 && z->depth != 4 && z->depth != 8 && z->depth != 16)  return xi__err("1/2/4/8/16-bit only","PNG not supported: 1/2/4/8/16-bit only");
            color = xi__get8(s);  if (color > 6)         return xi__err("bad ctype","Corrupt PNG");
            if (color == 3 && z->depth == 16)                  return xi__err("bad ctype","Corrupt PNG");
            if (color == 3) pal_img_n = 3; else if (color & 1) return xi__err("bad ctype","Corrupt PNG");
            comp  = xi__get8(s);  if (comp) return xi__err("bad comp method","Corrupt PNG");
            filter= xi__get8(s);  if (filter) return xi__err("bad filter method","Corrupt PNG");
            interlace = xi__get8(s); if (interlace>1) return xi__err("bad interlace method","Corrupt PNG");
            if (!s->img_x || !s->img_y) return xi__err("0-pixel image","Corrupt PNG");
            if (!pal_img_n) {
               s->img_n = (color & 2 ? 3 : 1) + (color & 4 ? 1 : 0);
               if ((1 << 30) / s->img_x / s->img_n < s->img_y) return xi__err("too large", "Image too large to decode");
            } else {
               // if paletted, then pal_n is our final components, and
               // img_n is # components to decompress/filter.
               s->img_n = 1;
               if ((1 << 30) / s->img_x / 4 < s->img_y) return xi__err("too large","Corrupt PNG");
            }
            // even with SCAN_header, have to scan to see if we have a tRNS
            break;
         }

         case XI__PNG_TYPE('P','L','T','E'):  {
            if (first) return xi__err("first not IHDR", "Corrupt PNG");
            if (c.length > 256*3) return xi__err("invalid PLTE","Corrupt PNG");
            pal_len = c.length / 3;
            if (pal_len * 3 != c.length) return xi__err("invalid PLTE","Corrupt PNG");
            for (i=0; i < pal_len; ++i) {
               palette[i*4+0] = xi__get8(s);
               palette[i*4+1] = xi__get8(s);
               palette[i*4+2] = xi__get8(s);
               palette[i*4+3] = 255;
            }
            break;
         }

         case XI__PNG_TYPE('t','R','N','S'): {
            if (first) return xi__err("first not IHDR", "Corrupt PNG");
            if (z->idata) return xi__err("tRNS after IDAT","Corrupt PNG");
            if (pal_img_n) {
               if (scan == XI__SCAN_header) { s->img_n = 4; return 1; }
               if (pal_len == 0) return xi__err("tRNS before PLTE","Corrupt PNG");
               if (c.length > pal_len) return xi__err("bad tRNS len","Corrupt PNG");
               pal_img_n = 4;
               for (i=0; i < c.length; ++i)
                  palette[i*4+3] = xi__get8(s);
            } else {
               if (!(s->img_n & 1)) return xi__err("tRNS with alpha","Corrupt PNG");
               if (c.length != (xi__uint32) s->img_n*2) return xi__err("bad tRNS len","Corrupt PNG");
               has_trans = 1;
               // non-paletted with tRNS = constant alpha. if header-scanning, we can stop now.
               if (scan == XI__SCAN_header) { ++s->img_n; return 1; }
               if (z->depth == 16) {
                  for (k = 0; k < s->img_n && k < 3; ++k) // extra loop test to suppress false GCC warning
                     tc16[k] = (xi__uint16)xi__get16be(s); // copy the values as-is
               } else {
                  for (k = 0; k < s->img_n && k < 3; ++k)
                     tc[k] = (xi_uc)(xi__get16be(s) & 255) * xi__depth_scale_table[z->depth]; // non 8-bit images will be larger
               }
            }
            break;
         }

         case XI__PNG_TYPE('I','D','A','T'): {
            if (first) return xi__err("first not IHDR", "Corrupt PNG");
            if (pal_img_n && !pal_len) return xi__err("no PLTE","Corrupt PNG");
            if (scan == XI__SCAN_header) {
               // header scan definitely stops at first IDAT
               if (pal_img_n)
                  s->img_n = pal_img_n;
               return 1;
            }
            if (c.length > (1u << 30)) return xi__err("IDAT size limit", "IDAT section larger than 2^30 bytes");
            if ((int)(ioff + c.length) < (int)ioff) return 0;
            if (ioff + c.length > idata_limit) {
               xi__uint32 idata_limit_old = idata_limit;
               xi_uc *p;
               if (idata_limit == 0) idata_limit = c.length > 4096 ? c.length : 4096;
               while (ioff + c.length > idata_limit)
                  idata_limit *= 2;
               XI_NOTUSED(idata_limit_old);
               p = (xi_uc *) XI_REALLOC_SIZED(z->idata, idata_limit_old, idata_limit); if (p == NULL) return xi__err("outofmem", "Out of memory");
               z->idata = p;
            }
            if (!xi__getn(s, z->idata+ioff,c.length)) return xi__err("outofdata","Corrupt PNG");
            ioff += c.length;
            break;
         }

         case XI__PNG_TYPE('I','E','N','D'): {
            xi__uint32 raw_len, bpl;
            if (first) return xi__err("first not IHDR", "Corrupt PNG");
            if (scan != XI__SCAN_load) return 1;
            if (z->idata == NULL) return xi__err("no IDAT","Corrupt PNG");
            // initial guess for decoded data size to avoid unnecessary reallocs
            bpl = (s->img_x * z->depth + 7) / 8; // bytes per line, per component
            raw_len = bpl * s->img_y * s->img_n /* pixels */ + s->img_y /* filter mode per row */;
            z->expanded = (xi_uc *) xi_zlib_decode_malloc_guesssize_headerflag((char *) z->idata, ioff, raw_len, (int *) &raw_len, !is_iphone);
            if (z->expanded == NULL) return 0; // zlib should set error
            XI_FREE(z->idata); z->idata = NULL;
            if ((req_comp == s->img_n+1 && req_comp != 3 && !pal_img_n) || has_trans)
               s->img_out_n = s->img_n+1;
            else
               s->img_out_n = s->img_n;
            if (!xi__create_png_image(z, z->expanded, raw_len, s->img_out_n, z->depth, color, interlace)) return 0;
            if (has_trans) {
               if (z->depth == 16) {
                  if (!xi__compute_transparency16(z, tc16, s->img_out_n)) return 0;
               } else {
                  if (!xi__compute_transparency(z, tc, s->img_out_n)) return 0;
               }
            }
            if (is_iphone && xi__de_iphone_flag && s->img_out_n > 2)
               xi__de_iphone(z);
            if (pal_img_n) {
               // pal_img_n == 3 or 4
               s->img_n = pal_img_n; // record the actual colors we had
               s->img_out_n = pal_img_n;
               if (req_comp >= 3) s->img_out_n = req_comp;
               if (!xi__expand_png_palette(z, palette, pal_len, s->img_out_n))
                  return 0;
            } else if (has_trans) {
               // non-paletted image with tRNS -> source image has (constant) alpha
               ++s->img_n;
            }
            XI_FREE(z->expanded); z->expanded = NULL;
            // end of PNG chunk, read and skip CRC
            xi__get32be(s);
            return 1;
         }

         default:
            // if critical, fail
            if (first) return xi__err("first not IHDR", "Corrupt PNG");
            if ((c.type & (1 << 29)) == 0) {
               #ifndef XI_NO_FAILURE_STRINGS
               // not threadsafe
               static char invalid_chunk[] = "XXXX PNG chunk not known";
               invalid_chunk[0] = XI__BYTECAST(c.type >> 24);
               invalid_chunk[1] = XI__BYTECAST(c.type >> 16);
               invalid_chunk[2] = XI__BYTECAST(c.type >>  8);
               invalid_chunk[3] = XI__BYTECAST(c.type >>  0);
               #endif
               return xi__err(invalid_chunk, "PNG not supported: unknown PNG chunk type");
            }
            xi__skip(s, c.length);
            break;
      }
      // end of PNG chunk, read and skip CRC
      xi__get32be(s);
   }
}

static void *xi__do_png(xi__png *p, int *x, int *y, int *n, int req_comp, xi__result_info *ri)
{
   void *result=NULL;
   if (req_comp < 0 || req_comp > 4) return xi__errpuc("bad req_comp", "Internal error");
   if (xi__parse_png_file(p, XI__SCAN_load, req_comp)) {
      if (p->depth <= 8)
         ri->bits_per_channel = 8;
      else if (p->depth == 16)
         ri->bits_per_channel = 16;
      else
         return xi__errpuc("bad bits_per_channel", "PNG not supported: unsupported color depth");
      result = p->out;
      p->out = NULL;
      if (req_comp && req_comp != p->s->img_out_n) {
         if (ri->bits_per_channel == 8)
            result = xi__convert_format((unsigned char *) result, p->s->img_out_n, req_comp, p->s->img_x, p->s->img_y);
         else
            result = xi__convert_format16((xi__uint16 *) result, p->s->img_out_n, req_comp, p->s->img_x, p->s->img_y);
         p->s->img_out_n = req_comp;
         if (result == NULL) return result;
      }
      *x = p->s->img_x;
      *y = p->s->img_y;
      if (n) *n = p->s->img_n;
   }
   XI_FREE(p->out);      p->out      = NULL;
   XI_FREE(p->expanded); p->expanded = NULL;
   XI_FREE(p->idata);    p->idata    = NULL;

   return result;
}

static void *xi__png_load(xi__context *s, int *x, int *y, int *comp, int req_comp, xi__result_info *ri)
{
   xi__png p;
   p.s = s;
   return xi__do_png(&p, x,y,comp,req_comp, ri);
}

static int xi__png_test(xi__context *s)
{
   int r;
   r = xi__check_png_header(s);
   xi__rewind(s);
   return r;
}

static int xi__png_info_raw(xi__png *p, int *x, int *y, int *comp)
{
   if (!xi__parse_png_file(p, XI__SCAN_header, 0)) {
      xi__rewind( p->s );
      return 0;
   }
   if (x) *x = p->s->img_x;
   if (y) *y = p->s->img_y;
   if (comp) *comp = p->s->img_n;
   return 1;
}

static int xi__png_info(xi__context *s, int *x, int *y, int *comp)
{
   xi__png p;
   p.s = s;
   return xi__png_info_raw(&p, x, y, comp);
}

static int xi__png_is16(xi__context *s)
{
   xi__png p;
   p.s = s;
   if (!xi__png_info_raw(&p, NULL, NULL, NULL))
  return 0;
   if (p.depth != 16) {
      xi__rewind(p.s);
      return 0;
   }
   return 1;
}
#endif

// Microsoft/Windows BMP image

#ifndef XI_NO_BMP
static int xi__bmp_test_raw(xi__context *s)
{
   int r;
   int sz;
   if (xi__get8(s) != 'B') return 0;
   if (xi__get8(s) != 'M') return 0;
   xi__get32le(s); // discard filesize
   xi__get16le(s); // discard reserved
   xi__get16le(s); // discard reserved
   xi__get32le(s); // discard data offset
   sz = xi__get32le(s);
   r = (sz == 12 || sz == 40 || sz == 56 || sz == 108 || sz == 124);
   return r;
}

static int xi__bmp_test(xi__context *s)
{
   int r = xi__bmp_test_raw(s);
   xi__rewind(s);
   return r;
}


// returns 0..31 for the highest set bit
static int xi__high_bit(unsigned int z)
{
   int n=0;
   if (z == 0) return -1;
   if (z >= 0x10000) { n += 16; z >>= 16; }
   if (z >= 0x00100) { n +=  8; z >>=  8; }
   if (z >= 0x00010) { n +=  4; z >>=  4; }
   if (z >= 0x00004) { n +=  2; z >>=  2; }
   if (z >= 0x00002) { n +=  1;/* >>=  1;*/ }
   return n;
}

static int xi__bitcount(unsigned int a)
{
   a = (a & 0x55555555) + ((a >>  1) & 0x55555555); // max 2
   a = (a & 0x33333333) + ((a >>  2) & 0x33333333); // max 4
   a = (a + (a >> 4)) & 0x0f0f0f0f; // max 8 per 4, now 8 bits
   a = (a + (a >> 8)); // max 16 per 8 bits
   a = (a + (a >> 16)); // max 32 per 8 bits
   return a & 0xff;
}

// extract an arbitrarily-aligned N-bit value (N=bits)
// from v, and then make it 8-bits long and fractionally
// extend it to full full range.
static int xi__shiftsigned(unsigned int v, int shift, int bits)
{
   static unsigned int mul_table[9] = {
      0,
      0xff/*0b11111111*/, 0x55/*0b01010101*/, 0x49/*0b01001001*/, 0x11/*0b00010001*/,
      0x21/*0b00100001*/, 0x41/*0b01000001*/, 0x81/*0b10000001*/, 0x01/*0b00000001*/,
   };
   static unsigned int shift_table[9] = {
      0, 0,0,1,0,2,4,6,0,
   };
   if (shift < 0)
      v <<= -shift;
   else
      v >>= shift;
   XI_ASSERT(v < 256);
   v >>= (8-bits);
   XI_ASSERT(bits >= 0 && bits <= 8);
   return (int) ((unsigned) v * mul_table[bits]) >> shift_table[bits];
}

typedef struct
{
   int bpp, offset, hsz;
   unsigned int mr,mg,mb,ma, all_a;
   int extra_read;
} xi__bmp_data;

static int xi__bmp_set_mask_defaults(xi__bmp_data *info, int compress)
{
   // BI_BITFIELDS specifies masks explicitly, don't override
   if (compress == 3)
      return 1;

   if (compress == 0) {
      if (info->bpp == 16) {
         info->mr = 31u << 10;
         info->mg = 31u <<  5;
         info->mb = 31u <<  0;
      } else if (info->bpp == 32) {
         info->mr = 0xffu << 16;
         info->mg = 0xffu <<  8;
         info->mb = 0xffu <<  0;
         info->ma = 0xffu << 24;
         info->all_a = 0; // if all_a is 0 at end, then we loaded alpha channel but it was all 0
      } else {
         // otherwise, use defaults, which is all-0
         info->mr = info->mg = info->mb = info->ma = 0;
      }
      return 1;
   }
   return 0; // error
}

static void *xi__bmp_parse_header(xi__context *s, xi__bmp_data *info)
{
   int hsz;
   if (xi__get8(s) != 'B' || xi__get8(s) != 'M') return xi__errpuc("not BMP", "Corrupt BMP");
   xi__get32le(s); // discard filesize
   xi__get16le(s); // discard reserved
   xi__get16le(s); // discard reserved
   info->offset = xi__get32le(s);
   info->hsz = hsz = xi__get32le(s);
   info->mr = info->mg = info->mb = info->ma = 0;
   info->extra_read = 14;

   if (info->offset < 0) return xi__errpuc("bad BMP", "bad BMP");

   if (hsz != 12 && hsz != 40 && hsz != 56 && hsz != 108 && hsz != 124) return xi__errpuc("unknown BMP", "BMP type not supported: unknown");
   if (hsz == 12) {
      s->img_x = xi__get16le(s);
      s->img_y = xi__get16le(s);
   } else {
      s->img_x = xi__get32le(s);
      s->img_y = xi__get32le(s);
   }
   if (xi__get16le(s) != 1) return xi__errpuc("bad BMP", "bad BMP");
   info->bpp = xi__get16le(s);
   if (hsz != 12) {
      int compress = xi__get32le(s);
      if (compress == 1 || compress == 2) return xi__errpuc("BMP RLE", "BMP type not supported: RLE");
      if (compress >= 4) return xi__errpuc("BMP JPEG/PNG", "BMP type not supported: unsupported compression"); // this includes PNG/JPEG modes
      if (compress == 3 && info->bpp != 16 && info->bpp != 32) return xi__errpuc("bad BMP", "bad BMP"); // bitfields requires 16 or 32 bits/pixel
      xi__get32le(s); // discard sizeof
      xi__get32le(s); // discard hres
      xi__get32le(s); // discard vres
      xi__get32le(s); // discard colorsused
      xi__get32le(s); // discard max important
      if (hsz == 40 || hsz == 56) {
         if (hsz == 56) {
            xi__get32le(s);
            xi__get32le(s);
            xi__get32le(s);
            xi__get32le(s);
         }
         if (info->bpp == 16 || info->bpp == 32) {
            if (compress == 0) {
               xi__bmp_set_mask_defaults(info, compress);
            } else if (compress == 3) {
               info->mr = xi__get32le(s);
               info->mg = xi__get32le(s);
               info->mb = xi__get32le(s);
               info->extra_read += 12;
               // not documented, but generated by photoshop and handled by mspaint
               if (info->mr == info->mg && info->mg == info->mb) {
                  // ?!?!?
                  return xi__errpuc("bad BMP", "bad BMP");
               }
            } else
               return xi__errpuc("bad BMP", "bad BMP");
         }
      } else {
         // V4/V5 header
         int i;
         if (hsz != 108 && hsz != 124)
            return xi__errpuc("bad BMP", "bad BMP");
         info->mr = xi__get32le(s);
         info->mg = xi__get32le(s);
         info->mb = xi__get32le(s);
         info->ma = xi__get32le(s);
         if (compress != 3) // override mr/mg/mb unless in BI_BITFIELDS mode, as per docs
            xi__bmp_set_mask_defaults(info, compress);
         xi__get32le(s); // discard color space
         for (i=0; i < 12; ++i)
            xi__get32le(s); // discard color space parameters
         if (hsz == 124) {
            xi__get32le(s); // discard rendering intent
            xi__get32le(s); // discard offset of profile data
            xi__get32le(s); // discard size of profile data
            xi__get32le(s); // discard reserved
         }
      }
   }
   return (void *) 1;
}


static void *xi__bmp_load(xi__context *s, int *x, int *y, int *comp, int req_comp, xi__result_info *ri)
{
   xi_uc *out;
   unsigned int mr=0,mg=0,mb=0,ma=0, all_a;
   xi_uc pal[256][4];
   int psize=0,i,j,width;
   int flip_vertically, pad, target;
   xi__bmp_data info;
   XI_NOTUSED(ri);

   info.all_a = 255;
   if (xi__bmp_parse_header(s, &info) == NULL)
      return NULL; // error code already set

   flip_vertically = ((int) s->img_y) > 0;
   s->img_y = abs((int) s->img_y);

   if (s->img_y > XI_MAX_DIMENSIONS) return xi__errpuc("too large","Very large image (corrupt?)");
   if (s->img_x > XI_MAX_DIMENSIONS) return xi__errpuc("too large","Very large image (corrupt?)");

   mr = info.mr;
   mg = info.mg;
   mb = info.mb;
   ma = info.ma;
   all_a = info.all_a;

   if (info.hsz == 12) {
      if (info.bpp < 24)
         psize = (info.offset - info.extra_read - 24) / 3;
   } else {
      if (info.bpp < 16)
         psize = (info.offset - info.extra_read - info.hsz) >> 2;
   }
   if (psize == 0) {
      // accept some number of extra bytes after the header, but if the offset points either to before
      // the header ends or implies a large amount of extra data, reject the file as malformed
      int bytes_read_so_far = s->callback_already_read + (int)(s->img_buffer - s->img_buffer_original);
      int header_limit = 1024; // max we actually read is below 256 bytes currently.
      int extra_data_limit = 256*4; // what ordinarily goes here is a palette; 256 entries*4 bytes is its max size.
      if (bytes_read_so_far <= 0 || bytes_read_so_far > header_limit) {
         return xi__errpuc("bad header", "Corrupt BMP");
      }
      // we established that bytes_read_so_far is positive and sensible.
      // the first half of this test rejects offsets that are either too small positives, or
      // negative, and guarantees that info.offset >= bytes_read_so_far > 0. this in turn
      // ensures the number computed in the second half of the test can't overflow.
      if (info.offset < bytes_read_so_far || info.offset - bytes_read_so_far > extra_data_limit) {
         return xi__errpuc("bad offset", "Corrupt BMP");
      } else {
         xi__skip(s, info.offset - bytes_read_so_far);
      }
   }

   if (info.bpp == 24 && ma == 0xff000000)
      s->img_n = 3;
   else
      s->img_n = ma ? 4 : 3;
   if (req_comp && req_comp >= 3) // we can directly decode 3 or 4
      target = req_comp;
   else
      target = s->img_n; // if they want monochrome, we'll post-convert

   // sanity-check size
   if (!xi__mad3sizes_valid(target, s->img_x, s->img_y, 0))
      return xi__errpuc("too large", "Corrupt BMP");

   out = (xi_uc *) xi__malloc_mad3(target, s->img_x, s->img_y, 0);
   if (!out) return xi__errpuc("outofmem", "Out of memory");
   if (info.bpp < 16) {
      int z=0;
      if (psize == 0 || psize > 256) { XI_FREE(out); return xi__errpuc("invalid", "Corrupt BMP"); }
      for (i=0; i < psize; ++i) {
         pal[i][2] = xi__get8(s);
         pal[i][1] = xi__get8(s);
         pal[i][0] = xi__get8(s);
         if (info.hsz != 12) xi__get8(s);
         pal[i][3] = 255;
      }
      xi__skip(s, info.offset - info.extra_read - info.hsz - psize * (info.hsz == 12 ? 3 : 4));
      if (info.bpp == 1) width = (s->img_x + 7) >> 3;
      else if (info.bpp == 4) width = (s->img_x + 1) >> 1;
      else if (info.bpp == 8) width = s->img_x;
      else { XI_FREE(out); return xi__errpuc("bad bpp", "Corrupt BMP"); }
      pad = (-width)&3;
      if (info.bpp == 1) {
         for (j=0; j < (int) s->img_y; ++j) {
            int bit_offset = 7, v = xi__get8(s);
            for (i=0; i < (int) s->img_x; ++i) {
               int color = (v>>bit_offset)&0x1;
               out[z++] = pal[color][0];
               out[z++] = pal[color][1];
               out[z++] = pal[color][2];
               if (target == 4) out[z++] = 255;
               if (i+1 == (int) s->img_x) break;
               if((--bit_offset) < 0) {
                  bit_offset = 7;
                  v = xi__get8(s);
               }
            }
            xi__skip(s, pad);
         }
      } else {
         for (j=0; j < (int) s->img_y; ++j) {
            for (i=0; i < (int) s->img_x; i += 2) {
               int v=xi__get8(s),v2=0;
               if (info.bpp == 4) {
                  v2 = v & 15;
                  v >>= 4;
               }
               out[z++] = pal[v][0];
               out[z++] = pal[v][1];
               out[z++] = pal[v][2];
               if (target == 4) out[z++] = 255;
               if (i+1 == (int) s->img_x) break;
               v = (info.bpp == 8) ? xi__get8(s) : v2;
               out[z++] = pal[v][0];
               out[z++] = pal[v][1];
               out[z++] = pal[v][2];
               if (target == 4) out[z++] = 255;
            }
            xi__skip(s, pad);
         }
      }
   } else {
      int rshift=0,gshift=0,bshift=0,ashift=0,rcount=0,gcount=0,bcount=0,acount=0;
      int z = 0;
      int easy=0;
      xi__skip(s, info.offset - info.extra_read - info.hsz);
      if (info.bpp == 24) width = 3 * s->img_x;
      else if (info.bpp == 16) width = 2*s->img_x;
      else /* bpp = 32 and pad = 0 */ width=0;
      pad = (-width) & 3;
      if (info.bpp == 24) {
         easy = 1;
      } else if (info.bpp == 32) {
         if (mb == 0xff && mg == 0xff00 && mr == 0x00ff0000 && ma == 0xff000000)
            easy = 2;
      }
      if (!easy) {
         if (!mr || !mg || !mb) { XI_FREE(out); return xi__errpuc("bad masks", "Corrupt BMP"); }
         // right shift amt to put high bit in position #7
         rshift = xi__high_bit(mr)-7; rcount = xi__bitcount(mr);
         gshift = xi__high_bit(mg)-7; gcount = xi__bitcount(mg);
         bshift = xi__high_bit(mb)-7; bcount = xi__bitcount(mb);
         ashift = xi__high_bit(ma)-7; acount = xi__bitcount(ma);
         if (rcount > 8 || gcount > 8 || bcount > 8 || acount > 8) { XI_FREE(out); return xi__errpuc("bad masks", "Corrupt BMP"); }
      }
      for (j=0; j < (int) s->img_y; ++j) {
         if (easy) {
            for (i=0; i < (int) s->img_x; ++i) {
               unsigned char a;
               out[z+2] = xi__get8(s);
               out[z+1] = xi__get8(s);
               out[z+0] = xi__get8(s);
               z += 3;
               a = (easy == 2 ? xi__get8(s) : 255);
               all_a |= a;
               if (target == 4) out[z++] = a;
            }
         } else {
            int bpp = info.bpp;
            for (i=0; i < (int) s->img_x; ++i) {
               xi__uint32 v = (bpp == 16 ? (xi__uint32) xi__get16le(s) : xi__get32le(s));
               unsigned int a;
               out[z++] = XI__BYTECAST(xi__shiftsigned(v & mr, rshift, rcount));
               out[z++] = XI__BYTECAST(xi__shiftsigned(v & mg, gshift, gcount));
               out[z++] = XI__BYTECAST(xi__shiftsigned(v & mb, bshift, bcount));
               a = (ma ? xi__shiftsigned(v & ma, ashift, acount) : 255);
               all_a |= a;
               if (target == 4) out[z++] = XI__BYTECAST(a);
            }
         }
         xi__skip(s, pad);
      }
   }

   // if alpha channel is all 0s, replace with all 255s
   if (target == 4 && all_a == 0)
      for (i=4*s->img_x*s->img_y-1; i >= 0; i -= 4)
         out[i] = 255;

   if (flip_vertically) {
      xi_uc t;
      for (j=0; j < (int) s->img_y>>1; ++j) {
         xi_uc *p1 = out +      j     *s->img_x*target;
         xi_uc *p2 = out + (s->img_y-1-j)*s->img_x*target;
         for (i=0; i < (int) s->img_x*target; ++i) {
            t = p1[i]; p1[i] = p2[i]; p2[i] = t;
         }
      }
   }

   if (req_comp && req_comp != target) {
      out = xi__convert_format(out, target, req_comp, s->img_x, s->img_y);
      if (out == NULL) return out; // xi__convert_format frees input on failure
   }

   *x = s->img_x;
   *y = s->img_y;
   if (comp) *comp = s->img_n;
   return out;
}
#endif

// Targa Truevision - TGA
// by Jonathan Dummer
#ifndef XI_NO_TGA
// returns XI_rgb or whatever, 0 on error
static int xi__tga_get_comp(int bits_per_pixel, int is_grey, int* is_rgb16)
{
   // only RGB or RGBA (incl. 16bit) or grey allowed
   if (is_rgb16) *is_rgb16 = 0;
   switch(bits_per_pixel) {
      case 8:  return XI_grey;
      case 16: if(is_grey) return XI_grey_alpha;
               // fallthrough
      case 15: if(is_rgb16) *is_rgb16 = 1;
               return XI_rgb;
      case 24: // fallthrough
      case 32: return bits_per_pixel/8;
      default: return 0;
   }
}

static int xi__tga_info(xi__context *s, int *x, int *y, int *comp)
{
    int tga_w, tga_h, tga_comp, tga_image_type, tga_bits_per_pixel, tga_colormap_bpp;
    int sz, tga_colormap_type;
    xi__get8(s);                   // discard Offset
    tga_colormap_type = xi__get8(s); // colormap type
    if( tga_colormap_type > 1 ) {
        xi__rewind(s);
        return 0;      // only RGB or indexed allowed
    }
    tga_image_type = xi__get8(s); // image type
    if ( tga_colormap_type == 1 ) { // colormapped (paletted) image
        if (tga_image_type != 1 && tga_image_type != 9) {
            xi__rewind(s);
            return 0;
        }
        xi__skip(s,4);       // skip index of first colormap entry and number of entries
        sz = xi__get8(s);    //   check bits per palette color entry
        if ( (sz != 8) && (sz != 15) && (sz != 16) && (sz != 24) && (sz != 32) ) {
            xi__rewind(s);
            return 0;
        }
        xi__skip(s,4);       // skip image x and y origin
        tga_colormap_bpp = sz;
    } else { // "normal" image w/o colormap - only RGB or grey allowed, +/- RLE
        if ( (tga_image_type != 2) && (tga_image_type != 3) && (tga_image_type != 10) && (tga_image_type != 11) ) {
            xi__rewind(s);
            return 0; // only RGB or grey allowed, +/- RLE
        }
        xi__skip(s,9); // skip colormap specification and image x/y origin
        tga_colormap_bpp = 0;
    }
    tga_w = xi__get16le(s);
    if( tga_w < 1 ) {
        xi__rewind(s);
        return 0;   // test width
    }
    tga_h = xi__get16le(s);
    if( tga_h < 1 ) {
        xi__rewind(s);
        return 0;   // test height
    }
    tga_bits_per_pixel = xi__get8(s); // bits per pixel
    xi__get8(s); // ignore alpha bits
    if (tga_colormap_bpp != 0) {
        if((tga_bits_per_pixel != 8) && (tga_bits_per_pixel != 16)) {
            // when using a colormap, tga_bits_per_pixel is the size of the indexes
            // I don't think anything but 8 or 16bit indexes makes sense
            xi__rewind(s);
            return 0;
        }
        tga_comp = xi__tga_get_comp(tga_colormap_bpp, 0, NULL);
    } else {
        tga_comp = xi__tga_get_comp(tga_bits_per_pixel, (tga_image_type == 3) || (tga_image_type == 11), NULL);
    }
    if(!tga_comp) {
      xi__rewind(s);
      return 0;
    }
    if (x) *x = tga_w;
    if (y) *y = tga_h;
    if (comp) *comp = tga_comp;
    return 1;                   // seems to have passed everything
}

static int xi__tga_test(xi__context *s)
{
   int res = 0;
   int sz, tga_color_type;
   xi__get8(s);      //   discard Offset
   tga_color_type = xi__get8(s);   //   color type
   if ( tga_color_type > 1 ) goto errorEnd;   //   only RGB or indexed allowed
   sz = xi__get8(s);   //   image type
   if ( tga_color_type == 1 ) { // colormapped (paletted) image
      if (sz != 1 && sz != 9) goto errorEnd; // colortype 1 demands image type 1 or 9
      xi__skip(s,4);       // skip index of first colormap entry and number of entries
      sz = xi__get8(s);    //   check bits per palette color entry
      if ( (sz != 8) && (sz != 15) && (sz != 16) && (sz != 24) && (sz != 32) ) goto errorEnd;
      xi__skip(s,4);       // skip image x and y origin
   } else { // "normal" image w/o colormap
      if ( (sz != 2) && (sz != 3) && (sz != 10) && (sz != 11) ) goto errorEnd; // only RGB or grey allowed, +/- RLE
      xi__skip(s,9); // skip colormap specification and image x/y origin
   }
   if ( xi__get16le(s) < 1 ) goto errorEnd;      //   test width
   if ( xi__get16le(s) < 1 ) goto errorEnd;      //   test height
   sz = xi__get8(s);   //   bits per pixel
   if ( (tga_color_type == 1) && (sz != 8) && (sz != 16) ) goto errorEnd; // for colormapped images, bpp is size of an index
   if ( (sz != 8) && (sz != 15) && (sz != 16) && (sz != 24) && (sz != 32) ) goto errorEnd;

   res = 1; // if we got this far, everything's good and we can return 1 instead of 0

errorEnd:
   xi__rewind(s);
   return res;
}

// read 16bit value and convert to 24bit RGB
static void xi__tga_read_rgb16(xi__context *s, xi_uc* out)
{
   xi__uint16 px = (xi__uint16)xi__get16le(s);
   xi__uint16 fiveBitMask = 31;
   // we have 3 channels with 5bits each
   int r = (px >> 10) & fiveBitMask;
   int g = (px >> 5) & fiveBitMask;
   int b = px & fiveBitMask;
   // Note that this saves the data in RGB(A) order, so it doesn't need to be swapped later
   out[0] = (xi_uc)((r * 255)/31);
   out[1] = (xi_uc)((g * 255)/31);
   out[2] = (xi_uc)((b * 255)/31);

   // some people claim that the most significant bit might be used for alpha
   // (possibly if an alpha-bit is set in the "image descriptor byte")
   // but that only made 16bit test images completely translucent..
   // so let's treat all 15 and 16bit TGAs as RGB with no alpha.
}

static void *xi__tga_load(xi__context *s, int *x, int *y, int *comp, int req_comp, xi__result_info *ri)
{
   //   read in the TGA header stuff
   int tga_offset = xi__get8(s);
   int tga_indexed = xi__get8(s);
   int tga_image_type = xi__get8(s);
   int tga_is_RLE = 0;
   int tga_palette_start = xi__get16le(s);
   int tga_palette_len = xi__get16le(s);
   int tga_palette_bits = xi__get8(s);
   int tga_x_origin = xi__get16le(s);
   int tga_y_origin = xi__get16le(s);
   int tga_width = xi__get16le(s);
   int tga_height = xi__get16le(s);
   int tga_bits_per_pixel = xi__get8(s);
   int tga_comp, tga_rgb16=0;
   int tga_inverted = xi__get8(s);
   // int tga_alpha_bits = tga_inverted & 15; // the 4 lowest bits - unused (useless?)
   //   image data
   unsigned char *tga_data;
   unsigned char *tga_palette = NULL;
   int i, j;
   unsigned char raw_data[4] = {0};
   int RLE_count = 0;
   int RLE_repeating = 0;
   int read_next_pixel = 1;
   XI_NOTUSED(ri);
   XI_NOTUSED(tga_x_origin); // @TODO
   XI_NOTUSED(tga_y_origin); // @TODO

   if (tga_height > XI_MAX_DIMENSIONS) return xi__errpuc("too large","Very large image (corrupt?)");
   if (tga_width > XI_MAX_DIMENSIONS) return xi__errpuc("too large","Very large image (corrupt?)");

   //   do a tiny bit of precessing
   if ( tga_image_type >= 8 )
   {
      tga_image_type -= 8;
      tga_is_RLE = 1;
   }
   tga_inverted = 1 - ((tga_inverted >> 5) & 1);

   //   If I'm paletted, then I'll use the number of bits from the palette
   if ( tga_indexed ) tga_comp = xi__tga_get_comp(tga_palette_bits, 0, &tga_rgb16);
   else tga_comp = xi__tga_get_comp(tga_bits_per_pixel, (tga_image_type == 3), &tga_rgb16);

   if(!tga_comp) // shouldn't really happen, xi__tga_test() should have ensured basic consistency
      return xi__errpuc("bad format", "Can't find out TGA pixelformat");

   //   tga info
   *x = tga_width;
   *y = tga_height;
   if (comp) *comp = tga_comp;

   if (!xi__mad3sizes_valid(tga_width, tga_height, tga_comp, 0))
      return xi__errpuc("too large", "Corrupt TGA");

   tga_data = (unsigned char*)xi__malloc_mad3(tga_width, tga_height, tga_comp, 0);
   if (!tga_data) return xi__errpuc("outofmem", "Out of memory");

   // skip to the data's starting position (offset usually = 0)
   xi__skip(s, tga_offset );

   if ( !tga_indexed && !tga_is_RLE && !tga_rgb16 ) {
      for (i=0; i < tga_height; ++i) {
         int row = tga_inverted ? tga_height -i - 1 : i;
         xi_uc *tga_row = tga_data + row*tga_width*tga_comp;
         xi__getn(s, tga_row, tga_width * tga_comp);
      }
   } else  {
      //   do I need to load a palette?
      if ( tga_indexed)
      {
         if (tga_palette_len == 0) {  /* you have to have at least one entry! */
            XI_FREE(tga_data);
            return xi__errpuc("bad palette", "Corrupt TGA");
         }

         //   any data to skip? (offset usually = 0)
         xi__skip(s, tga_palette_start );
         //   load the palette
         tga_palette = (unsigned char*)xi__malloc_mad2(tga_palette_len, tga_comp, 0);
         if (!tga_palette) {
            XI_FREE(tga_data);
            return xi__errpuc("outofmem", "Out of memory");
         }
         if (tga_rgb16) {
            xi_uc *pal_entry = tga_palette;
            XI_ASSERT(tga_comp == XI_rgb);
            for (i=0; i < tga_palette_len; ++i) {
               xi__tga_read_rgb16(s, pal_entry);
               pal_entry += tga_comp;
            }
         } else if (!xi__getn(s, tga_palette, tga_palette_len * tga_comp)) {
               XI_FREE(tga_data);
               XI_FREE(tga_palette);
               return xi__errpuc("bad palette", "Corrupt TGA");
         }
      }
      //   load the data
      for (i=0; i < tga_width * tga_height; ++i)
      {
         //   if I'm in RLE mode, do I need to get a RLE xi__pngchunk?
         if ( tga_is_RLE )
         {
            if ( RLE_count == 0 )
            {
               //   yep, get the next byte as a RLE command
               int RLE_cmd = xi__get8(s);
               RLE_count = 1 + (RLE_cmd & 127);
               RLE_repeating = RLE_cmd >> 7;
               read_next_pixel = 1;
            } else if ( !RLE_repeating )
            {
               read_next_pixel = 1;
            }
         } else
         {
            read_next_pixel = 1;
         }
         //   OK, if I need to read a pixel, do it now
         if ( read_next_pixel )
         {
            //   load however much data we did have
            if ( tga_indexed )
            {
               // read in index, then perform the lookup
               int pal_idx = (tga_bits_per_pixel == 8) ? xi__get8(s) : xi__get16le(s);
               if ( pal_idx >= tga_palette_len ) {
                  // invalid index
                  pal_idx = 0;
               }
               pal_idx *= tga_comp;
               for (j = 0; j < tga_comp; ++j) {
                  raw_data[j] = tga_palette[pal_idx+j];
               }
            } else if(tga_rgb16) {
               XI_ASSERT(tga_comp == XI_rgb);
               xi__tga_read_rgb16(s, raw_data);
            } else {
               //   read in the data raw
               for (j = 0; j < tga_comp; ++j) {
                  raw_data[j] = xi__get8(s);
               }
            }
            //   clear the reading flag for the next pixel
            read_next_pixel = 0;
         } // end of reading a pixel

         // copy data
         for (j = 0; j < tga_comp; ++j)
           tga_data[i*tga_comp+j] = raw_data[j];

         //   in case we're in RLE mode, keep counting down
         --RLE_count;
      }
      //   do I need to invert the image?
      if ( tga_inverted )
      {
         for (j = 0; j*2 < tga_height; ++j)
         {
            int index1 = j * tga_width * tga_comp;
            int index2 = (tga_height - 1 - j) * tga_width * tga_comp;
            for (i = tga_width * tga_comp; i > 0; --i)
            {
               unsigned char temp = tga_data[index1];
               tga_data[index1] = tga_data[index2];
               tga_data[index2] = temp;
               ++index1;
               ++index2;
            }
         }
      }
      //   clear my palette, if I had one
      if ( tga_palette != NULL )
      {
         XI_FREE( tga_palette );
      }
   }

   // swap RGB - if the source data was RGB16, it already is in the right order
   if (tga_comp >= 3 && !tga_rgb16)
   {
      unsigned char* tga_pixel = tga_data;
      for (i=0; i < tga_width * tga_height; ++i)
      {
         unsigned char temp = tga_pixel[0];
         tga_pixel[0] = tga_pixel[2];
         tga_pixel[2] = temp;
         tga_pixel += tga_comp;
      }
   }

   // convert to target component count
   if (req_comp && req_comp != tga_comp)
      tga_data = xi__convert_format(tga_data, tga_comp, req_comp, tga_width, tga_height);

   //   the things I do to get rid of an error message, and yet keep
   //   Microsoft's C compilers happy... [8^(
   tga_palette_start = tga_palette_len = tga_palette_bits =
         tga_x_origin = tga_y_origin = 0;
   XI_NOTUSED(tga_palette_start);
   //   OK, done
   return tga_data;
}
#endif

// *************************************************************************************************
// Photoshop PSD loader -- PD by Thatcher Ulrich, integration by Nicolas Schulz, tweaked by XI

#ifndef XI_NO_PSD
static int xi__psd_test(xi__context *s)
{
   int r = (xi__get32be(s) == 0x38425053);
   xi__rewind(s);
   return r;
}

static int xi__psd_decode_rle(xi__context *s, xi_uc *p, int pixelCount)
{
   int count, nleft, len;

   count = 0;
   while ((nleft = pixelCount - count) > 0) {
      len = xi__get8(s);
      if (len == 128) {
         // No-op.
      } else if (len < 128) {
         // Copy next len+1 bytes literally.
         len++;
         if (len > nleft) return 0; // corrupt data
         count += len;
         while (len) {
            *p = xi__get8(s);
            p += 4;
            len--;
         }
      } else if (len > 128) {
         xi_uc   val;
         // Next -len+1 bytes in the dest are replicated from next source byte.
         // (Interpret len as a negative 8-bit int.)
         len = 257 - len;
         if (len > nleft) return 0; // corrupt data
         val = xi__get8(s);
         count += len;
         while (len) {
            *p = val;
            p += 4;
            len--;
         }
      }
   }

   return 1;
}

static void *xi__psd_load(xi__context *s, int *x, int *y, int *comp, int req_comp, xi__result_info *ri, int bpc)
{
   int pixelCount;
   int channelCount, compression;
   int channel, i;
   int bitdepth;
   int w,h;
   xi_uc *out;
   XI_NOTUSED(ri);

   // Check identifier
   if (xi__get32be(s) != 0x38425053)   // "8BPS"
      return xi__errpuc("not PSD", "Corrupt PSD image");

   // Check file type version.
   if (xi__get16be(s) != 1)
      return xi__errpuc("wrong version", "Unsupported version of PSD image");

   // Skip 6 reserved bytes.
   xi__skip(s, 6 );

   // Read the number of channels (R, G, B, A, etc).
   channelCount = xi__get16be(s);
   if (channelCount < 0 || channelCount > 16)
      return xi__errpuc("wrong channel count", "Unsupported number of channels in PSD image");

   // Read the rows and columns of the image.
   h = xi__get32be(s);
   w = xi__get32be(s);

   if (h > XI_MAX_DIMENSIONS) return xi__errpuc("too large","Very large image (corrupt?)");
   if (w > XI_MAX_DIMENSIONS) return xi__errpuc("too large","Very large image (corrupt?)");

   // Make sure the depth is 8 bits.
   bitdepth = xi__get16be(s);
   if (bitdepth != 8 && bitdepth != 16)
      return xi__errpuc("unsupported bit depth", "PSD bit depth is not 8 or 16 bit");

   // Make sure the color mode is RGB.
   // Valid options are:
   //   0: Bitmap
   //   1: Grayscale
   //   2: Indexed color
   //   3: RGB color
   //   4: CMYK color
   //   7: Multichannel
   //   8: Duotone
   //   9: Lab color
   if (xi__get16be(s) != 3)
      return xi__errpuc("wrong color format", "PSD is not in RGB color format");

   // Skip the Mode Data.  (It's the palette for indexed color; other info for other modes.)
   xi__skip(s,xi__get32be(s) );

   // Skip the image resources.  (resolution, pen tool paths, etc)
   xi__skip(s, xi__get32be(s) );

   // Skip the reserved data.
   xi__skip(s, xi__get32be(s) );

   // Find out if the data is compressed.
   // Known values:
   //   0: no compression
   //   1: RLE compressed
   compression = xi__get16be(s);
   if (compression > 1)
      return xi__errpuc("bad compression", "PSD has an unknown compression format");

   // Check size
   if (!xi__mad3sizes_valid(4, w, h, 0))
      return xi__errpuc("too large", "Corrupt PSD");

   // Create the destination image.

   if (!compression && bitdepth == 16 && bpc == 16) {
      out = (xi_uc *) xi__malloc_mad3(8, w, h, 0);
      ri->bits_per_channel = 16;
   } else
      out = (xi_uc *) xi__malloc(4 * w*h);

   if (!out) return xi__errpuc("outofmem", "Out of memory");
   pixelCount = w*h;

   // Initialize the data to zero.
   //memset( out, 0, pixelCount * 4 );

   // Finally, the image data.
   if (compression) {
      // RLE as used by .PSD and .TIFF
      // Loop until you get the number of unpacked bytes you are expecting:
      //     Read the next source byte into n.
      //     If n is between 0 and 127 inclusive, copy the next n+1 bytes literally.
      //     Else if n is between -127 and -1 inclusive, copy the next byte -n+1 times.
      //     Else if n is 128, noop.
      // Endloop

      // The RLE-compressed data is preceded by a 2-byte data count for each row in the data,
      // which we're going to just skip.
      xi__skip(s, h * channelCount * 2 );

      // Read the RLE data by channel.
      for (channel = 0; channel < 4; channel++) {
         xi_uc *p;

         p = out+channel;
         if (channel >= channelCount) {
            // Fill this channel with default data.
            for (i = 0; i < pixelCount; i++, p += 4)
               *p = (channel == 3 ? 255 : 0);
         } else {
            // Read the RLE data.
            if (!xi__psd_decode_rle(s, p, pixelCount)) {
               XI_FREE(out);
               return xi__errpuc("corrupt", "bad RLE data");
            }
         }
      }

   } else {
      // We're at the raw image data.  It's each channel in order (Red, Green, Blue, Alpha, ...)
      // where each channel consists of an 8-bit (or 16-bit) value for each pixel in the image.

      // Read the data by channel.
      for (channel = 0; channel < 4; channel++) {
         if (channel >= channelCount) {
            // Fill this channel with default data.
            if (bitdepth == 16 && bpc == 16) {
               xi__uint16 *q = ((xi__uint16 *) out) + channel;
               xi__uint16 val = channel == 3 ? 65535 : 0;
               for (i = 0; i < pixelCount; i++, q += 4)
                  *q = val;
            } else {
               xi_uc *p = out+channel;
               xi_uc val = channel == 3 ? 255 : 0;
               for (i = 0; i < pixelCount; i++, p += 4)
                  *p = val;
            }
         } else {
            if (ri->bits_per_channel == 16) {    // output bpc
               xi__uint16 *q = ((xi__uint16 *) out) + channel;
               for (i = 0; i < pixelCount; i++, q += 4)
                  *q = (xi__uint16) xi__get16be(s);
            } else {
               xi_uc *p = out+channel;
               if (bitdepth == 16) {  // input bpc
                  for (i = 0; i < pixelCount; i++, p += 4)
                     *p = (xi_uc) (xi__get16be(s) >> 8);
               } else {
                  for (i = 0; i < pixelCount; i++, p += 4)
                     *p = xi__get8(s);
               }
            }
         }
      }
   }

   // remove weird white matte from PSD
   if (channelCount >= 4) {
      if (ri->bits_per_channel == 16) {
         for (i=0; i < w*h; ++i) {
            xi__uint16 *pixel = (xi__uint16 *) out + 4*i;
            if (pixel[3] != 0 && pixel[3] != 65535) {
               float a = pixel[3] / 65535.0f;
               float ra = 1.0f / a;
               float inv_a = 65535.0f * (1 - ra);
               pixel[0] = (xi__uint16) (pixel[0]*ra + inv_a);
               pixel[1] = (xi__uint16) (pixel[1]*ra + inv_a);
               pixel[2] = (xi__uint16) (pixel[2]*ra + inv_a);
            }
         }
      } else {
         for (i=0; i < w*h; ++i) {
            unsigned char *pixel = out + 4*i;
            if (pixel[3] != 0 && pixel[3] != 255) {
               float a = pixel[3] / 255.0f;
               float ra = 1.0f / a;
               float inv_a = 255.0f * (1 - ra);
               pixel[0] = (unsigned char) (pixel[0]*ra + inv_a);
               pixel[1] = (unsigned char) (pixel[1]*ra + inv_a);
               pixel[2] = (unsigned char) (pixel[2]*ra + inv_a);
            }
         }
      }
   }

   // convert to desired output format
   if (req_comp && req_comp != 4) {
      if (ri->bits_per_channel == 16)
         out = (xi_uc *) xi__convert_format16((xi__uint16 *) out, 4, req_comp, w, h);
      else
         out = xi__convert_format(out, 4, req_comp, w, h);
      if (out == NULL) return out; // xi__convert_format frees input on failure
   }

   if (comp) *comp = 4;
   *y = h;
   *x = w;

   return out;
}
#endif

// *************************************************************************************************
// Softimage PIC loader
// by Tom Seddon
//
// See http://softimage.wiki.softimage.com/index.php/INFO:_PIC_file_format
// See http://ozviz.wasp.uwa.edu.au/~pbourke/dataformats/softimagepic/

#ifndef XI_NO_PIC
static int xi__pic_is4(xi__context *s,const char *str)
{
   int i;
   for (i=0; i<4; ++i)
      if (xi__get8(s) != (xi_uc)str[i])
         return 0;

   return 1;
}

static int xi__pic_test_core(xi__context *s)
{
   int i;

   if (!xi__pic_is4(s,"\x53\x80\xF6\x34"))
      return 0;

   for(i=0;i<84;++i)
      xi__get8(s);

   if (!xi__pic_is4(s,"PICT"))
      return 0;

   return 1;
}

typedef struct
{
   xi_uc size,type,channel;
} xi__pic_packet;

static xi_uc *xi__readval(xi__context *s, int channel, xi_uc *dest)
{
   int mask=0x80, i;

   for (i=0; i<4; ++i, mask>>=1) {
      if (channel & mask) {
         if (xi__at_eof(s)) return xi__errpuc("bad file","PIC file too short");
         dest[i]=xi__get8(s);
      }
   }

   return dest;
}

static void xi__copyval(int channel,xi_uc *dest,const xi_uc *src)
{
   int mask=0x80,i;

   for (i=0;i<4; ++i, mask>>=1)
      if (channel&mask)
         dest[i]=src[i];
}

static xi_uc *xi__pic_load_core(xi__context *s,int width,int height,int *comp, xi_uc *result)
{
   int act_comp=0,num_packets=0,y,chained;
   xi__pic_packet packets[10];

   // this will (should...) cater for even some bizarre stuff like having data
    // for the same channel in multiple packets.
   do {
      xi__pic_packet *packet;

      if (num_packets==sizeof(packets)/sizeof(packets[0]))
         return xi__errpuc("bad format","too many packets");

      packet = &packets[num_packets++];

      chained = xi__get8(s);
      packet->size    = xi__get8(s);
      packet->type    = xi__get8(s);
      packet->channel = xi__get8(s);

      act_comp |= packet->channel;

      if (xi__at_eof(s))          return xi__errpuc("bad file","file too short (reading packets)");
      if (packet->size != 8)  return xi__errpuc("bad format","packet isn't 8bpp");
   } while (chained);

   *comp = (act_comp & 0x10 ? 4 : 3); // has alpha channel?

   for(y=0; y<height; ++y) {
      int packet_idx;

      for(packet_idx=0; packet_idx < num_packets; ++packet_idx) {
         xi__pic_packet *packet = &packets[packet_idx];
         xi_uc *dest = result+y*width*4;

         switch (packet->type) {
            default:
               return xi__errpuc("bad format","packet has bad compression type");

            case 0: {//uncompressed
               int x;

               for(x=0;x<width;++x, dest+=4)
                  if (!xi__readval(s,packet->channel,dest))
                     return 0;
               break;
            }

            case 1://Pure RLE
               {
                  int left=width, i;

                  while (left>0) {
                     xi_uc count,value[4];

                     count=xi__get8(s);
                     if (xi__at_eof(s))   return xi__errpuc("bad file","file too short (pure read count)");

                     if (count > left)
                        count = (xi_uc) left;

                     if (!xi__readval(s,packet->channel,value))  return 0;

                     for(i=0; i<count; ++i,dest+=4)
                        xi__copyval(packet->channel,dest,value);
                     left -= count;
                  }
               }
               break;

            case 2: {//Mixed RLE
               int left=width;
               while (left>0) {
                  int count = xi__get8(s), i;
                  if (xi__at_eof(s))  return xi__errpuc("bad file","file too short (mixed read count)");

                  if (count >= 128) { // Repeated
                     xi_uc value[4];

                     if (count==128)
                        count = xi__get16be(s);
                     else
                        count -= 127;
                     if (count > left)
                        return xi__errpuc("bad file","scanline overrun");

                     if (!xi__readval(s,packet->channel,value))
                        return 0;

                     for(i=0;i<count;++i, dest += 4)
                        xi__copyval(packet->channel,dest,value);
                  } else { // Raw
                     ++count;
                     if (count>left) return xi__errpuc("bad file","scanline overrun");

                     for(i=0;i<count;++i, dest+=4)
                        if (!xi__readval(s,packet->channel,dest))
                           return 0;
                  }
                  left-=count;
               }
               break;
            }
         }
      }
   }

   return result;
}

static void *xi__pic_load(xi__context *s,int *px,int *py,int *comp,int req_comp, xi__result_info *ri)
{
   xi_uc *result;
   int i, x,y, internal_comp;
   XI_NOTUSED(ri);

   if (!comp) comp = &internal_comp;

   for (i=0; i<92; ++i)
      xi__get8(s);

   x = xi__get16be(s);
   y = xi__get16be(s);

   if (y > XI_MAX_DIMENSIONS) return xi__errpuc("too large","Very large image (corrupt?)");
   if (x > XI_MAX_DIMENSIONS) return xi__errpuc("too large","Very large image (corrupt?)");

   if (xi__at_eof(s))  return xi__errpuc("bad file","file too short (pic header)");
   if (!xi__mad3sizes_valid(x, y, 4, 0)) return xi__errpuc("too large", "PIC image too large to decode");

   xi__get32be(s); //skip `ratio'
   xi__get16be(s); //skip `fields'
   xi__get16be(s); //skip `pad'

   // intermediate buffer is RGBA
   result = (xi_uc *) xi__malloc_mad3(x, y, 4, 0);
   if (!result) return xi__errpuc("outofmem", "Out of memory");
   memset(result, 0xff, x*y*4);

   if (!xi__pic_load_core(s,x,y,comp, result)) {
      XI_FREE(result);
      result=0;
   }
   *px = x;
   *py = y;
   if (req_comp == 0) req_comp = *comp;
   result=xi__convert_format(result,4,req_comp,x,y);

   return result;
}

static int xi__pic_test(xi__context *s)
{
   int r = xi__pic_test_core(s);
   xi__rewind(s);
   return r;
}
#endif

// *************************************************************************************************
// GIF loader -- public domain by Jean-Marc Lienher -- simplified/shrunk by stb

#ifndef XI_NO_GIF
typedef struct
{
   xi__int16 prefix;
   xi_uc first;
   xi_uc suffix;
} xi__gif_lzw;

typedef struct
{
   int w,h;
   xi_uc *out;                 // output buffer (always 4 components)
   xi_uc *background;          // The current "background" as far as a gif is concerned
   xi_uc *history;
   int flags, bgindex, ratio, transparent, eflags;
   xi_uc  pal[256][4];
   xi_uc lpal[256][4];
   xi__gif_lzw codes[8192];
   xi_uc *color_table;
   int parse, step;
   int lflags;
   int start_x, start_y;
   int max_x, max_y;
   int cur_x, cur_y;
   int line_size;
   int delay;
} xi__gif;

static int xi__gif_test_raw(xi__context *s)
{
   int sz;
   if (xi__get8(s) != 'G' || xi__get8(s) != 'I' || xi__get8(s) != 'F' || xi__get8(s) != '8') return 0;
   sz = xi__get8(s);
   if (sz != '9' && sz != '7') return 0;
   if (xi__get8(s) != 'a') return 0;
   return 1;
}

static int xi__gif_test(xi__context *s)
{
   int r = xi__gif_test_raw(s);
   xi__rewind(s);
   return r;
}

static void xi__gif_parse_colortable(xi__context *s, xi_uc pal[256][4], int num_entries, int transp)
{
   int i;
   for (i=0; i < num_entries; ++i) {
      pal[i][2] = xi__get8(s);
      pal[i][1] = xi__get8(s);
      pal[i][0] = xi__get8(s);
      pal[i][3] = transp == i ? 0 : 255;
   }
}

static int xi__gif_header(xi__context *s, xi__gif *g, int *comp, int is_info)
{
   xi_uc version;
   if (xi__get8(s) != 'G' || xi__get8(s) != 'I' || xi__get8(s) != 'F' || xi__get8(s) != '8')
      return xi__err("not GIF", "Corrupt GIF");

   version = xi__get8(s);
   if (version != '7' && version != '9')    return xi__err("not GIF", "Corrupt GIF");
   if (xi__get8(s) != 'a')                return xi__err("not GIF", "Corrupt GIF");

   xi__g_failure_reason = "";
   g->w = xi__get16le(s);
   g->h = xi__get16le(s);
   g->flags = xi__get8(s);
   g->bgindex = xi__get8(s);
   g->ratio = xi__get8(s);
   g->transparent = -1;

   if (g->w > XI_MAX_DIMENSIONS) return xi__err("too large","Very large image (corrupt?)");
   if (g->h > XI_MAX_DIMENSIONS) return xi__err("too large","Very large image (corrupt?)");

   if (comp != 0) *comp = 4;  // can't actually tell whether it's 3 or 4 until we parse the comments

   if (is_info) return 1;

   if (g->flags & 0x80)
      xi__gif_parse_colortable(s,g->pal, 2 << (g->flags & 7), -1);

   return 1;
}

static int xi__gif_info_raw(xi__context *s, int *x, int *y, int *comp)
{
   xi__gif* g = (xi__gif*) xi__malloc(sizeof(xi__gif));
   if (!g) return xi__err("outofmem", "Out of memory");
   if (!xi__gif_header(s, g, comp, 1)) {
      XI_FREE(g);
      xi__rewind( s );
      return 0;
   }
   if (x) *x = g->w;
   if (y) *y = g->h;
   XI_FREE(g);
   return 1;
}

static void xi__out_gif_code(xi__gif *g, xi__uint16 code)
{
   xi_uc *p, *c;
   int idx;

   // recurse to decode the prefixes, since the linked-list is backwards,
   // and working backwards through an interleaved image would be nasty
   if (g->codes[code].prefix >= 0)
      xi__out_gif_code(g, g->codes[code].prefix);

   if (g->cur_y >= g->max_y) return;

   idx = g->cur_x + g->cur_y;
   p = &g->out[idx];
   g->history[idx / 4] = 1;

   c = &g->color_table[g->codes[code].suffix * 4];
   if (c[3] > 128) { // don't render transparent pixels;
      p[0] = c[2];
      p[1] = c[1];
      p[2] = c[0];
      p[3] = c[3];
   }
   g->cur_x += 4;

   if (g->cur_x >= g->max_x) {
      g->cur_x = g->start_x;
      g->cur_y += g->step;

      while (g->cur_y >= g->max_y && g->parse > 0) {
         g->step = (1 << g->parse) * g->line_size;
         g->cur_y = g->start_y + (g->step >> 1);
         --g->parse;
      }
   }
}

static xi_uc *xi__process_gif_raster(xi__context *s, xi__gif *g)
{
   xi_uc lzw_cs;
   xi__int32 len, init_code;
   xi__uint32 first;
   xi__int32 codesize, codemask, avail, oldcode, bits, valid_bits, clear;
   xi__gif_lzw *p;

   lzw_cs = xi__get8(s);
   if (lzw_cs > 12) return NULL;
   clear = 1 << lzw_cs;
   first = 1;
   codesize = lzw_cs + 1;
   codemask = (1 << codesize) - 1;
   bits = 0;
   valid_bits = 0;
   for (init_code = 0; init_code < clear; init_code++) {
      g->codes[init_code].prefix = -1;
      g->codes[init_code].first = (xi_uc) init_code;
      g->codes[init_code].suffix = (xi_uc) init_code;
   }

   // support no starting clear code
   avail = clear+2;
   oldcode = -1;

   len = 0;
   for(;;) {
      if (valid_bits < codesize) {
         if (len == 0) {
            len = xi__get8(s); // start new block
            if (len == 0)
               return g->out;
         }
         --len;
         bits |= (xi__int32) xi__get8(s) << valid_bits;
         valid_bits += 8;
      } else {
         xi__int32 code = bits & codemask;
         bits >>= codesize;
         valid_bits -= codesize;
         // @OPTIMIZE: is there some way we can accelerate the non-clear path?
         if (code == clear) {  // clear code
            codesize = lzw_cs + 1;
            codemask = (1 << codesize) - 1;
            avail = clear + 2;
            oldcode = -1;
            first = 0;
         } else if (code == clear + 1) { // end of stream code
            xi__skip(s, len);
            while ((len = xi__get8(s)) > 0)
               xi__skip(s,len);
            return g->out;
         } else if (code <= avail) {
            if (first) {
               return xi__errpuc("no clear code", "Corrupt GIF");
            }

            if (oldcode >= 0) {
               p = &g->codes[avail++];
               if (avail > 8192) {
                  return xi__errpuc("too many codes", "Corrupt GIF");
               }

               p->prefix = (xi__int16) oldcode;
               p->first = g->codes[oldcode].first;
               p->suffix = (code == avail) ? p->first : g->codes[code].first;
            } else if (code == avail)
               return xi__errpuc("illegal code in raster", "Corrupt GIF");

            xi__out_gif_code(g, (xi__uint16) code);

            if ((avail & codemask) == 0 && avail <= 0x0FFF) {
               codesize++;
               codemask = (1 << codesize) - 1;
            }

            oldcode = code;
         } else {
            return xi__errpuc("illegal code in raster", "Corrupt GIF");
         }
      }
   }
}

// this function is designed to support animated gifs, although stb_image doesn't support it
// two back is the image from two frames ago, used for a very specific disposal format
static xi_uc *xi__gif_load_next(xi__context *s, xi__gif *g, int *comp, int req_comp, xi_uc *two_back)
{
   int dispose;
   int first_frame;
   int pi;
   int pcount;
   XI_NOTUSED(req_comp);

   // on first frame, any non-written pixels get the background colour (non-transparent)
   first_frame = 0;
   if (g->out == 0) {
      if (!xi__gif_header(s, g, comp,0)) return 0; // xi__g_failure_reason set by xi__gif_header
      if (!xi__mad3sizes_valid(4, g->w, g->h, 0))
         return xi__errpuc("too large", "GIF image is too large");
      pcount = g->w * g->h;
      g->out = (xi_uc *) xi__malloc(4 * pcount);
      g->background = (xi_uc *) xi__malloc(4 * pcount);
      g->history = (xi_uc *) xi__malloc(pcount);
      if (!g->out || !g->background || !g->history)
         return xi__errpuc("outofmem", "Out of memory");

      // image is treated as "transparent" at the start - ie, nothing overwrites the current background;
      // background colour is only used for pixels that are not rendered first frame, after that "background"
      // color refers to the color that was there the previous frame.
      memset(g->out, 0x00, 4 * pcount);
      memset(g->background, 0x00, 4 * pcount); // state of the background (starts transparent)
      memset(g->history, 0x00, pcount);        // pixels that were affected previous frame
      first_frame = 1;
   } else {
      // second frame - how do we dispose of the previous one?
      dispose = (g->eflags & 0x1C) >> 2;
      pcount = g->w * g->h;

      if ((dispose == 3) && (two_back == 0)) {
         dispose = 2; // if I don't have an image to revert back to, default to the old background
      }

      if (dispose == 3) { // use previous graphic
         for (pi = 0; pi < pcount; ++pi) {
            if (g->history[pi]) {
               memcpy( &g->out[pi * 4], &two_back[pi * 4], 4 );
            }
         }
      } else if (dispose == 2) {
         // restore what was changed last frame to background before that frame;
         for (pi = 0; pi < pcount; ++pi) {
            if (g->history[pi]) {
               memcpy( &g->out[pi * 4], &g->background[pi * 4], 4 );
            }
         }
      } else {
         // This is a non-disposal case eithe way, so just
         // leave the pixels as is, and they will become the new background
         // 1: do not dispose
         // 0:  not specified.
      }

      // background is what out is after the undoing of the previou frame;
      memcpy( g->background, g->out, 4 * g->w * g->h );
   }

   // clear my history;
   memset( g->history, 0x00, g->w * g->h );        // pixels that were affected previous frame

   for (;;) {
      int tag = xi__get8(s);
      switch (tag) {
         case 0x2C: /* Image Descriptor */
         {
            xi__int32 x, y, w, h;
            xi_uc *o;

            x = xi__get16le(s);
            y = xi__get16le(s);
            w = xi__get16le(s);
            h = xi__get16le(s);
            if (((x + w) > (g->w)) || ((y + h) > (g->h)))
               return xi__errpuc("bad Image Descriptor", "Corrupt GIF");

            g->line_size = g->w * 4;
            g->start_x = x * 4;
            g->start_y = y * g->line_size;
            g->max_x   = g->start_x + w * 4;
            g->max_y   = g->start_y + h * g->line_size;
            g->cur_x   = g->start_x;
            g->cur_y   = g->start_y;

            // if the width of the specified rectangle is 0, that means
            // we may not see *any* pixels or the image is malformed;
            // to make sure this is caught, move the current y down to
            // max_y (which is what out_gif_code checks).
            if (w == 0)
               g->cur_y = g->max_y;

            g->lflags = xi__get8(s);

            if (g->lflags & 0x40) {
               g->step = 8 * g->line_size; // first interlaced spacing
               g->parse = 3;
            } else {
               g->step = g->line_size;
               g->parse = 0;
            }

            if (g->lflags & 0x80) {
               xi__gif_parse_colortable(s,g->lpal, 2 << (g->lflags & 7), g->eflags & 0x01 ? g->transparent : -1);
               g->color_table = (xi_uc *) g->lpal;
            } else if (g->flags & 0x80) {
               g->color_table = (xi_uc *) g->pal;
            } else
               return xi__errpuc("missing color table", "Corrupt GIF");

            o = xi__process_gif_raster(s, g);
            if (!o) return NULL;

            // if this was the first frame,
            pcount = g->w * g->h;
            if (first_frame && (g->bgindex > 0)) {
               // if first frame, any pixel not drawn to gets the background color
               for (pi = 0; pi < pcount; ++pi) {
                  if (g->history[pi] == 0) {
                     g->pal[g->bgindex][3] = 255; // just in case it was made transparent, undo that; It will be reset next frame if need be;
                     memcpy( &g->out[pi * 4], &g->pal[g->bgindex], 4 );
                  }
               }
            }

            return o;
         }

         case 0x21: // Comment Extension.
         {
            int len;
            int ext = xi__get8(s);
            if (ext == 0xF9) { // Graphic Control Extension.
               len = xi__get8(s);
               if (len == 4) {
                  g->eflags = xi__get8(s);
                  g->delay = 10 * xi__get16le(s); // delay - 1/100th of a second, saving as 1/1000ths.

                  // unset old transparent
                  if (g->transparent >= 0) {
                     g->pal[g->transparent][3] = 255;
                  }
                  if (g->eflags & 0x01) {
                     g->transparent = xi__get8(s);
                     if (g->transparent >= 0) {
                        g->pal[g->transparent][3] = 0;
                     }
                  } else {
                     // don't need transparent
                     xi__skip(s, 1);
                     g->transparent = -1;
                  }
               } else {
                  xi__skip(s, len);
                  break;
               }
            }
            while ((len = xi__get8(s)) != 0) {
               xi__skip(s, len);
            }
            break;
         }

         case 0x3B: // gif stream termination code
            return (xi_uc *) s; // using '1' causes warning on some compilers

         default:
            return xi__errpuc("unknown code", "Corrupt GIF");
      }
   }
}

static void *xi__load_gif_main_outofmem(xi__gif *g, xi_uc *out, int **delays)
{
   XI_FREE(g->out);
   XI_FREE(g->history);
   XI_FREE(g->background);

   if (out) XI_FREE(out);
   if (delays && *delays) XI_FREE(*delays);
   return xi__errpuc("outofmem", "Out of memory");
}

static void *xi__load_gif_main(xi__context *s, int **delays, int *x, int *y, int *z, int *comp, int req_comp)
{
   if (xi__gif_test(s)) {
      int layers = 0;
      xi_uc *u = 0;
      xi_uc *out = 0;
      xi_uc *two_back = 0;
      xi__gif g;
      int stride;
      int out_size = 0;
      int delays_size = 0;

      XI_NOTUSED(out_size);
      XI_NOTUSED(delays_size);

      memset(&g, 0, sizeof(g));
      if (delays) {
         *delays = 0;
      }

      do {
         u = xi__gif_load_next(s, &g, comp, req_comp, two_back);
         if (u == (xi_uc *) s) u = 0;  // end of animated gif marker

         if (u) {
            *x = g.w;
            *y = g.h;
            ++layers;
            stride = g.w * g.h * 4;

            if (out) {
               void *tmp = (xi_uc*) XI_REALLOC_SIZED( out, out_size, layers * stride );
               if (!tmp)
                  return xi__load_gif_main_outofmem(&g, out, delays);
               else {
                   out = (xi_uc*) tmp;
                   out_size = layers * stride;
               }

               if (delays) {
                  int *new_delays = (int*) XI_REALLOC_SIZED( *delays, delays_size, sizeof(int) * layers );
                  if (!new_delays)
                     return xi__load_gif_main_outofmem(&g, out, delays);
                  *delays = new_delays;
                  delays_size = layers * sizeof(int);
               }
            } else {
               out = (xi_uc*)xi__malloc( layers * stride );
               if (!out)
                  return xi__load_gif_main_outofmem(&g, out, delays);
               out_size = layers * stride;
               if (delays) {
                  *delays = (int*) xi__malloc( layers * sizeof(int) );
                  if (!*delays)
                     return xi__load_gif_main_outofmem(&g, out, delays);
                  delays_size = layers * sizeof(int);
               }
            }
            memcpy( out + ((layers - 1) * stride), u, stride );
            if (layers >= 2) {
               two_back = out - 2 * stride;
            }

            if (delays) {
               (*delays)[layers - 1U] = g.delay;
            }
         }
      } while (u != 0);

      // free temp buffer;
      XI_FREE(g.out);
      XI_FREE(g.history);
      XI_FREE(g.background);

      // do the final conversion after loading everything;
      if (req_comp && req_comp != 4)
         out = xi__convert_format(out, 4, req_comp, layers * g.w, g.h);

      *z = layers;
      return out;
   } else {
      return xi__errpuc("not GIF", "Image was not as a gif type.");
   }
}

static void *xi__gif_load(xi__context *s, int *x, int *y, int *comp, int req_comp, xi__result_info *ri)
{
   xi_uc *u = 0;
   xi__gif g;
   memset(&g, 0, sizeof(g));
   XI_NOTUSED(ri);

   u = xi__gif_load_next(s, &g, comp, req_comp, 0);
   if (u == (xi_uc *) s) u = 0;  // end of animated gif marker
   if (u) {
      *x = g.w;
      *y = g.h;

      // moved conversion to after successful load so that the same
      // can be done for multiple frames.
      if (req_comp && req_comp != 4)
         u = xi__convert_format(u, 4, req_comp, g.w, g.h);
   } else if (g.out) {
      // if there was an error and we allocated an image buffer, free it!
      XI_FREE(g.out);
   }

   // free buffers needed for multiple frame loading;
   XI_FREE(g.history);
   XI_FREE(g.background);

   return u;
}

static int xi__gif_info(xi__context *s, int *x, int *y, int *comp)
{
   return xi__gif_info_raw(s,x,y,comp);
}
#endif

// *************************************************************************************************
// Radiance RGBE HDR loader
// originally by Nicolas Schulz
#ifndef XI_NO_HDR
static int xi__hdr_test_core(xi__context *s, const char *signature)
{
   int i;
   for (i=0; signature[i]; ++i)
      if (xi__get8(s) != signature[i])
          return 0;
   xi__rewind(s);
   return 1;
}

static int xi__hdr_test(xi__context* s)
{
   int r = xi__hdr_test_core(s, "#?RADIANCE\n");
   xi__rewind(s);
   if(!r) {
       r = xi__hdr_test_core(s, "#?RGBE\n");
       xi__rewind(s);
   }
   return r;
}

#define XI__HDR_BUFLEN  1024
static char *xi__hdr_gettoken(xi__context *z, char *buffer)
{
   int len=0;
   char c = '\0';

   c = (char) xi__get8(z);

   while (!xi__at_eof(z) && c != '\n') {
      buffer[len++] = c;
      if (len == XI__HDR_BUFLEN-1) {
         // flush to end of line
         while (!xi__at_eof(z) && xi__get8(z) != '\n')
            ;
         break;
      }
      c = (char) xi__get8(z);
   }

   buffer[len] = 0;
   return buffer;
}

static void xi__hdr_convert(float *output, xi_uc *input, int req_comp)
{
   if ( input[3] != 0 ) {
      float f1;
      // Exponent
      f1 = (float) ldexp(1.0f, input[3] - (int)(128 + 8));
      if (req_comp <= 2)
         output[0] = (input[0] + input[1] + input[2]) * f1 / 3;
      else {
         output[0] = input[0] * f1;
         output[1] = input[1] * f1;
         output[2] = input[2] * f1;
      }
      if (req_comp == 2) output[1] = 1;
      if (req_comp == 4) output[3] = 1;
   } else {
      switch (req_comp) {
         case 4: output[3] = 1; /* fallthrough */
         case 3: output[0] = output[1] = output[2] = 0;
                 break;
         case 2: output[1] = 1; /* fallthrough */
         case 1: output[0] = 0;
                 break;
      }
   }
}

static float *xi__hdr_load(xi__context *s, int *x, int *y, int *comp, int req_comp, xi__result_info *ri)
{
   char buffer[XI__HDR_BUFLEN];
   char *token;
   int valid = 0;
   int width, height;
   xi_uc *scanline;
   float *hdr_data;
   int len;
   unsigned char count, value;
   int i, j, k, c1,c2, z;
   const char *headerToken;
   XI_NOTUSED(ri);

   // Check identifier
   headerToken = xi__hdr_gettoken(s,buffer);
   if (strcmp(headerToken, "#?RADIANCE") != 0 && strcmp(headerToken, "#?RGBE") != 0)
      return xi__errpf("not HDR", "Corrupt HDR image");

   // Parse header
   for(;;) {
      token = xi__hdr_gettoken(s,buffer);
      if (token[0] == 0) break;
      if (strcmp(token, "FORMAT=32-bit_rle_rgbe") == 0) valid = 1;
   }

   if (!valid)    return xi__errpf("unsupported format", "Unsupported HDR format");

   // Parse width and height
   // can't use sscanf() if we're not using stdio!
   token = xi__hdr_gettoken(s,buffer);
   if (strncmp(token, "-Y ", 3))  return xi__errpf("unsupported data layout", "Unsupported HDR format");
   token += 3;
   height = (int) strtol(token, &token, 10);
   while (*token == ' ') ++token;
   if (strncmp(token, "+X ", 3))  return xi__errpf("unsupported data layout", "Unsupported HDR format");
   token += 3;
   width = (int) strtol(token, NULL, 10);

   if (height > XI_MAX_DIMENSIONS) return xi__errpf("too large","Very large image (corrupt?)");
   if (width > XI_MAX_DIMENSIONS) return xi__errpf("too large","Very large image (corrupt?)");

   *x = width;
   *y = height;

   if (comp) *comp = 3;
   if (req_comp == 0) req_comp = 3;

   if (!xi__mad4sizes_valid(width, height, req_comp, sizeof(float), 0))
      return xi__errpf("too large", "HDR image is too large");

   // Read data
   hdr_data = (float *) xi__malloc_mad4(width, height, req_comp, sizeof(float), 0);
   if (!hdr_data)
      return xi__errpf("outofmem", "Out of memory");

   // Load image data
   // image data is stored as some number of sca
   if ( width < 8 || width >= 32768) {
      // Read flat data
      for (j=0; j < height; ++j) {
         for (i=0; i < width; ++i) {
            xi_uc rgbe[4];
           main_decode_loop:
            xi__getn(s, rgbe, 4);
            xi__hdr_convert(hdr_data + j * width * req_comp + i * req_comp, rgbe, req_comp);
         }
      }
   } else {
      // Read RLE-encoded data
      scanline = NULL;

      for (j = 0; j < height; ++j) {
         c1 = xi__get8(s);
         c2 = xi__get8(s);
         len = xi__get8(s);
         if (c1 != 2 || c2 != 2 || (len & 0x80)) {
            // not run-length encoded, so we have to actually use THIS data as a decoded
            // pixel (note this can't be a valid pixel--one of RGB must be >= 128)
            xi_uc rgbe[4];
            rgbe[0] = (xi_uc) c1;
            rgbe[1] = (xi_uc) c2;
            rgbe[2] = (xi_uc) len;
            rgbe[3] = (xi_uc) xi__get8(s);
            xi__hdr_convert(hdr_data, rgbe, req_comp);
            i = 1;
            j = 0;
            XI_FREE(scanline);
            goto main_decode_loop; // yes, this makes no sense
         }
         len <<= 8;
         len |= xi__get8(s);
         if (len != width) { XI_FREE(hdr_data); XI_FREE(scanline); return xi__errpf("invalid decoded scanline length", "corrupt HDR"); }
         if (scanline == NULL) {
            scanline = (xi_uc *) xi__malloc_mad2(width, 4, 0);
            if (!scanline) {
               XI_FREE(hdr_data);
               return xi__errpf("outofmem", "Out of memory");
            }
         }

         for (k = 0; k < 4; ++k) {
            int nleft;
            i = 0;
            while ((nleft = width - i) > 0) {
               count = xi__get8(s);
               if (count > 128) {
                  // Run
                  value = xi__get8(s);
                  count -= 128;
                  if ((count == 0) || (count > nleft)) { XI_FREE(hdr_data); XI_FREE(scanline); return xi__errpf("corrupt", "bad RLE data in HDR"); }
                  for (z = 0; z < count; ++z)
                     scanline[i++ * 4 + k] = value;
               } else {
                  // Dump
                  if ((count == 0) || (count > nleft)) { XI_FREE(hdr_data); XI_FREE(scanline); return xi__errpf("corrupt", "bad RLE data in HDR"); }
                  for (z = 0; z < count; ++z)
                     scanline[i++ * 4 + k] = xi__get8(s);
               }
            }
         }
         for (i=0; i < width; ++i)
            xi__hdr_convert(hdr_data+(j*width + i)*req_comp, scanline + i*4, req_comp);
      }
      if (scanline)
         XI_FREE(scanline);
   }

   return hdr_data;
}

static int xi__hdr_info(xi__context *s, int *x, int *y, int *comp)
{
   char buffer[XI__HDR_BUFLEN];
   char *token;
   int valid = 0;
   int dummy;

   if (!x) x = &dummy;
   if (!y) y = &dummy;
   if (!comp) comp = &dummy;

   if (xi__hdr_test(s) == 0) {
       xi__rewind( s );
       return 0;
   }

   for(;;) {
      token = xi__hdr_gettoken(s,buffer);
      if (token[0] == 0) break;
      if (strcmp(token, "FORMAT=32-bit_rle_rgbe") == 0) valid = 1;
   }

   if (!valid) {
       xi__rewind( s );
       return 0;
   }
   token = xi__hdr_gettoken(s,buffer);
   if (strncmp(token, "-Y ", 3)) {
       xi__rewind( s );
       return 0;
   }
   token += 3;
   *y = (int) strtol(token, &token, 10);
   while (*token == ' ') ++token;
   if (strncmp(token, "+X ", 3)) {
       xi__rewind( s );
       return 0;
   }
   token += 3;
   *x = (int) strtol(token, NULL, 10);
   *comp = 3;
   return 1;
}
#endif // XI_NO_HDR

#ifndef XI_NO_BMP
static int xi__bmp_info(xi__context *s, int *x, int *y, int *comp)
{
   void *p;
   xi__bmp_data info;

   info.all_a = 255;
   p = xi__bmp_parse_header(s, &info);
   if (p == NULL) {
      xi__rewind( s );
      return 0;
   }
   if (x) *x = s->img_x;
   if (y) *y = s->img_y;
   if (comp) {
      if (info.bpp == 24 && info.ma == 0xff000000)
         *comp = 3;
      else
         *comp = info.ma ? 4 : 3;
   }
   return 1;
}
#endif

#ifndef XI_NO_PSD
static int xi__psd_info(xi__context *s, int *x, int *y, int *comp)
{
   int channelCount, dummy, depth;
   if (!x) x = &dummy;
   if (!y) y = &dummy;
   if (!comp) comp = &dummy;
   if (xi__get32be(s) != 0x38425053) {
       xi__rewind( s );
       return 0;
   }
   if (xi__get16be(s) != 1) {
       xi__rewind( s );
       return 0;
   }
   xi__skip(s, 6);
   channelCount = xi__get16be(s);
   if (channelCount < 0 || channelCount > 16) {
       xi__rewind( s );
       return 0;
   }
   *y = xi__get32be(s);
   *x = xi__get32be(s);
   depth = xi__get16be(s);
   if (depth != 8 && depth != 16) {
       xi__rewind( s );
       return 0;
   }
   if (xi__get16be(s) != 3) {
       xi__rewind( s );
       return 0;
   }
   *comp = 4;
   return 1;
}

static int xi__psd_is16(xi__context *s)
{
   int channelCount, depth;
   if (xi__get32be(s) != 0x38425053) {
       xi__rewind( s );
       return 0;
   }
   if (xi__get16be(s) != 1) {
       xi__rewind( s );
       return 0;
   }
   xi__skip(s, 6);
   channelCount = xi__get16be(s);
   if (channelCount < 0 || channelCount > 16) {
       xi__rewind( s );
       return 0;
   }
   XI_NOTUSED(xi__get32be(s));
   XI_NOTUSED(xi__get32be(s));
   depth = xi__get16be(s);
   if (depth != 16) {
       xi__rewind( s );
       return 0;
   }
   return 1;
}
#endif

#ifndef XI_NO_PIC
static int xi__pic_info(xi__context *s, int *x, int *y, int *comp)
{
   int act_comp=0,num_packets=0,chained,dummy;
   xi__pic_packet packets[10];

   if (!x) x = &dummy;
   if (!y) y = &dummy;
   if (!comp) comp = &dummy;

   if (!xi__pic_is4(s,"\x53\x80\xF6\x34")) {
      xi__rewind(s);
      return 0;
   }

   xi__skip(s, 88);

   *x = xi__get16be(s);
   *y = xi__get16be(s);
   if (xi__at_eof(s)) {
      xi__rewind( s);
      return 0;
   }
   if ( (*x) != 0 && (1 << 28) / (*x) < (*y)) {
      xi__rewind( s );
      return 0;
   }

   xi__skip(s, 8);

   do {
      xi__pic_packet *packet;

      if (num_packets==sizeof(packets)/sizeof(packets[0]))
         return 0;

      packet = &packets[num_packets++];
      chained = xi__get8(s);
      packet->size    = xi__get8(s);
      packet->type    = xi__get8(s);
      packet->channel = xi__get8(s);
      act_comp |= packet->channel;

      if (xi__at_eof(s)) {
          xi__rewind( s );
          return 0;
      }
      if (packet->size != 8) {
          xi__rewind( s );
          return 0;
      }
   } while (chained);

   *comp = (act_comp & 0x10 ? 4 : 3);

   return 1;
}
#endif

// *************************************************************************************************
// Portable Gray Map and Portable Pixel Map loader
// by Ken Miller
//
// PGM: http://netpbm.sourceforge.net/doc/pgm.html
// PPM: http://netpbm.sourceforge.net/doc/ppm.html
//
// Known limitations:
//    Does not support comments in the header section
//    Does not support ASCII image data (formats P2 and P3)

#ifndef XI_NO_PNM

static int      xi__pnm_test(xi__context *s)
{
   char p, t;
   p = (char) xi__get8(s);
   t = (char) xi__get8(s);
   if (p != 'P' || (t != '5' && t != '6')) {
       xi__rewind( s );
       return 0;
   }
   return 1;
}

static void *xi__pnm_load(xi__context *s, int *x, int *y, int *comp, int req_comp, xi__result_info *ri)
{
   xi_uc *out;
   XI_NOTUSED(ri);

   ri->bits_per_channel = xi__pnm_info(s, (int *)&s->img_x, (int *)&s->img_y, (int *)&s->img_n);
   if (ri->bits_per_channel == 0)
      return 0;

   if (s->img_y > XI_MAX_DIMENSIONS) return xi__errpuc("too large","Very large image (corrupt?)");
   if (s->img_x > XI_MAX_DIMENSIONS) return xi__errpuc("too large","Very large image (corrupt?)");

   *x = s->img_x;
   *y = s->img_y;
   if (comp) *comp = s->img_n;

   if (!xi__mad4sizes_valid(s->img_n, s->img_x, s->img_y, ri->bits_per_channel / 8, 0))
      return xi__errpuc("too large", "PNM too large");

   out = (xi_uc *) xi__malloc_mad4(s->img_n, s->img_x, s->img_y, ri->bits_per_channel / 8, 0);
   if (!out) return xi__errpuc("outofmem", "Out of memory");
   if (!xi__getn(s, out, s->img_n * s->img_x * s->img_y * (ri->bits_per_channel / 8))) {
      XI_FREE(out);
      return xi__errpuc("bad PNM", "PNM file truncated");
   }

   if (req_comp && req_comp != s->img_n) {
      if (ri->bits_per_channel == 16) {
         out = (xi_uc *) xi__convert_format16((xi__uint16 *) out, s->img_n, req_comp, s->img_x, s->img_y);
      } else {
         out = xi__convert_format(out, s->img_n, req_comp, s->img_x, s->img_y);
      }
      if (out == NULL) return out; // xi__convert_format frees input on failure
   }
   return out;
}

static int      xi__pnm_isspace(char c)
{
   return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

static void     xi__pnm_skip_whitespace(xi__context *s, char *c)
{
   for (;;) {
      while (!xi__at_eof(s) && xi__pnm_isspace(*c))
         *c = (char) xi__get8(s);

      if (xi__at_eof(s) || *c != '#')
         break;

      while (!xi__at_eof(s) && *c != '\n' && *c != '\r' )
         *c = (char) xi__get8(s);
   }
}

static int      xi__pnm_isdigit(char c)
{
   return c >= '0' && c <= '9';
}

static int      xi__pnm_getinteger(xi__context *s, char *c)
{
   int value = 0;

   while (!xi__at_eof(s) && xi__pnm_isdigit(*c)) {
      value = value*10 + (*c - '0');
      *c = (char) xi__get8(s);
      if((value > 214748364) || (value == 214748364 && *c > '7'))
          return xi__err("integer parse overflow", "Parsing an integer in the PPM header overflowed a 32-bit int");
   }

   return value;
}

static int      xi__pnm_info(xi__context *s, int *x, int *y, int *comp)
{
   int maxv, dummy;
   char c, p, t;

   if (!x) x = &dummy;
   if (!y) y = &dummy;
   if (!comp) comp = &dummy;

   xi__rewind(s);

   // Get identifier
   p = (char) xi__get8(s);
   t = (char) xi__get8(s);
   if (p != 'P' || (t != '5' && t != '6')) {
       xi__rewind(s);
       return 0;
   }

   *comp = (t == '6') ? 3 : 1;  // '5' is 1-component .pgm; '6' is 3-component .ppm

   c = (char) xi__get8(s);
   xi__pnm_skip_whitespace(s, &c);

   *x = xi__pnm_getinteger(s, &c); // read width
   if(*x == 0)
       return xi__err("invalid width", "PPM image header had zero or overflowing width");
   xi__pnm_skip_whitespace(s, &c);

   *y = xi__pnm_getinteger(s, &c); // read height
   if (*y == 0)
       return xi__err("invalid width", "PPM image header had zero or overflowing width");
   xi__pnm_skip_whitespace(s, &c);

   maxv = xi__pnm_getinteger(s, &c);  // read max value
   if (maxv > 65535)
      return xi__err("max value > 65535", "PPM image supports only 8-bit and 16-bit images");
   else if (maxv > 255)
      return 16;
   else
      return 8;
}

static int xi__pnm_is16(xi__context *s)
{
   if (xi__pnm_info(s, NULL, NULL, NULL) == 16)
  return 1;
   return 0;
}
#endif

static int xi__info_main(xi__context *s, int *x, int *y, int *comp)
{
   #ifndef XI_NO_JPEG
   if (xi__jpeg_info(s, x, y, comp)) return 1;
   #endif

   #ifndef XI_NO_PNG
   if (xi__png_info(s, x, y, comp))  return 1;
   #endif

   #ifndef XI_NO_GIF
   if (xi__gif_info(s, x, y, comp))  return 1;
   #endif

   #ifndef XI_NO_BMP
   if (xi__bmp_info(s, x, y, comp))  return 1;
   #endif

   #ifndef XI_NO_PSD
   if (xi__psd_info(s, x, y, comp))  return 1;
   #endif

   #ifndef XI_NO_PIC
   if (xi__pic_info(s, x, y, comp))  return 1;
   #endif

   #ifndef XI_NO_PNM
   if (xi__pnm_info(s, x, y, comp))  return 1;
   #endif

   #ifndef XI_NO_HDR
   if (xi__hdr_info(s, x, y, comp))  return 1;
   #endif

   // test tga last because it's a crappy test!
   #ifndef XI_NO_TGA
   if (xi__tga_info(s, x, y, comp))
       return 1;
   #endif
   return xi__err("unknown image type", "Image not of any known type, or corrupt");
}

static int xi__is_16_main(xi__context *s)
{
   #ifndef XI_NO_PNG
   if (xi__png_is16(s))  return 1;
   #endif

   #ifndef XI_NO_PSD
   if (xi__psd_is16(s))  return 1;
   #endif

   #ifndef XI_NO_PNM
   if (xi__pnm_is16(s))  return 1;
   #endif
   return 0;
}

#ifndef XI_NO_STDIO
XIDEF int xi_info(char const *filename, int *x, int *y, int *comp)
{
    FILE *f = xi__fopen(filename, "rb");
    int result;
    if (!f) return xi__err("can't fopen", "Unable to open file");
    result = xi_info_from_file(f, x, y, comp);
    fclose(f);
    return result;
}

XIDEF int xi_info_from_file(FILE *f, int *x, int *y, int *comp)
{
   int r;
   xi__context s;
   long pos = ftell(f);
   xi__start_file(&s, f);
   r = xi__info_main(&s,x,y,comp);
   fseek(f,pos,SEEK_SET);
   return r;
}

XIDEF int xi_is_16_bit(char const *filename)
{
    FILE *f = xi__fopen(filename, "rb");
    int result;
    if (!f) return xi__err("can't fopen", "Unable to open file");
    result = xi_is_16_bit_from_file(f);
    fclose(f);
    return result;
}

XIDEF int xi_is_16_bit_from_file(FILE *f)
{
   int r;
   xi__context s;
   long pos = ftell(f);
   xi__start_file(&s, f);
   r = xi__is_16_main(&s);
   fseek(f,pos,SEEK_SET);
   return r;
}
#endif // !XI_NO_STDIO

XIDEF int xi_info_from_memory(xi_uc const *buffer, int len, int *x, int *y, int *comp)
{
   xi__context s;
   xi__start_mem(&s,buffer,len);
   return xi__info_main(&s,x,y,comp);
}

XIDEF int xi_info_from_callbacks(xi_io_callbacks const *c, void *user, int *x, int *y, int *comp)
{
   xi__context s;
   xi__start_callbacks(&s, (xi_io_callbacks *) c, user);
   return xi__info_main(&s,x,y,comp);
}

XIDEF int xi_is_16_bit_from_memory(xi_uc const *buffer, int len)
{
   xi__context s;
   xi__start_mem(&s,buffer,len);
   return xi__is_16_main(&s);
}

XIDEF int xi_is_16_bit_from_callbacks(xi_io_callbacks const *c, void *user)
{
   xi__context s;
   xi__start_callbacks(&s, (xi_io_callbacks *) c, user);
   return xi__is_16_main(&s);
}

#endif // XI_IMAGE_IMPLEMENTATION