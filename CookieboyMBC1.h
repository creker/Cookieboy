#ifndef COOKIEBOYMBC1_H
#define COOKIEBOYMBC1_H

#include "CookieboyDefs.h"
#include "CookieboyMBC.h"

namespace Cookieboy
{

/*
This is the first MBC chip for the gameboy. Any newer MBC chips are working similiar.

Note that the memory in range 0000-7FFF is used for both reading from ROM, and for writing to the MBCs Control Registers.

0000-3FFF - ROM Bank 00 (Read Only)
			This area always contains the first 16KBytes of the cartridge ROM.

4000-7FFF - ROM Bank 01-7F (Read Only)
			This area may contain any of the further 16KByte banks of the ROM, allowing to address up to 125 ROM Banks (almost 2MByte). 
			Bank numbers 20h, 40h, and 60h cannot be used, resulting in the odd amount of 125 banks.

A000-BFFF - RAM Bank 00-03, if any (Read/Write)
			This area is used to address external RAM in the cartridge (if any). External RAM is often battery buffered, allowing to store game positions
			or high score tables, even if the gameboy is turned off, or if the cartridge is removed from the gameboy. Available RAM sizes are: 
			2KByte (at A000-A7FF), 8KByte (at A000-BFFF), and 32KByte (in form of four 8K banks at A000-BFFF).

0000-1FFF - RAM Enable (Write Only)
			Before external RAM can be read or written, it must be enabled by writing to this address space. It is recommended to disable external RAM 
			after accessing it, in order to protect its contents from damage during power down of the gameboy. Usually the following values are used:
			  00h  Disable RAM (default)
			  0Ah  Enable RAM
			Practically any value with 0Ah in the lower 4 bits enables RAM, and any other value disables RAM.

2000-3FFF - ROM Bank Number (Write Only)
			Writing to this address space selects the lower 5 bits of the ROM Bank Number (in range 01-1Fh). When 00h is written, the MBC translates that
			to bank 01h also. That doesn't harm so far, because ROM Bank 00h can be always directly accessed by reading from 0000-3FFF.
			But (when using the register below to specify the upper ROM Bank bits), the same happens for Bank 20h, 40h, and 60h.
			Any attempt to address these ROM Banks will select Bank 21h, 41h, and 61h instead.

4000-5FFF - RAM Bank Number - or - Upper Bits of ROM Bank Number (Write Only) This 2bit register can be used to select a RAM Bank in range from 00-03h, 
			or to specify the upper two bits (Bit 5-6) of the ROM Bank number, depending on the current ROM/RAM Mode. (See below.)

6000-7FFF - ROM/RAM Mode Select (Write Only)
			This 1bit Register selects whether the two bits of the above register should be used as upper two bits of the ROM Bank, or as RAM Bank Number.
			  00h = ROM Banking Mode (up to 8KByte RAM, 2MByte ROM) (default)
			  01h = RAM Banking Mode (up to 32KByte RAM, 512KByte ROM)

The program may freely switch between both modes, the only limitiation is that only RAM Bank 00h can be used during Mode 0, 
and only ROM Banks 00-1Fh can be used during Mode 1.
*/
class MBC1 : public MBC
{
public:
	enum MBC1ModesEnum
	{
		MBC1MODE_16_8 = 0,
		MBC1MODE_4_32 = 1
	};

	MBC1(BYTE *ROM, DWORD ROMSize, BYTE *RAMBanks, DWORD RAMSize) : MBC(ROM, ROMSize, RAMBanks, RAMSize)
	{
		RAMEnabled = false;
		Mode = MBC1MODE_16_8;
		RAMOffset = 0;
		ROMOffset = ROMBankSize;
	}

	virtual void Write(WORD addr, BYTE value)
	{
		switch (addr & 0xF000)
		{
		//RAM enable/disable
		case 0x0000:
		case 0x1000:
			RAMEnabled = (value & 0x0F) == 0x0A;
			break;

		//ROM bank switching (5 LSB)
		case 0x2000:
		case 0x3000:
			//Setting 5 LSB (0x1F) of ROMOffset without touching 2 MSB (0x60)
			ROMOffset = (((ROMOffset / ROMBankSize) & 0x60) | (value & 0x1F));
			ROMOffset %= ROMSize;

			if (ROMOffset == 0x00) ROMOffset = 0x01;
			else if (ROMOffset == 0x20) ROMOffset = 0x21;
			else if (ROMOffset == 0x40) ROMOffset = 0x41;
			else if (ROMOffset == 0x60) ROMOffset = 0x61;

			ROMOffset *= ROMBankSize;
			break;

		//RAM bank switching/Writing 2 MSB of ROM bank address
		case 0x4000:
		case 0x5000:
			if (Mode == MBC1MODE_16_8)
			{
				ROMOffset = ((ROMOffset / ROMBankSize) & 0x1F) | ((value & 0x3) << 5);
				ROMOffset %= ROMSize;

				if (ROMOffset == 0x00) ROMOffset = 0x01;
				else if (ROMOffset == 0x20) ROMOffset = 0x21;
				else if (ROMOffset == 0x40) ROMOffset = 0x41;
				else if (ROMOffset == 0x60) ROMOffset = 0x61;

				ROMOffset *= ROMBankSize;
			}
			else
			{
				RAMOffset = value & 0x3;
				RAMOffset %= RAMSize;
				RAMOffset *= RAMBankSize;
			}
			break;

		//MBC1 mode switching
		case 0x6000:
		case 0x7000:
			if ((value & 0x1) == 0)
			{
				Mode = MBC1MODE_16_8;
			}
			else
			{
				Mode = MBC1MODE_4_32;
			}
			break;

		//Switchable RAM bank
		case 0xA000:
		case 0xB000:
			if (RAMEnabled)
			{
				if (Mode == MBC1MODE_16_8)
				{
					RAMBanks[addr - 0xA000] = value;
				}
				else
				{
					RAMBanks[RAMOffset + (addr - 0xA000)] = value;
				}
			}
			break;
		}
	}

	virtual BYTE Read(WORD addr)
	{
		switch (addr & 0xF000)
		{
		//ROM bank 0
		case 0x0000:
		case 0x1000:
		case 0x2000:
		case 0x3000:
			return ROM[addr];

		//ROM bank 1
		case 0x4000:
		case 0x5000:
		case 0x6000:
		case 0x7000:
			return ROM[ROMOffset + (addr - 0x4000)];

		//Switchable RAM bank
		case 0xA000:
		case 0xB000:
			if (RAMEnabled)
			{
				if (Mode == MBC1MODE_16_8)
				{
					return RAMBanks[addr - 0xA000];
				}
				else
				{
					return RAMBanks[RAMOffset + (addr - 0xA000)];
				}
			}
		}

		return 0xFF;
	}

private:
	bool RAMEnabled;
	MBC1ModesEnum Mode;	//Selected MBC1 mode
};

}

#endif