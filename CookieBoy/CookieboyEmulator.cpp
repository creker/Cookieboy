#include "CookieboyEmulator.h"

Cookieboy::Emulator::Emulator(GPU::RGBPalettes palette, int soundSampleRate, int soundSampleBufferLength)
{
	INT = new Interrupts();
	Gpu = new GPU(palette);
	TIMA = new TIMATimer();
	DIV = new DividerTimer();
	Input = new Joypad();
	Serial = new SerialIO();
	SPU = new Sound(soundSampleRate, soundSampleBufferLength);

	MMU = new Memory(*Gpu, *DIV, *TIMA, *Input, *SPU, *Serial, *INT);
	Cpu = new CPU(*MMU, *Gpu, *DIV, *TIMA, *Input, *SPU, *Serial, *INT);
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

void Cookieboy::Emulator::Step()
{
	Cpu->Step();
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