#include "CookieboyEmulator.h"
#include "CookieboyCPU.h"
#include "CookieboyInterrupts.h"
#include "CookieboyMemory.h"
#include "CookieboyDividerTimer.h"
#include "CookieboyTIMATimer.h"
#include "CookieboySerialIO.h"
#include "CookieboySound.h"
#include "CookieboyROMInfo.h"

Cookieboy::Emulator::Emulator(GPU::DMGPalettes palette, int soundSampleRate, int soundSampleBufferLength) :
CGB(false),
CGBDoubleSpeed(false)
{
	INT = new Interrupts();
	Gpu = new GPU(CGB, *INT, palette);
	TIMA = new TIMATimer(CGB, CGBDoubleSpeed);
	DIV = new DividerTimer(CGB, CGBDoubleSpeed);
	Input = new Joypad();
	Serial = new SerialIO(CGB, CGBDoubleSpeed);
	SPU = new Sound(CGB, soundSampleRate, soundSampleBufferLength);

	MMU = new Memory(CGB, CGBDoubleSpeed, *Gpu, *DIV, *TIMA, *Input, *SPU, *Serial, *INT);
	Cpu = new CPU(CGB, CGBDoubleSpeed, *MMU, *Gpu, *DIV, *TIMA, *Input, *SPU, *Serial, *INT);
}

Cookieboy::Emulator::~Emulator()
{
	delete Cpu;
	delete Gpu;
	delete DIV;
	delete TIMA;
	delete MMU;
	delete Serial;
	delete SPU;
	delete INT;
	delete Input;
}

const Cookieboy::ROMInfo* Cookieboy::Emulator::LoadROM(const char *ROMPath) 
{
	CGB = false;
	CGBDoubleSpeed = false;

	const ROMInfo *info = MMU->LoadROM(ROMPath);
	
	return info;
}

void Cookieboy::Emulator::Step()
{
	Cpu->Step();
}

void Cookieboy::Emulator::UpdateJoypad(Joypad::ButtonsState &state) 
{ 
	Input->UpdateJoypad(state); 
}

void Cookieboy::Emulator::Reset()
{
	Cpu->Reset();
	Gpu->Reset();
	DIV->Reset();
	TIMA->Reset();
	Input->Reset();
	MMU->Reset();
	Serial->Reset();
	SPU->Reset();
	INT->Reset();
}

void Cookieboy::Emulator::UseBIOS(bool BIOS)
{
	if (BIOS)
	{
		Reset();
	}
	else
	{
		Cpu->EmulateBIOS();
		Gpu->EmulateBIOS();
		DIV->EmulateBIOS();
		TIMA->EmulateBIOS();
		Input->EmulateBIOS();
		MMU->EmulateBIOS();
		Serial->EmulateBIOS();
		SPU->EmulateBIOS();
		INT->EmulateBIOS();
	}
}

const DWORD** Cookieboy::Emulator::GPUFramebuffer() 
{ 
	return Gpu->GetFramebuffer();
}

bool Cookieboy::Emulator::IsNewGPUFrameReady()
{
	return Gpu->IsNewFrameReady(); 
}

void Cookieboy::Emulator::WaitForNewGPUFrame()
{ 
	Gpu->WaitForNewFrame();
}

const short* Cookieboy::Emulator::SoundFrameBuffer() 
{ 
	return SPU->GetSoundFramebuffer();
}

bool Cookieboy::Emulator::IsNewSoundFrameReady()
{ 
	return SPU->IsNewFrameReady(); 
}

void Cookieboy::Emulator::WaitForNewSoundFrame() 
{ 
	return SPU->WaitForNewFrame(); 
}

void Cookieboy::Emulator::ToggleLCDBackground()
{
	Gpu->ToggleBackground();
}

void Cookieboy::Emulator::ToggleLCDWindow()
{
	Gpu->ToggleWindow();
}

void Cookieboy::Emulator::ToggleLCDSprites()
{
	Gpu->ToggleSprites();
}

void Cookieboy::Emulator::ToggleSound1()
{
	SPU->ToggleSound1();
}

void Cookieboy::Emulator::ToggleSound2()
{
	SPU->ToggleSound2();
}

void Cookieboy::Emulator::ToggleSound3()
{
	SPU->ToggleSound3();
}

void Cookieboy::Emulator::ToggleSound4()
{
	SPU->ToggleSound4();
}
