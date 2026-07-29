#include "AudioDevice.h"

#include "core/Log.h"
#include "core/MathsHelpers.h"
#include "core/StringHelpers.h"
#include "core/hp_assert.h"
#include "core/Helpers.h" // HP_UNUSED

#include <platform/platform_audio.h>

// TODO: remove and just use platform layer
static int s_bufferSizeInSampleFrames;
static bool s_audioPaused;

bool AudioDevice::Init(unsigned int requestedBufferSizeInSampleFrames, unsigned int sampleRate)
{
	s_audioPaused = false;

	// The emulator generates audio in signed 16-bit stereo format.
	// #TODO: Expose this if required.
	if (!platform_audio_init()) { return false; }


	return true;
}

void AudioDevice::Shutdown()
{
	platform_audio_shutdown();
}

bool AudioDevice::IsInitialised()
{
	return true;
}

bool AudioDevice::IsPaused()
{
	return s_audioPaused;
}

// TODO:
void AudioDevice::Pause()
{
	//s_audioPaused = true;
}

// TODO:
void AudioDevice::Resume()
{
	//SDL_ResumeAudioDevice(s_audioDeviceID);
	//s_audioPaused = false;
}

const char* AudioDevice::GetDeviceName()
{
	return "WASAPI Sound Card";
}

unsigned int AudioDevice::GetSampleRate()
{
	return platform_audio_output_sample_rate;
}

unsigned int AudioDevice::GetChannelCount()
{
	return 2; // TODO: get from WASAPI
}

void AudioDevice::PutAudioStreamData(const void* data, unsigned int lengthBytes)
{
	platform_audio_push((const s16*)data, lengthBytes / kAudioFrameSize);
}

unsigned int AudioDevice::GetQueueSizeFrames()
{
	unsigned int queueSizeBytes = platform_audio_get_queued_frames();
	unsigned int queueSizeFrames = queueSizeBytes / kAudioFrameSize;
	return queueSizeFrames;
}

void AudioDevice::ClearAudioStream()
{
	// do nothing for now
}

unsigned int AudioDevice::GetBufferSizeFrames()
{
	return s_bufferSizeInSampleFrames;
}

float AudioDevice::FramesToMs(unsigned int frames, unsigned int sampleRate)
{
	// frames / frames per second = seconds
	return (frames * 1000.0f) / sampleRate;
}


float AudioDevice::GetLatencyMs()
{
	return FramesToMs(s_bufferSizeInSampleFrames, 48000);
}
