#ifndef COOKIEBOYMBC2_H
#define COOKIEBOYMBC2_H

#include "CookieboyDefs.h"
#include "CookieboyMBC.h"

namespace Cookieboy
{

/*
0000-3FFF - ROM Bank 00 (Read Only)
			Same as for MBC1.

4000-7FFF - ROM Bank 01-0F (Read Only)
			Same as for MBC1, but only a total of 16 ROM banks is supported.

A000-A1FF - 512x4bits RAM, built-in into the MBC2 chip (Read/Write)
			The MBC2 doesn't support external RAM, instead it includes 512x4 bits of built-in RAM (in the MBC2 chip itself).
			It still requires an external battery to save data during power-off though.
			As the data consists of 4bit values, only the lower 4 bits of the "bytes" in this memory area are used.

0000-1FFF - RAM Enable (Write Only)
			The least significant bit of the upper address byte must be zero to enable/disable cart RAM. For example 
			the following addresses can be used to enable/disable cart RAM: 0000-00FF, 0200-02FF, 0400-04FF, ..., 1E00-1EFF.
			The suggested address range to use for MBC2 ram enable/disable is 0000-00FF.

2000-3FFF - ROM Bank Number (Write Only)
			Writing a value (XXXXBBBB - X = Don't cares, B = bank select bits) into 2000-3FFF area will select an appropriate ROM bank at 4000-7FFF.

The least significant bit of the upper address byte must be one to select a ROM bank. For example the following addresses can be used to select a ROM bank: 2100-21FF, 2300-23FF, 2500-25FF, ..., 3F00-3FFF.
The suggested address range to use for MBC2 rom bank selection is 2100-21FF.
*/
class MBC2 : public MBC
{
public:
	MBC2(BYTE *ROM, DWORD ROMSize, BYTE *RAMBanks, DWORD RAMSize) : MBC(ROM, ROMSize, RAMBanks, RAMSize)
	{
		ROMOffset = ROMBankSize;
		RAMOffset = 0;
	}

	virtual void Write(WORD addr, BYTE value)
	{
		switch (addr & 0xF000)
		{
		//ROM bank switching
		case 0x2000:
		case 0x3000:
			ROMOffset = value & 0xF;
			ROMOffset %= ROMSize;

			if (ROMOffset == 0)
			{
				ROMOffset = 1;
			}

			ROMOffset *= ROMBankSize;
			break;

		//RAM bank 0
		case 0xA000:
		case 0xB000:
			RAMBanks[addr - 0xA000] = value & 0xF;
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
			return RAMBanks[addr - 0xA000] & 0xF;
		}

		return 0xFF;
	}
};

}

#endif