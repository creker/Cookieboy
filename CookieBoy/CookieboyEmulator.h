#ifndef COOKIEBOYEMULATOR_H
#define COOKIEBOYEMULATOR_H

#include "CookieboyDefs.h"
#include "CookieboyCPU.h"
#include "CookieboyInterrupts.h"
#include "CookieboyMemory.h"
#include "CookieboyGPU.h"
#include "CookieboyDividerTimer.h"
#include "CookieboyTIMATimer.h"
#include "CookieboyJoypad.h"
#include "CookieboySerialIO.h"
#include "CookieboySound.h"
#include "CookieboyROMInfo.h"

namespace Cookieboy
{

class Emulator
{
public:
	Emulator(GPU::RGBPalettes palette = GPU::RGBPALETTE_REAL, int soundSampleRate = 44100, int soundSampleBufferLength = 1024);
	~Emulator();

	const ROMInfo* LoadROM(const char *ROMPath) { return MMU->LoadROM(ROMPath); }

	void Step();
	void UpdateJoypad(Joypad::ButtonsState &state) { Input->UpdateJoypad(state); }

	void Reset();

	void UseBIOS(bool BIOS);

	const DWORD** GPUFramebuffer() { return Gpu->GetFramebuffer(); }
	bool IsNewGPUFrameReady() { return Gpu->IsNewFrameReady(); }
	void WaitForNewGPUFrame() { Gpu->WaitForNewFrame(); }

	const short* SoundFrameBuffer() { return SPU->GetSoundFramebuffer(); }
	bool IsNewSoundFrameReady() { return SPU->IsNewFrameReady(); }
	void WaitForNewSoundFrame() { return SPU->WaitForNewFrame(); }

private:
	CPU *Cpu;
	GPU *Gpu;
	Interrupts *INT;
	Memory *MMU;
	DividerTimer *DIV;
	TIMATimer *TIMA;
	Joypad *Input;
	SerialIO *Serial;
	Sound *SPU;
};

}

#endif