#ifndef COOKIEBOYMBC5_H
#define COOKIEBOYMBC5_H

#include "CookieboyDefs.h"
#include "CookieboyMBC.h"

namespace Cookieboy
{

/*
It is similar to the MBC3 (but no RTC) but can access up to 64mbits of ROM and up to 1mbit of RAM.

The lower 8 bits of the 9-bit rom bank select is written to the 2000-2FFF area while the upper bit
is written to the least significant bit of the 3000-3FFF area.

Writing a value (XXXXBBBB - X = Don't care, B = bank select bits) into 4000-5FFF area will select
an appropriate RAM bank at A000-BFFF if the cart contains RAM. Ram sizes are 64kbit,256kbit, & 1mbit.

Also, this is the first MBC that allows rom bank 0 to appear in the 4000-7FFF range by writing $000 to the rom bank select.
*/
class MBC5 : public MBC
{
public:
	MBC5(BYTE *ROM, DWORD ROMSize, BYTE *RAMBanks, DWORD RAMSize) : MBC(ROM, ROMSize, RAMBanks, RAMSize)
	{
		ROMOffset = ROMBankSize;
		RAMOffset = 0;
	}

	virtual void Write(WORD addr, BYTE value)
	{
		switch (addr & 0xF000)
		{
		//ROM bank switching (8 LSB)
		case 0x2000:
			ROMOffset = ((ROMOffset / ROMBankSize) & 0x100) | value;
			ROMOffset %= ROMSize;
			ROMOffset *= ROMBankSize;
			break;

		//ROM bank switching (1 MSB)
		case 0x3000:
			ROMOffset = ((ROMOffset / ROMBankSize) & 0xFF) | ((value & 0x1) << 9);
			ROMOffset %= ROMSize;
			ROMOffset *= ROMBankSize;
			break;

		//RAM bank switching
		case 0x4000:
		case 0x5000:
			RAMOffset = value & 0xF;
			RAMOffset %= RAMSize;
			RAMOffset *= RAMBankSize;
			break;

		//Switchable RAM bank
		case 0xA000:
		case 0xB000:
			RAMBanks[RAMOffset + (addr - 0xA000)] = value;
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

		//RAM bank 0
		case 0xA000:
		case 0xB000:
			return RAMBanks[RAMOffset + (addr - 0xA000)];
		}

		return 0xFF;
	}
};

}

#endif