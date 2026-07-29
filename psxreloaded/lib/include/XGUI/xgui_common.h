#ifndef XGUI_COMMON_H
#define XGUI_COMMON_H

#ifdef _WIN32
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif // _WIN32

#include <cstdint>

#define i8 int8_t
#define i16 int16_t
#define i32 int32_t
#define i64 int64_t

#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t

#define f32 float
#define f64 double

#ifdef XGUIBUILDDYNAMIC

#ifdef XEXPORT
// Exports
#ifdef _MSC_VER
#define XAPI __declspec(dllexport)
#else
#define XAPI __attribute__((visibility("default")))
#endif
#else
// Imports
#ifdef _MSC_VER
#define XAPI __declspec(dllimport)
#else
#define XAPI
#endif
#endif

#else

#define XAPI

#endif // XGUIBUILDDYNAMIC

#define XCLAMP(value, min, max) (value <= min) ? min : (value >= max) ? max : value;

// Inlining
#ifdef _MSC_VER
#define XINLINE __forceinline
#define XNOINLINE __declspec(noinline)
#else
#define XINLINE static inline
#define XNOINLINE
#endif

#endif // XGUI_COMMON_H