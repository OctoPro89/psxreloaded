#pragma once

#ifdef __EMSCRIPTEN__

#include <stdint.h>

#ifdef __cplusplus
#include <psx/MemoryCard.h>
class EmuWASMFSTools
{
public:
	static int MemoryCardFromBuffer(int cardIndex, const uint8_t* data, int size);
	static int RomFromBuffer(const char* fileExtension, const uint8_t* data, int size);
};

extern "C" {
#endif // __cplusplus

void backendDownloadMemoryCard(int port, const char* downloadName);
void frontendOpenMemoryCardPicker(int index);
void frontendOpenROMPicker();

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __EMSCRIPTEN__