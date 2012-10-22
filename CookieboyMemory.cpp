#include "CookieboyMemory.h"
#include <stdio.h>

#include "CookieboyMBC.h"
#include "CookieboyDividerTimer.h"
#include "CookieboyTIMATimer.h"
#include "CookieboyInterrupts.h"
#include "CookieboyJoypad.h"
#include "CookieboyGPU.h"
#include "CookieboySerialIO.h"
#include "CookieboySound.h"
#include "CookieboyMBC_ROMOnly.h"
#include "CookieboyMBC1.h"
#include "CookieboyMBC2.h"
#include "CookieboyMBC3.h"
#include "CookieboyMBC5.h"
#include "CookieboyMBC_MMM01.h"

BYTE Cookieboy::Memory::BIOS[0x100] = {	0x31,0xFE,0xFF,0xAF,0x21,0xFF,0x9F,0x32,0xCB,0x7C,0x20,0xFB,0x21,0x26,0xFF,0x0E,
										0x11,0x3E,0x80,0x32,0xE2,0x0C,0x3E,0xF3,0xE2,0x32,0x3E,0x77,0x77,0x3E,0xFC,0xE0,
										0x47,0x11,0x04,0x01,0x21,0x10,0x80,0x1A,0xCD,0x95,0x00,0xCD,0x96,0x00,0x13,0x7B,
										0xFE,0x34,0x20,0xF3,0x11,0xD8,0x00,0x06,0x08,0x1A,0x13,0x22,0x23,0x05,0x20,0xF9,
										0x3E,0x19,0xEA,0x10,0x99,0x21,0x2F,0x99,0x0E,0x0C,0x3D,0x28,0x08,0x32,0x0D,0x20,
										0xF9,0x2E,0x0F,0x18,0xF3,0x67,0x3E,0x64,0x57,0xE0,0x42,0x3E,0x91,0xE0,0x40,0x04,
										0x1E,0x02,0x0E,0x0C,0xF0,0x44,0xFE,0x90,0x20,0xFA,0x0D,0x20,0xF7,0x1D,0x20,0xF2,
										0x0E,0x13,0x24,0x7C,0x1E,0x83,0xFE,0x62,0x28,0x06,0x1E,0xC1,0xFE,0x64,0x20,0x06,
										0x7B,0xE2,0x0C,0x3E,0x87,0xE2,0xF0,0x42,0x90,0xE0,0x42,0x15,0x20,0xD2,0x05,0x20,
										0x4F,0x16,0x20,0x18,0xCB,0x4F,0x06,0x04,0xC5,0xCB,0x11,0x17,0xC1,0xCB,0x11,0x17,
										0x05,0x20,0xF5,0x22,0x23,0x22,0x23,0xC9,0xCE,0xED,0x66,0x66,0xCC,0x0D,0x00,0x0B,
										0x03,0x73,0x00,0x83,0x00,0x0C,0x00,0x0D,0x00,0x08,0x11,0x1F,0x88,0x89,0x00,0x0E,
										0xDC,0xCC,0x6E,0xE6,0xDD,0xDD,0xD9,0x99,0xBB,0xBB,0x67,0x63,0x6E,0x0E,0xEC,0xCC,
										0xDD,0xDC,0x99,0x9F,0xBB,0xB9,0x33,0x3E,0x3C,0x42,0xB9,0xA5,0xB9,0xA5,0x42,0x3C,
										0x21,0x04,0x01,0x11,0xA8,0x00,0x1A,0x13,0xBE,0x20,0xFE,0x23,0x7D,0xFE,0x34,0x20,
										0xF5,0x06,0x19,0x78,0x86,0x23,0x05,0x20,0xFB,0x86,0x20,0xFE,0x3E,0x01,0xE0,0x50};

BYTE Cookieboy::Memory::NintendoGraphic[48] = { 0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
												0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
												0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E};

Cookieboy::Memory::Memory(Cookieboy::GPU &GPU, 
						  Cookieboy::DividerTimer &DIV,
						  Cookieboy::TIMATimer &TIMA,
						  Cookieboy::Joypad &joypad, 
						  Cookieboy::Sound &sound,
						  Cookieboy::SerialIO &serial, 
						  Cookieboy::Interrupts &INT):
GPU(GPU),
DIV(DIV),
TIMA(TIMA),
Joypad(joypad),
Sound(sound),
Serial(serial),
INT(INT),
InBIOS(true),
MBC(NULL),
RAMBanks(NULL),
ROM(NULL)
{
	Reset();
}

Cookieboy::Memory::~Memory()
{
	SaveRAM();

	if (ROM != NULL)
	{
		delete[] ROM;
	}
	if (RAMBanks != NULL)
	{
		delete[] RAMBanks;
	}
	if (MBC != NULL)
	{
		delete[] MBC;
	}
}

const Cookieboy::ROMInfo* Cookieboy::Memory::LoadROM(const char* ROMPath)
{
	FILE* file = NULL;
	fopen_s(&file, ROMPath, "rb");

	if (file == NULL)
	{
		return NULL;
	}

	SaveRAM();
	Reset();

	fseek(file, 0, SEEK_END);
	int filesize = ftell(file);
	fseek(file, 0, SEEK_SET);

	ROM = new BYTE[filesize];
	if (fread(ROM, 1, filesize, file) != filesize)
	{
		delete[] ROM;

		ROMLoaded = false;

		fclose(file);
		return NULL;
	}

	fclose(file);

	//Checking Nintendo scrolling graphic. 
	//Real Gameboy won't run if it's invalid. We are checking just to be sure that input file is Gameboy ROM
	for (int i = 0; i < 48; i++)
	{
		if (NintendoGraphic[i] != ROM[i + 0x104])
		{
			delete[] ROM;
			ROM = NULL;
			ROMLoaded = false;

			return NULL;
		}
	}

	//Checking header checksum. 
	//Real Gameboy won't run if it's invalid. We are checking just to be sure that input file is Gameboy ROM
	BYTE Complement = 0;
	for (int i = 0x134; i <= 0x14C; i++)
	{
		Complement = Complement - ROM[i] - 1; 
	}
	if (Complement != ROM[0x14D])
	{
		delete[] ROM;
		ROM = NULL;
		ROMLoaded = false;

		return NULL;
	}

	LoadedROMInfo.ReadROMInfo(ROM);
	strncpy_s(LoadedROMInfo.ROMFile, ROMPath, 250);
	LoadedROMInfo.ROMFile[250] = '\0';

	if (LoadedROMInfo.ROMSize == 0)
	{
		LoadedROMInfo.ROMSize = filesize / ROMBankSize;
	}

	int RAMSize = (LoadedROMInfo.RAMSize == 0) ? 1 : LoadedROMInfo.RAMSize;
	RAMBanks = new BYTE[RAMSize * RAMBankSize];

	//On power ON cartridge RAM is filled with random values
	srand(clock());
	for (int i = 0; i < RAMSize * RAMBankSize; i++)
	{
		RAMBanks[i] = rand() % 0x100;
	}

	switch(LoadedROMInfo.MMCType)
	{
	case ROMInfo::MMC_ROMONLY:
		MBC = new MBC_ROMOnly(ROM, LoadedROMInfo.ROMSize, RAMBanks, RAMSize);
		break;

	case ROMInfo::MMC_MBC1:
		MBC = new MBC1(ROM, LoadedROMInfo.ROMSize, RAMBanks, RAMSize);
		break;

	case ROMInfo::MMC_MBC2:
		MBC = new MBC2(ROM, LoadedROMInfo.ROMSize, RAMBanks, RAMSize);
		break;

	case ROMInfo::MMC_MBC3:
		MBC = new MBC3(ROM, LoadedROMInfo.ROMSize, RAMBanks, RAMSize);
		break;

	case ROMInfo::MMC_MBC5:
		MBC = new MBC5(ROM, LoadedROMInfo.ROMSize, RAMBanks, RAMSize);
		break;

	case ROMInfo::MMC_MMM01:
		MBC = new MBC_MMM01(ROM, LoadedROMInfo.ROMSize, RAMBanks, RAMSize);
		break;

	default:
		ROMLoaded = false;
		delete[] ROM;
		ROM = NULL;

		return NULL;
	}

	ROMLoaded = true;

	//Restoring RAM contents if there is battery
	LoadRAM();

	return &LoadedROMInfo;
}

void Cookieboy::Memory::Reset()
{
	if (ROM != NULL)
	{
		delete[] ROM;
	}
	if (RAMBanks != NULL)
	{
		delete[] RAMBanks;
	}
	if (MBC != NULL)
	{
		delete[] MBC;
	}

	ROM = NULL;
	RAMBanks = NULL;
	MBC = NULL;

	ROMLoaded = false;

	//On power ON gameboy internal RAM is filled with random values
	srand(clock());
	for (int i = 0; i < 0x2000; i++)
	{
		InternalRAM1[i] = rand() % 0x100;
	}
	for (int i = 0; i < 0x7F; i++)
	{
		InternalRAM2[i] = rand() % 0x100;
	}
}

void Cookieboy::Memory::EmulateBIOS()
{
	InBIOS = false;
}

void Cookieboy::Memory::Write(WORD addr, BYTE value)
{
	switch (addr & 0xF000)
	{
	//ROM bank 0
	case 0x0000:
	case 0x1000:
	case 0x2000:
	case 0x3000:
	//Switchable ROM bank
	case 0x4000:
	case 0x5000:
	case 0x6000:
	case 0x7000:
	//Switchable RAM bank
	case 0xA000:
	case 0xB000:
		if (!InBIOS || addr >= 0x100)
		{
			MBC->Write(addr, value);
		}
		break;

	//Video RAM
	case 0x8000:
	case 0x9000:
		GPU.WriteVRAM(addr - 0x8000, value);
		break;

	//Internal RAM
	case 0xC000:
	case 0xD000:
		InternalRAM1[addr - 0xC000] = value;
		break;

	//Internal RAM echo
	case 0xE000:
		InternalRAM1[addr - 0xE000] = value;
		break;

	case 0xF000:
		switch (addr & 0xF00)
		{
		//Internal RAM echo
		case 0x000:
		case 0x100:
		case 0x200:
		case 0x300:
		case 0x400:
		case 0x500:
		case 0x600:
		case 0x700:
		case 0x800:
		case 0x900:
		case 0xA00:
		case 0xB00:
		case 0xC00:
		case 0xD00:
			InternalRAM1[addr - 0xE000] = value;
			break;

		//Sprite attrib memory
		case 0xE00:
			GPU.WriteOAM(addr - 0xFE00, value);
			break;

		case 0xF00:
			switch (addr & 0xFF)
			{
			case IOPORT_P1:
				Joypad.P1Changed(value);
				break;

			case IOPORT_SB:
				Serial.SBChanged(value);
				break;

			case IOPORT_SC:
				Serial.SCChanged(value);
				break;

			case IOPORT_DIV:
				DIV.DIVChanged(value);
				break;

			case IOPORT_TIMA:
				TIMA.TIMAChanged(value);
				break;

			case IOPORT_TMA:
				TIMA.TMAChanged(value);
				break;

			case IOPORT_TAC:
				TIMA.TACChanged(value);
				break;

			case IOPORT_IF:
				INT.IFChanged(value);
				break;

			//Sound mode 1 registers
			case IOPORT_NR10:
				Sound.NR10Changed(value);
				break;

			case IOPORT_NR11:
				Sound.NR11Changed(value);
				break;

			case IOPORT_NR12:
				Sound.NR12Changed(value);
				break;

			case IOPORT_NR13:
				Sound.NR13Changed(value);
				break;

			case IOPORT_NR14:
				Sound.NR14Changed(value);
				break;

			//Sound mode 2 registers
			case IOPORT_NR21:
				Sound.NR21Changed(value);
				break;

			case IOPORT_NR22:
				Sound.NR22Changed(value);
				break;

			case IOPORT_NR23:
				Sound.NR23Changed(value);
				break;

			case IOPORT_NR24:
				Sound.NR24Changed(value);
				break;

			//Sound mode 3 registers
			case IOPORT_NR30:
				Sound.NR30Changed(value);
				break;

			case IOPORT_NR31:
				Sound.NR31Changed(value);
				break;

			case IOPORT_NR32:
				Sound.NR32Changed(value);
				break;

			case IOPORT_NR33:
				Sound.NR33Changed(value);
				break;

			case IOPORT_NR34:
				Sound.NR34Changed(value);
				break;

			//Sound mode 4 registers
			case IOPORT_NR41:
				Sound.NR41Changed(value);
				break;

			case IOPORT_NR42:
				Sound.NR42Changed(value);
				break;

			case IOPORT_NR43:
				Sound.NR43Changed(value);
				break;

			case IOPORT_NR44:
				Sound.NR44Changed(value);
				break;

			//Misc registers
			case IOPORT_NR50:
				Sound.NR50Changed(value);
				break;

			case IOPORT_NR51:
				Sound.NR51Changed(value);
				break;

			case IOPORT_NR52:
				Sound.NR52Changed(value);
				break;

			case IOPORT_WAVEPATRAM:
			case IOPORT_WAVEPATRAM + 1:
			case IOPORT_WAVEPATRAM + 2:
			case IOPORT_WAVEPATRAM + 3:
			case IOPORT_WAVEPATRAM + 4:
			case IOPORT_WAVEPATRAM + 5:
			case IOPORT_WAVEPATRAM + 6:
			case IOPORT_WAVEPATRAM + 7:
			case IOPORT_WAVEPATRAM + 8:
			case IOPORT_WAVEPATRAM + 9:
			case IOPORT_WAVEPATRAM + 10:
			case IOPORT_WAVEPATRAM + 11:
			case IOPORT_WAVEPATRAM + 12:
			case IOPORT_WAVEPATRAM + 13:
			case IOPORT_WAVEPATRAM + 14:
			case IOPORT_WAVEPATRAM + 15:
				Sound.WaveRAMChanged((addr & 0xFF) - IOPORT_WAVEPATRAM, value);
				break;

			case IOPORT_LCDC:
				GPU.LCDCChanged(value, INT);
				break;

			case IOPORT_STAT:
				GPU.STATChanged(value, INT);
				break;

			case IOPORT_SCY:
				GPU.SCYChanged(value);
				break;

			case IOPORT_SCX:
				GPU.SCXChanged(value);
				break;

			case IOPORT_LY:
				GPU.LYChanged(value);
				break;

			case IOPORT_LYC:
				GPU.LYCChanged(value);
				break;

			case IOPORT_DMA:
				GPU.DMAChanged(value, *this);
				break;

			case IOPORT_BGP:
				GPU.BGPChanged(value);
				break;

			case IOPORT_OBP0:
				GPU.OBP0Changed(value);
				break;

			case IOPORT_OBP1:
				GPU.OBP1Changed(value);
				break;

			case IOPORT_WY:
				GPU.WYChanged(value);
				break;

			case IOPORT_WX:
				GPU.WXChanged(value);
				break;

			case 0x50:
				//Writing to 0xFF50 disables BIOS
				//This is done by last instruction in bootstrap ROM
				InBIOS = false;
				break;

			case 0xFF:
				INT.IEChanged(value);
				break;

			default:
				if (addr >= 0xFF80 && addr <= 0xFFFE)//Internal RAM
				{
					InternalRAM2[addr - 0xFF80] = value;
				}
				break;
			}
		}
		break;
	}
}

BYTE Cookieboy::Memory::Read(WORD addr)
{
	switch (addr & 0xF000)
	{
	//ROM bank 0
	case 0x0000:
	case 0x1000:
	case 0x2000:
	case 0x3000:
	//Switchable ROM bank
	case 0x4000:
	case 0x5000:
	case 0x6000:
	case 0x7000:
	//Switchable RAM bank
	case 0xA000:
	case 0xB000:
		if (InBIOS && addr < 0x100)
		{
			return BIOS[addr];
		}
		else
		{
			return MBC->Read(addr);
		}

	//Video RAM
	case 0x8000:
	case 0x9000:
		return GPU.ReadVRAM(addr - 0x8000);

	//Internal RAM
	case 0xC000:
	case 0xD000:
		return InternalRAM1[addr - 0xC000];

	//Internal RAM echo
	case 0xE000:
		return InternalRAM1[addr - 0xE000];

	case 0xF000:
		switch (addr & 0xF00)
		{
		//Internal RAM echo
		case 0x000:
		case 0x100:
		case 0x200:
		case 0x300:
		case 0x400:
		case 0x500:
		case 0x600:
		case 0x700:
		case 0x800:
		case 0x900:
		case 0xA00:
		case 0xB00:
		case 0xC00:
		case 0xD00:
			return InternalRAM1[addr - 0xE000];

		//Sprite attrib memory
		case 0xE00:
			return GPU.ReadOAM(addr - 0xFE00);

		case 0xF00:
			switch (addr & 0xFF)
			{
			case IOPORT_P1:
				return Joypad.GetP1();

			case IOPORT_SB:
				return Serial.GetSB();

			case IOPORT_SC:
				return Serial.GetSC();

			case IOPORT_DIV:
				return DIV.GetDIV();

			case IOPORT_TIMA:
				return TIMA.GetTIMA();

			case IOPORT_TMA:
				return TIMA.GetTMA();

			case IOPORT_TAC:
				return TIMA.GetTAC();

			case IOPORT_IF:
				return INT.GetIF();

			case IOPORT_NR10:
				return Sound.GetNR10();

			case IOPORT_NR11:
				return Sound.GetNR11();
			
			case IOPORT_NR12:
				return Sound.GetNR12();

			case IOPORT_NR13:
				return Sound.GetNR13();

			case IOPORT_NR14:
				return Sound.GetNR14();

			case IOPORT_NR21:
				return Sound.GetNR21();

			case IOPORT_NR22:
				return Sound.GetNR22();

			case IOPORT_NR23:
				return Sound.GetNR23();

			case IOPORT_NR24:
				return Sound.GetNR24();

			case IOPORT_NR30:
				return Sound.GetNR30();

			case IOPORT_NR31:
				return Sound.GetNR31();

			case IOPORT_NR32:
				return Sound.GetNR32();

			case IOPORT_NR33:
				return Sound.GetNR33();

			case IOPORT_NR34:
				return Sound.GetNR34();

			case IOPORT_NR41:
				return Sound.GetNR41();

			case IOPORT_NR42:
				return Sound.GetNR42();

			case IOPORT_NR43:
				return Sound.GetNR43();

			case IOPORT_NR44:
				return Sound.GetNR44();

			case IOPORT_NR50:
				return Sound.GetNR50();

			case IOPORT_NR51:
				return Sound.GetNR51();

			case IOPORT_NR52:
				return Sound.GetNR52();

			case IOPORT_WAVEPATRAM:
			case IOPORT_WAVEPATRAM + 1:
			case IOPORT_WAVEPATRAM + 2:
			case IOPORT_WAVEPATRAM + 3:
			case IOPORT_WAVEPATRAM + 4:
			case IOPORT_WAVEPATRAM + 5:
			case IOPORT_WAVEPATRAM + 6:
			case IOPORT_WAVEPATRAM + 7:
			case IOPORT_WAVEPATRAM + 8:
			case IOPORT_WAVEPATRAM + 9:
			case IOPORT_WAVEPATRAM + 10:
			case IOPORT_WAVEPATRAM + 11:
			case IOPORT_WAVEPATRAM + 12:
			case IOPORT_WAVEPATRAM + 13:
			case IOPORT_WAVEPATRAM + 14:
			case IOPORT_WAVEPATRAM + 15:
				return Sound.GetWaveRAM((addr & 0xFF) - IOPORT_WAVEPATRAM);

			case IOPORT_LCDC:
				return GPU.GetLCDC();

			case IOPORT_STAT:
				return GPU.GetSTAT();

			case IOPORT_SCY:
				return GPU.GetSCY();

			case IOPORT_SCX:
				return GPU.GetSCX();

			case IOPORT_LY:
				return GPU.GetLY();

			case IOPORT_LYC:
				return GPU.GetLYC();

			case IOPORT_BGP:
				return GPU.GetBGP();

			case IOPORT_OBP0:
				return GPU.GetOBP0();

			case IOPORT_OBP1:
				return GPU.GetOBP1();

			case IOPORT_WY:
				return GPU.GetWY();

			case IOPORT_WX:
				return GPU.GetWX();

			case 0xFF:
				return INT.GetIE();

			default:
				if (addr >= 0xFF80 && addr <= 0xFFFE)//Internal RAM
				{
					return InternalRAM2[addr - 0xFF80];
				}
				else 
				{
					return 0xFF;
				}
			}

		default:
			return 0xFF;
		}
		break;

	default:
		return 0xFF;
	}
}

bool Cookieboy::Memory::SaveRAM()
{
	if (!ROMLoaded || !LoadedROMInfo.BatterySupport || !LoadedROMInfo.RAMSize)
	{
		return true;
	}

	char savefile[255];
	strncpy_s(savefile, LoadedROMInfo.ROMFile, 250);
	savefile[250] = '\0';
	char* dotpos = strrchr(savefile, '.');
	if (dotpos != NULL)
	{
		*dotpos = '\0';
	}
	strncat_s(savefile, ".sav", 4);

	return MBC->SaveRAM(savefile, LoadedROMInfo.RAMSize * RAMBankSize);
}

bool Cookieboy::Memory::LoadRAM()
{
	if (!ROMLoaded || !LoadedROMInfo.BatterySupport || !LoadedROMInfo.RAMSize)
	{
		return true;
	}

	char savefile[255];
	strncpy_s(savefile, LoadedROMInfo.ROMFile, 250);
	savefile[250] = '\0';
	char* dotpos = strrchr(savefile, '.');
	if (dotpos != NULL)
	{
		*dotpos = '\0';
	}
	strncat_s(savefile, ".sav", 4);

	return MBC->LoadRAM(savefile, LoadedROMInfo.RAMSize * RAMBankSize);
}