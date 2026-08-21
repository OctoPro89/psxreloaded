#include "EmscriptenHelpers.h"
#ifdef __EMSCRIPTEN__

#include <emscripten/html5.h>
#include <Host.h>
#include <psx/Bus.h>
#include <psx/MemoryCard.h>
#include <psx/SIO.h>
#include <psx/CD.h>
#include <string.h>

static int currentMemoryCardIndex = 0;
static int currentlySwappingDiscs = 0;

EM_JS(void, __downloadMemoryCard__, (int port, const char* filename), {
	const ptr = Module._GetMemoryCardData(port);
	const size = Module._GetMemoryCardSize();

	// NOTE: slice() COPIES data, subarray() gets a view, using slice() to be safer
	const bytes = HEAPU8.slice(ptr, ptr + size);

	const blob = new Blob([bytes], {
		type: "application/octet-stream"
	});

	const url = URL.createObjectURL(blob);

	const a = document.createElement("a");
	a.href = url;
	a.download = UTF8ToString(filename);
	a.click();

	URL.revokeObjectURL(url);
});

EM_JS(void, __openMemoryCardPicker__, (), {
	const input = document.createElement('input');
	input.type = 'file';
	input.accept = '.mcr,.mc,.mcd';

	input.onchange = async() => {
		const file = input.files[0];
		if (!file)
			return;

		const bytes = new Uint8Array(await file.arrayBuffer());

		const ptr = _malloc(bytes.length);
		HEAPU8.set(bytes, ptr);

		Module._LoadMemoryCardFromBuffer(ptr, bytes.length);

		Module._memfree(ptr);
	};

	input.click();
});

EM_JS(void, __openROMPicker__, (), {
	const input = document.createElement('input');
	input.type = 'file';
	input.accept = '.bin';

	input.onchange = async() => {
		const file = input.files[0];
		if (!file)
			return;

		const bytes = new Uint8Array(await file.arrayBuffer());

		const ptr = _malloc(bytes.length);
		HEAPU8.set(bytes, ptr);

		//const filename = file.name;
		//const dot = filename.lastIndexOf('.');
		//const ext = dot >= 0 ? filename.substring(dot) : "";
		
		Module._LoadROMFromBuffer(ptr, bytes.length);

		//Module._memfree(extPtr);
		Module._memfree(ptr);
	};

	input.click();
});

extern "C" const uint8_t* EMSCRIPTEN_KEEPALIVE GetMemoryCardData(int port)
{
	SIO& sio = Host::GetBus().GetSIO();
	ControllerPort& cport = sio.GetPort(0);
	MemoryCard& card = cport.GetMemoryCard();
	return card.GetData();
}

extern "C" int EMSCRIPTEN_KEEPALIVE GetMemoryCardSize()
{
	return MemoryCard::kSizeBytes;
}

// TODO: use return vals
#include <stdio.h>
extern "C" void EMSCRIPTEN_KEEPALIVE LoadROMFromBuffer(const uint8_t* data, int size)
{
	// TODO: Get actual extension
	EmuWASMFSTools::RomFromBuffer(".bin", data, size);
}

extern "C" void EMSCRIPTEN_KEEPALIVE LoadMemoryCardFromBuffer(const uint8_t* data, int size)
{
	EmuWASMFSTools::MemoryCardFromBuffer(currentMemoryCardIndex, data, size);
}

// Module._free isn't defined in JS for whatever reason???
extern "C" void EMSCRIPTEN_KEEPALIVE memfree(void* block) { free(block); }

void backendDownloadMemoryCard(int port, const char* downloadName) { __downloadMemoryCard__(port, downloadName); }
void frontendOpenMemoryCardPicker(int index) { currentMemoryCardIndex = index; __openMemoryCardPicker__(); }
void frontendOpenROMPicker(int swappingDiscs) { currentlySwappingDiscs = swappingDiscs; __openROMPicker__(); }

int EmuWASMFSTools::MemoryCardFromBuffer(int cardIndex, const uint8_t* data, int size)
{
	SIO& sio = Host::GetBus().GetSIO();
	ControllerPort& cport = sio.GetPort(cardIndex);
	MemoryCard& card = cport.GetMemoryCard();
	if (size != MemoryCard::kSizeBytes) { return 0; }
	memcpy(card.m_data, data, size);
	card.m_flag = 0x08; // Set FLAG bit 3 to indicate directory not yet read, as if the card was just inserted.

	// insert it
	if (!cport.IsMemoryCardInserted()) { cport.SetMemoryCardInserted(true); }
	return 1;
}

int EmuWASMFSTools::RomFromBuffer(const char* extension, const uint8_t* data, int size)
{
	CD& cd = Host::GetCD();

	if (!data || !size || !extension) { return false; }

	if (strcmp(extension, ".bin") == 0)
	{
		if (CD::kPregapSizeBytes + size > CD::kMaxSizeBytes)
		{
			return 0;
		}

		memset(cd.m_data, 0, CD::kPregapSizeBytes);
		memcpy((void*)(cd.m_data + CD::kPregapSizeBytes), data, size);
		cd.m_sizeBytes = CD::kPregapSizeBytes + size;
		cd.m_sizeSectors = cd.m_sizeBytes / CD::kSectorSizeBytes;
		cd.m_numTracks = 1; // assume one track
		cd.m_trackIndex1LBA[0] = 0;

		for (unsigned int i = 1; i < CD::kMaxTracks; i++)
		{
			cd.m_trackDataType[i] = CD::TrackDataType::UNKNOWN;
			cd.m_trackIndex0LBA[i] = 0;
			cd.m_trackIndex1LBA[i] = 0;
		}

		strncpy(cd.m_name, "Uploaded ROM", 256); // TODO

		// insert it
		if (currentlySwappingDiscs)
		{
			Host::GetBus().GetCDROM().SwapDisc(cd);
		}
		else
		{
			Host::GetBus().GetCDROM().InsertDisc(cd);
		}
		return 1;
	}
	else if (strcmp(extension, ".cue") == 0)
	{
		return false; // not currently supported
	}
	else
	{
		return false;
	}

}

#endif // __EMSCRIPTEN__
