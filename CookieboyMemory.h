#ifndef COOKIEBOYMEMORY_H
#define COOKIEBOYMEMORY_H

#include "CookieboyDefs.h"
#include "CookieboyROMInfo.h"

namespace Cookieboy
{

class GPU;
class DividerTimer;
class TIMATimer;
class Joypad;
class Sound;
class SerialIO;
class Interrupts;
class MBC;
class SpeedSwitcher;

const int WRAMBankSize = 0x1000;

class Memory
{
public:
	/*
	Registers in the "I/O ports" part of the RAM
	*/
	enum IOPortsEnum
	{
		IOPORT_P1 = 0x00,	//Register for reading joy pad info (R/W)
		IOPORT_SB = 0x01,	//Serial transfer data (R/W)
		IOPORT_SC = 0x02,	//Serial IO control (R/W)
		IOPORT_DIV = 0x04,	//Divider Register. Writing any value sets it to $00 (R/W)
		IOPORT_TIMA = 0x05,	//Timer counter (R/W)
		IOPORT_TMA = 0x06,	//Timer Modulo (R/W)
		IOPORT_TAC = 0x07,	//Timer Control (R/W)
		IOPORT_IF = 0x0F,	//Interrupt Flag (R/W)
		IOPORT_NR10 = 0x10,	//Sound Mode 1 register, sweep register (R/W)
		IOPORT_NR11 = 0x11,	//Sound Mode 1 register, Sound length/Wave pattern duty (R/W)
		IOPORT_NR12 = 0x12,	//Sound Mode 1 register, Envelope (R/W)
		IOPORT_NR13 = 0x13,	//Sound Mode 1 register, Frequency lo (W)
		IOPORT_NR14 = 0x14,	//Sound Mode 1 register, Frequency hi (R/W)
		IOPORT_NR20 = 0x15,	//Not used
		IOPORT_NR21 = 0x16,	//Sound Mode 2 register, Sound Length; Wave Pattern Duty (R/W)
		IOPORT_NR22 = 0x17,	//Sound Mode 2 register, envelope (R/W)
		IOPORT_NR23 = 0x18,	//Sound Mode 2 register, frequency lo data (W)
		IOPORT_NR24 = 0x19,	//Sound Mode 2 register, frequency hi data (R/W)
		IOPORT_NR30 = 0x1A,	//Sound Mode 3 register, Sound on/off (R/W)
		IOPORT_NR31 = 0x1B,	//Sound Mode 3 register, sound length (R/W)
		IOPORT_NR32 = 0x1C,	//Sound Mode 3 register, Select output level (R/W)
		IOPORT_NR33 = 0x1D,	//Sound Mode 3 register, frequency's lower data (W)
		IOPORT_NR34 = 0x1E,	//Sound Mode 3 register, frequency's higher data (R/W)
		IOPORT_NR40 = 0x1F,	//Not used
		IOPORT_NR41 = 0x20,	//Sound Mode 4 register, sound length (R/W)
		IOPORT_NR42 = 0x21,	//Sound Mode 4 register, envelope (R/W)
		IOPORT_NR43 = 0x22,	//Sound Mode 4 register, polynomial counter (R/W)
		IOPORT_NR44 = 0x23,	//Sound Mode 4 register, counter/consecutive; inital (R/W)
		IOPORT_NR50 = 0x24,	//Channel control / ON-OFF / Volume (R/W)
		IOPORT_NR51 = 0x25,	//Selection of Sound output terminal (R/W)
		IOPORT_NR52 = 0x26, //Sound on/off (R/W)
		IOPORT_WAVEPATRAM = 0x30,	//Waveform storage for arbitrary sound data
		IOPORT_LCDC = 0x40,	//LCD Control (R/W)
		IOPORT_STAT = 0x41,	//LCDC Status (R/W)
		IOPORT_SCY = 0x42,	//Scroll Y (R/W)
		IOPORT_SCX = 0x43,	//Scroll X (R/W)
		IOPORT_LY = 0x44,	//LCDC Y-Coordinate. Writing will reset the counter (R)
		IOPORT_LYC = 0x45,	//LY Compare (R/W)
		IOPORT_DMA = 0x46,	//DMA Transfer and Start Address (W)
		IOPORT_BGP = 0x47,	//BG & Window Palette Data (R/W)
		IOPORT_OBP0 = 0x48,	//Object Palette 0 Data (R/W)
		IOPORT_OBP1 = 0x49,	//Object Palette 1 Data (R/W)
		IOPORT_WY = 0x4A,	//Window Y Position (R/W)
		IOPORT_WX = 0x4B,	//Window X Position (R/W)
		IOPORT_KEY1 = 0x4D, //Prepare Speed Switch
		IOPORT_VBK = 0x4F,	//Current Video Memory (VRAM) Bank
		IOPORT_HDMA1 = 0x51,//New DMA Source, High
		IOPORT_HDMA2 = 0x52,//New DMA Source, Low
		IOPORT_HDMA3 = 0x53,//New DMA Destination, High
		IOPORT_HDMA4 = 0x54,//New DMA Destination, Low
		IOPORT_HDMA5 = 0x55,//New DMA Length/Mode/Start
		IOPORT_BGPI = 0x68,	//Background Palette Index
		IOPORT_BGPD = 0x69,	//Background Palette Data
		IOPORT_OBPI = 0x6A, //Sprite Palette Index
		IOPORT_OBPD = 0x6B,	//Sprite Palette Data
		IOPORT_SVBK = 0x70	//WRAM Bank
	};

	enum EmulationModes
	{
		EMULMODE_FORCEDMG = 0,
		EMULMODE_FORCECGB = 1,
		EMULMODE_AUTO = 2
	};

	Memory(bool &_CGB, bool &_CGBDoubleSpeed, SpeedSwitcher &speedSwitcher, GPU &GPU, DividerTimer &DIV, TIMATimer &TIMA, Joypad &joypad, Sound &sound, SerialIO &serial, Interrupts &INT);
	~Memory();

	const ROMInfo* LoadROM(const char* ROMPath, EmulationModes mode = EMULMODE_AUTO);
	bool IsROMLoaded() { return ROMLoaded; }
	void Reset(bool clearROM = false);

	void EmulateBIOS();

	void Write(WORD addr, BYTE value);
	BYTE Read(WORD addr);

	bool SaveRAM();
	bool LoadRAM();

private:

	bool &CGB;
	bool &CGBDoubleSpeed;

	bool InBIOS;
	static BYTE BIOS[0x100];

	//Memory map
											//RAM section				Start    End	Comments
											//-------------------------------------------------------------------------------------------------------------------
	BYTE* ROM;								//ROM banks 0, 1			0x0000 - 0x7FFF - holds entire ROM including ROM banks.
	//BYTE VRAM[0x2000];					//Video RAM					0x8000 - 0x9FFF - Video RAM. Holds tile patterns
	BYTE* RAMBanks;							//Switchable RAM bank		0xA000 - 0xBFFF - holds cartridge RAM banks
	BYTE WRAM[WRAMBankSize * 8];			//Work RAM banks			0xC000 - 0xDFFF
											//Work RAM echo				0xE000 - 0xFDFF
	//BYTE OAM[0xA0];						//Sprite attrib memory		0xFE00 - 0xFE9F - Holds sprites information (position, pattern, flags, color)
											//Unusable RAM				0xFEA0 - 0xFEFF
	//BYTE IOPorts[0x80];					//I/0 ports					0xFF00 - 0xFF7F - holds joypad, GPU, sound, serial port, timers, interrupt registers
	BYTE HRAM[0x7F];						//Hight RAM					0xFF80 - 0xFFFE - Additional Gameboy RAM. Usually holds stack
	//BYTE IE;								//Interrupt enable reg		0xFFFF
											//
											//Commented parts of memory mentioned just for clarification and actually located in appropriate classes

	DWORD WRAMOffset;//Points to current WRAM bank address

	MBC *MBC;

	//Cartridge
	bool ROMLoaded;
	ROMInfo LoadedROMInfo;
	static BYTE NintendoGraphic[48];

	//Hardware units
	SpeedSwitcher &CGBSpeedSwitcher;
	GPU &GPU;
	Joypad &Joypad;
	Sound &Sound;
	DividerTimer &DIV;
	TIMATimer &TIMA;
	SerialIO &Serial;
	Interrupts &INT;

	//Undocumented registers
	BYTE HIDDEN1;
	BYTE HIDDEN2;
	BYTE HIDDEN3;
	BYTE HIDDEN4;
	BYTE HIDDEN5;
};

}

#endif