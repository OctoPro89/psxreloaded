#pragma once

#include <core/Types.h>

#ifdef __cplusplus
extern "C" {
#endif
extern u32 platform_audio_output_sample_rate;

u8 platform_audio_init();
void platform_audio_shutdown();
void platform_audio_push(const s16* samples, u32 frames);
u32 platform_audio_get_queued_frames();
void platform_audio_clear();
#ifdef __cplusplus
}
#endif