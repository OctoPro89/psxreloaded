#include "platform_audio.h"

// TODO: clean up
#define true 1
#define false 0

#define AUDIO_MAX_SOUNDS 32
#define AUDIO_SAMPLE_RATE 48000
#define AUDIO_NUM_CHANNELS 2

#ifdef _WIN32
#pragma comment(lib, "winmm.lib")

#define INITGUID

#include <initguid.h>

DEFINE_GUID(CLSID_MMDeviceEnumerator,
    0xbcde0395, 0xe52f, 0x467c, 0x8e, 0x3d, 0xc4, 0x57, 0x92, 0x91, 0x69, 0x2e);

DEFINE_GUID(IID_IMMDeviceEnumerator,
    0xa95664d2, 0x9614, 0x4f35, 0xa7, 0x46, 0xde, 0x8d, 0xb6, 0x36, 0x17, 0xe6);

DEFINE_GUID(IID_IAudioClient,
    0x1cb9ad4c, 0xdbfa, 0x4c32, 0xb1, 0x78, 0xc2, 0xf5, 0x3b, 0xd5, 0xc6, 0x86);

DEFINE_GUID(IID_IAudioClient2,
    0x726778CD, 0xF60A, 0x4eda, 0x82, 0xDE,
    0xe4, 0x76, 0x10, 0xcd, 0x78, 0xaa);

DEFINE_GUID(IID_IAudioRenderClient,
    0xf294acfc, 0x3146, 0x4483, 0xa7, 0xbf, 0xad, 0xdc, 0xa7, 0xc2, 0x60, 0xe2);

#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <audiopolicy.h>
#include <stdio.h>

static IAudioClient* audio_client = NULL;
static IAudioRenderClient* render_client = NULL;

static HANDLE audio_event = NULL;
static HANDLE thread_handle = NULL;

static volatile u8 running = false;

static UINT32 buffer_frame_count;

#define AUDIO_BUFFER_FRAMES 4096 // 4096 / 48000 = 85ms

typedef struct
{
    s16 samples[AUDIO_BUFFER_FRAMES * 2];

    volatile u32 read_pos;
    volatile u32 write_pos;
} platform_audio_buffer;

static platform_audio_buffer audio_buffer = { 0 };

u32 platform_audio_output_sample_rate = 48000;

static u32 audio_buffer_available(void)
{
    u32 read = audio_buffer.read_pos;
    u32 write = audio_buffer.write_pos;

    if (write >= read)
        return write - read;

    return AUDIO_BUFFER_FRAMES - (read - write);
}

u32 platform_audio_queued_frames(void)
{
    return audio_buffer_available();
}

void platform_audio_push(const s16* samples, u32 frame_count)
{
    u32 write = audio_buffer.write_pos;
    u32 read = audio_buffer.read_pos;

    for (u32 i = 0; i < frame_count; i++)
    {
        u32 next = (write + 1) % AUDIO_BUFFER_FRAMES;

        // Buffer full -> drop newest sample.
        if (next == read)
            break;

        audio_buffer.samples[write * 2 + 0] = samples[i * 2 + 0];
        audio_buffer.samples[write * 2 + 1] = samples[i * 2 + 1];

        write = next;
    }

    audio_buffer.write_pos = write;
}


static void platform_audio_pop(float* output, u32 frame_count)
{
    u32 read = audio_buffer.read_pos;
    u32 write = audio_buffer.write_pos;

    for (u32 i = 0; i < frame_count; i++)
    {
        if (read == write)
        {
            output[i * 2 + 0] = 0.0f;
            output[i * 2 + 1] = 0.0f;
        }
        else
        {
            // s16 -> float32
            output[i * 2 + 0] = audio_buffer.samples[read * 2 + 0] / 32768.0f;
            output[i * 2 + 1] = audio_buffer.samples[read * 2 + 1] / 32768.0f;

            read = (read + 1) % AUDIO_BUFFER_FRAMES;
        }
    }

    audio_buffer.read_pos = read;
}

#define FRAMES_PER_BUFFER 512

DWORD WINAPI audio_thread_proc(void* param)
{
    while (running)
    {
        WaitForSingleObject(audio_event, INFINITE);

        UINT32 padding;
        audio_client->lpVtbl->GetCurrentPadding(audio_client, &padding);

        UINT32 frames_available = buffer_frame_count - padding;
        if (frames_available == 0)
            continue;

        BYTE* data;

        render_client->lpVtbl->GetBuffer(
            render_client,
            frames_available,
            &data);

        platform_audio_pop((float*)data, frames_available);

        render_client->lpVtbl->ReleaseBuffer(
            render_client,
            frames_available,
            0);
    }

    return 0;
}

u8 platform_audio_init() {
    if (FAILED(CoInitialize(NULL))) { return false; }

    IMMDeviceEnumerator* enumerator = NULL;
    IMMDevice* device = NULL;
    WAVEFORMATEX* format = NULL;

    HRESULT hr = CoCreateInstance(
        &CLSID_MMDeviceEnumerator,
        NULL,
        CLSCTX_ALL,
        &IID_IMMDeviceEnumerator,
        (void**)&enumerator
    );

    if (FAILED(hr)) { return false; }

    hr = enumerator->lpVtbl->GetDefaultAudioEndpoint(
        enumerator,
        eRender,
        eConsole,
        &device
    );

    if (FAILED(hr)) { return false; }

    hr = device->lpVtbl->Activate(
        device,
        &IID_IAudioClient2,
        CLSCTX_ALL,
        NULL,
        (void**)&audio_client
    );

    if (FAILED(hr)) { return false; }

    hr = audio_client->lpVtbl->GetMixFormat(audio_client, &format);
    if (FAILED(hr)) { return false; }

    platform_audio_output_sample_rate = format->nSamplesPerSec;

    REFERENCE_TIME buffer_duration = 300000; // change based on latency
    hr = audio_client->lpVtbl->Initialize(
        audio_client,
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
        buffer_duration,
        0,
        format,
        NULL
    );

    if (FAILED(hr)) { return false; }

    audio_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    audio_client->lpVtbl->SetEventHandle(audio_client, audio_event);

    hr = audio_client->lpVtbl->GetService(
        audio_client,
        &IID_IAudioRenderClient,
        (void**)&render_client
    );

    if (FAILED(hr)) { return false; }

    audio_client->lpVtbl->Start(audio_client);

    running = true;
    thread_handle = CreateThread(NULL, 0, audio_thread_proc, NULL, 0, NULL);

    CoTaskMemFree(format);
    device->lpVtbl->Release(device);
    enumerator->lpVtbl->Release(enumerator);

    hr = audio_client->lpVtbl->GetBufferSize(audio_client, &buffer_frame_count);
    if (FAILED(hr))
        return false;

    return true;
}

void platform_audio_shutdown() {
    running = false;
    WaitForSingleObject(thread_handle, INFINITE);
    CloseHandle(thread_handle);
    audio_client->lpVtbl->Stop(audio_client);
    render_client->lpVtbl->Release(render_client);
    audio_client->lpVtbl->Release(audio_client);
    CloseHandle(audio_event);

    CoUninitialize();
}

u32 platform_audio_get_queued_frames(void)
{
    u32 read = audio_buffer.read_pos;
    u32 write = audio_buffer.write_pos;

    if (write >= read)
        return write - read;

    return AUDIO_BUFFER_FRAMES - read + write;
}

#endif // _WIN32

#ifdef __EMSCRIPTEN__

#ifdef PSXRELOADED_WASM_AUDIO

#include <emscripten/html5.h>
#include <stdio.h>

EM_JS(void, wasm_audio_init, (), {
    console.log("wasm_audio_init()");
    try {
        console.log("Creating AudioContext()");
        window.audioCtx = new AudioContext({ sampleRate: 41000 });
        console.log(`Created AudioContext(): ${window.audioCtx}`); 

        window.workletNode = null;

        const audioWorkletSource = `
            class PSXReloadedProcessor extends AudioWorkletProcessor {
			constructor() {
				super();

				// 4096 stereo frames ~= 85 ms at 48 kHz
				this.bufferFrames = 4096;
				this.bufferSamples = this.bufferFrames * 2;

				this.buffer = new Float32Array(this.bufferSamples);

				this.readPos = 0;
				this.writePos = 0;

				this.port.onmessage = (event) => {
					if (event.data.type !== "push")
						return;

					const samples = event.data.samples;

					for (let i = 0; i < samples.length; i += 2) {
						const next =
							(this.writePos + 2) % this.bufferSamples;

						// Full: drop incoming audio.
						if (next === this.readPos)
							break;

						this.buffer[this.writePos] = samples[i];
						this.buffer[this.writePos + 1] = samples[i + 1];

						this.writePos = next;
					}
				};
			}

			process(inputs, outputs) {
				const output = outputs[0];

				const left = output[0];
				const right = output[1];

				for (let i = 0; i < left.length; i++) {
					if (this.readPos !== this.writePos) {
						left[i] = this.buffer[this.readPos];
						right[i] = this.buffer[this.readPos + 1];

						this.readPos =
							(this.readPos + 2) % this.bufferSamples;
					} else {
						left[i] = 0;
						right[i] = 0;
					}
				}

				return true;
			}
		}

		registerProcessor(
			"psxreloaded-processor",
			PSXReloadedProcessor
		);
        `;

        const dataUri = "data:application/javascript," + encodeURIComponent(audioWorkletSource);

        window.audioCtx.audioWorklet.addModule(dataUri).then(() => {
            window.workletNode = new AudioWorkletNode(
                window.audioCtx, "psxreloaded-processor", {
                    numberOfOutputs: 1,
                    outputChannelCount : [2]
                }
            );

            window.workletNode.connect(window.audioCtx.destination);
        });
    } catch (e) {
        console.log(`[PSXReloaded]: Javascript AudioWorklet API failed to initialize. Audio will not be available. Browser Error: ${e}`);
        alert(`[PSXReloaded]: Javascript AudioWorklet API failed to initialize. Audio will not be available. Browser Error: ${e}`);
        return 1;
    }
});

EM_JS(void, wasm_audio_push,
    (const s16* samples, u32 frames), {

    if (!window.workletNode) {
        return;
    }

    const count = frames * 2;

    const input = HEAP16.subarray(
        samples >> 1,
        (samples >> 1) + count
    );

    const output = new Float32Array(count);

    for (let i = 0; i < count; i++) {
        output[i] = input[i] / 32768.0;
    }

    window.workletNode.port.postMessage(
        {
            type: "push",
            samples: output
        },
        [output.buffer]
    );
});


EM_JS(void, wasm_audio_shutdown, (), {
    if (window.audioCtx) {
        window.audioCtx.close();
        window.audioCtx = null;
        window.workletNode = null;
    }
});

bool wasm_audio_initialized = false;

#ifdef __cplusplus
extern "C"
#endif // __cplusplus
void EMSCRIPTEN_KEEPALIVE wasmAudioInit()
{
	if (wasm_audio_initialized) { return; }
    wasm_audio_init();
    wasm_audio_initialized = true;
}

u32 platform_audio_output_sample_rate = 41000; // 48000;

#endif // PSXRELOADED_WASM_AUDIO

u8 platform_audio_init()
{
    return true;
}

void platform_audio_shutdown()
{
    wasm_audio_shutdown();
}

void platform_audio_push(const s16* samples, u32 frames)
{
    if (!wasm_audio_initialized) { return; }
    wasm_audio_push(samples, frames);
}

u32 platform_audio_get_queued_frames()
{
    return 0;
}

#endif // __EMSCRIPTEN__