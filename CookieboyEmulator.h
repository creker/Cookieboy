#ifndef COOKIEBOYEMULATOR_H
#define COOKIEBOYEMULATOR_H

#include "CookieboyDefs.h"
#include "CookieboyGPU.h"
#include "CookieboyJoypad.h"

namespace Cookieboy
{

class CPU;
class SpeedSwitcher;
class Interrupts;
class Memory;
class DividerTimer;
class TIMATimer;
class Joypad;
class SerialIO;
class Sound;
struct ROMInfo;

class Emulator
{
public:
	Emulator(GPU::DMGPalettes palette = GPU::RGBPALETTE_REAL, int soundSampleRate = 44100, int soundSampleBufferLength = 1024);
	~Emulator();

	const ROMInfo* LoadROM(const char *ROMPath);

	void Step();
	void UpdateJoypad(Joypad::ButtonsState &state);

	void Reset();

	void UseBIOS(bool BIOS);

	const DWORD** GPUFramebuffer();
	bool IsNewGPUFrameReady();
	void WaitForNewGPUFrame();

	const short* SoundFrameBuffer();
	bool IsNewSoundFrameReady();
	void WaitForNewSoundFrame();

	void ToggleLCDBackground();
	void ToggleLCDWindow();
	void ToggleLCDSprites();
	void ToggleSound1();
	void ToggleSound2();
	void ToggleSound3();
	void ToggleSound4();

private:

	bool CGB;
	bool CGBDoubleSpeed;

	CPU *Cpu;
	SpeedSwitcher *CGBSpeedSwitcher;
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